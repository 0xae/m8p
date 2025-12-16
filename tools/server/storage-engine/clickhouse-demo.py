from time import gmtime, strftime
import requests
import sys
import json
import re
import uuid
import psycopg2
from psycopg2 import pool
from psycopg2.extras import RealDictCursor

from clickhouse_connect import get_client
from nltk.stem import SnowballStemmer

CLIENT_NAME = "clh_2"
client = stm__get(CLIENT_NAME)
if not client:
    client = get_client(
        host="clickhouse.insolvency.ai", 
        port=8123, 
        username="admin", 
        password="clickhouseAdmin22!N0",
        connect_timeout=5,
    )
    stm__put(CLIENT_NAME, client)

def CLH_QUERY(QUERY, params, json_resp=True, count_q=False):
    # client = get_client(
    #     host="clickhouse.insolvency.ai", 
    #     port=8123, 
    #     username="admin", 
    #     password="clickhouseAdmin22!N0"
    # )

    R = client.query(QUERY, parameters=params)
    # R = client.query(QUERY, parameters={"number": param})
    # print("(CLH_QUERY) Response: ", R)
    if json_resp:
        data = R.result_rows
        columns = R.column_names
        json_result = [dict(zip(columns, row)) for row in data]
        return {'r':json_result, 'count_c':len(json_result)}
    else:
        return {'r':R.result_rows, 'count_c':len(R.result_rows)}

return CLH_QUERY('SELECT * FROM "default"."company_officers" LIMIT 10', [])