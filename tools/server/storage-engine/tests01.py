import json
import requests
import sys
import unittest
import argparse
import math
# from websitegen import generate_html_report
from time import gmtime, strftime
from pathlib import Path

from M8P import M8API
from fixtures import get_fixtures

try:
    from websitegen import generate_html_report
    import xmltodict
except Exception as e:
    print("RUN `pip install xmltodict` to get GPU configuration")

import json
import subprocess

API_BASE_URL = "http://127.0.0.1:8090"
CHAT_ENDPOINT = f"{API_BASE_URL}/v1/chat/completions"
HEALTH_ENDPOINT = f"{API_BASE_URL}/health"  # Standard llama.cpp health check
DETAILS_ENDPOINT = f"{API_BASE_URL}/props"  # Standard llama.cpp health check

GENERATION_CONFIG = {
    "temperature": 0.1,  # Low temp for consistent testing
    "max_tokens": -1,
    "stream": False
}

EXEC_TIMES = 1

DEFAULT_TESTS_CASES = get_fixtures()

class MetricsAggregator:
    def __init__(self):
        self.samples = []

    def add(self, metrics):
        """Ingest a single metrics dictionary."""
        # Calculate derived metrics immediately so we can track them too
        total_latency = metrics.get('prompt_ms', 0) + metrics.get('predicted_ms', 0)
        
        # Store a simplified object with the fields we care about
        clean_sample = {
            'tps': metrics.get('predicted_per_second', 0),
            'latency_gen': metrics.get('predicted_ms', 0),
            'latency_prompt': metrics.get('prompt_ms', 0),
            'latency_total': total_latency,
            'tokens_gen': metrics.get('predicted_n', 0)
        }
        self.samples.append(clean_sample)

    def _calculate_stats(self, key):
        """Helper to calculate min, max, avg, p99 for a specific key."""
        values = [s[key] for s in self.samples]
        if not values:
            return 0, 0, 0, 0
            
        values.sort()
        n = len(values)
        
        _min = values[0]
        _max = values[-1]
        _avg = sum(values) / n
        
        # P99 Calculation (Nearest Rank)
        p99_index = int(math.ceil(0.99 * n)) - 1
        _p99 = values[max(0, p99_index)]
        
        return _min, _max, _avg, _p99

    def print_summary(self):
        if not self.samples:
            print("No metrics collected.")

        print(f"\n{'METRIC':<20} | {'AVG':<10} | {'P99':<10} | {'MIN':<10} | {'MAX':<10}")
        print("-" * 70)

        metrics_resp = []

        metrics_to_report = [
            ("Tokens/Sec (Speed)", 'tps', "t/s"),
            ("Latency (Total)", 'latency_total', "ms"),
            ("Latency (Gen)", 'latency_gen', "ms"),
            ("Latency (Prompt)", 'latency_prompt', "ms"),
            ("Tokens Generated", 'tokens_gen', "")
        ]

        for label, key, unit in metrics_to_report:
            _min, _max, _avg, _p99 = self._calculate_stats(key)
            
            # Formatting: 2 decimal places
            row = (
                f"{label:<20} | "
                f"{_avg:<10.2f} | "
                f"{_p99:<10.2f} | "
                f"{_min:<10.2f} | "
                f"{_max:<10.2f}"
            )
            print(row)

            metrics_resp.append({
                'label' : label,
                'key' : key,
                'avg' : _avg,
                'min' : _min,
                'max' : _max,
                'p99' : _p99,
            })

        print("-" * 70)
        print(f"Total Requests: {len(self.samples)}")

        return metrics_resp


class TestM8CoreInstructionSet(unittest.TestCase):
    def setUp(self):
        try:
            resp = requests.get(DETAILS_ENDPOINT)
            model_conf = resp.json()
            model_name = model_conf['model_path']
            # print(f"MODEL: {model_name}")
        except Exception as e:
            print("FAILED TO GET MODEL DETAILS: ", e)
            pass

        # self.test_cases = json.load(f)
        self.test_cases = DEFAULT_TESTS_CASES

    def test_dynamic_json_cases(self):
        print("=============================================")
        print(f"[Dynamic Test] Running cases ")
        agg = MetricsAggregator()
        statistics = {
            'failed' : 0,
            'correct' : 0,
            'uncertain' : 0,
            't_calls' : 0,
            'std' : {},
        }

        exec_c = 1
        total_tests_run = len(self.test_cases) * EXEC_TIMES

        i = 0
        while i < EXEC_TIMES:
            try:
                for case in self.test_cases:
                    with self.subTest(case_id=case['id']):
                        case_id = case['id']
                        asm_code = case['content'].strip()
                        print(f"-> Test {exec_c}/{total_tests_run} :{case['title']}")
                        hasError = False

                        ret = M8API.RunScript(asm_code, host="http://127.0.0.1:8090") ## dry session
                        status = ret['Status']
                        ret_val = ret.get('R', 'NO_RESPONSE')

                        if not statistics['std'].get(case_id):
                            statistics['std'][case_id] = {'s':0, 'e':0, 'u':0, 'title':case['title']}

                        if ret_val=='NO_RESPONSE':
                            if ret.get('Error'):
                                hasError=True
                                print(f"Error {status} # {ret['Error']} ")
                            else: 
                                print(f"Error {status} # {ret} ")
                        else:
                            cmp_val = json.dumps(ret_val)
                            if 'expected_value' in case.keys():
                                evalue = case['expected_value']
                                try:
                                    self.assertEqual(evalue, cmp_val)
                                except Exception as e:
                                    statistics['failed'] += 1
                                    statistics['std'][case_id]['e'] += 1                                    
                                    raise e

                            print(f"Response {status} # {ret_val} ")

                        if status!='OK' and not case.get('expected_to_failed'):
                            statistics['failed'] += 1
                            statistics['std'][case_id]['e'] += 1
                            if hasError:
                                self.assertTrue(False, f"SCRIPT ERRORED {case['title']}#{case_id}: {ret['Error']}\n=========== CODE\n{asm_code}\n========", )
                            else:
                                self.assertTrue(False, f"FAILED: {ret_val}")

                        statistics['std'][case_id]['s'] += 1
                        statistics['correct'] += 1
                        exec_c += 1
            finally:
                i += 1

        model_name = ""
        try:
            resp = requests.get(DETAILS_ENDPOINT)
            model_conf = resp.json()
            model_name = model_conf['model_path']
        except Exception as e:
            print("FAILED TO GET MODEL DETAILS: ", e)

        print("=====================================================")
        print("                   TEST PARAMS                       ")
        print("=====================================================")
        print(f"MODEL: {model_name}")
        print(f"EXEC_TIMES: {EXEC_TIMES}")

        html_TEST_PARAMS = {
            'MODEL' : model_name,
            "EXEC_TIMES" : EXEC_TIMES,
        }

        html_RESULTS = {
            'total_tests_run' : total_tests_run,
            'success_count' : statistics['correct'],
            'failure_count' : statistics['failed'],
            'uncertain_count' : statistics['uncertain']
        }

        print("=====================================================")
        print("                   TEST RESULTS                      ")
        print("=====================================================")

        print(f"Tests run: {total_tests_run} tests")
        print(f"Success: {statistics['correct']}")
        print(f"Failed:  {statistics['failed']}")
        print(f"Uncertain: {statistics['uncertain']}")

        for K in statistics['std'].keys():
            m = statistics['std'][K]
            print(f"[success={m['s']}, error={m['e']}] {m['title']}")

        print("================================")
        print(f"Success Rate: {(statistics['correct']/total_tests_run) * 100}%")
        print(f"Error Rate: {(statistics['failed']/total_tests_run) * 100}%")

        # print("html_TEST_PARAMS: ", html_TEST_PARAMS)
        # print("html_RESULTS: ", html_RESULTS)
        # print(statistics['std'])

        try:
            mn_name = model_name
            m_realname = model_name
            if len(model_name)>0:
                path = Path(model_name)
                mn_name = path.name
                m_realname = mn_name
                mn_name = mn_name.replace('.gguf', '')

            tms = strftime("%Y%m%d%H%M%S", gmtime())
            # sitename = mn_name + '-' + tms + ".html"
            sitename = "tests.php"
            website_html = generate_html_report(
                html_TEST_PARAMS, 
                html_RESULTS, 
                statistics['std'],
            )

            with open(sitename, "w") as f:
                f.write(website_html)

            print("GENERATE TESTS: " + sitename)

        except Exception as e:
            print("FAILED TO GENERATE WEB-REPORT: ", e)


if __name__ == '__main__':
    try:
        parser = argparse.ArgumentParser(description="M8P Microprocessor HTTP Test Suite")
        parser.add_argument("--exectime", type=int, default=1, help="How many times to execute the testing")
        parser.add_argument("--thinkmode", type=str, default="", help="Enable special thinking mode for prompts (none/deep/chain)")
        parser.add_argument("--temperature", type=float, default=0, help="Sampling temperature override")
        parser.add_argument("--testcases", type=str, default="odoo_test_cases.json", help="Comma-separated list of tests to run")
        # # args = parser.parse_args()
        args, unknown = parser.parse_known_args()
        # if args.temperature>=0:
        #     GENERATION_CONFIG['temperature'] = args.temperature
        if args.exectime>=1:
            EXEC_TIMES = args.exectime
        # if len(args.thinkmode)>0:
        #     think_mode = args.thinkmode
        # if len(args.testcases)>0:
        #     TEST_CASE_FILE = args.testcases
    except Exception as e:
        print("FAILED TO SET PARAMTERS: ", e)

    unittest.main(argv=[sys.argv[0]] + unknown)