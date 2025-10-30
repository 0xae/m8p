## Test with real FOMC statements and speeches

## December 2023 FOMC - Dovish pivot, market rallied hard
store <dec2023> The Committee decided to maintain the target range for the federal funds rate at 5.25 to 5.5 percent. The Committee would be prepared to adjust the stance of monetary policy as appropriate if risks emerge. Inflation has eased over the past year but remains elevated.
llm_embed <dec2023> <d23> dim=16
vdb_search FED_LANGUAGE <d23> <dec23_match> distance=0.55
llm_detokenize <dec23_match> <dec23_result>

## July 2023 FOMC - Peak hawkish, final hike of cycle
store <jul2023> The Committee decided to raise the target range for the federal funds rate to 5.25 to 5.5 percent. The Committee will continue to assess additional information and its implications for monetary policy. Inflation remains elevated and the Committee remains highly attentive to inflation risks.
llm_embed <jul2023> <j23> dim=16
vdb_search FED_LANGUAGE <j23> <jul23_match> distance=0.55
llm_detokenize <jul23_match> <jul23_result>

## March 2020 FOMC - Emergency pandemic cuts
store <mar2020> The Committee decided to lower the target range for the federal funds rate to 0 to 0.25 percent. The coronavirus outbreak has harmed communities and disrupted economic activity. The Committee is prepared to use its full range of tools to support the flow of credit.
llm_embed <mar2020> <m20> dim=16
vdb_search FED_LANGUAGE <m20> <mar20_match> distance=0.55
llm_detokenize <mar20_match> <mar20_result>

## September 2021 - Taper announcement coming
store <sep2021> If progress continues broadly as expected the Committee judges that a moderation in the pace of asset purchases may soon be warranted. The path of the economy continues to depend on the course of the virus. Supply and demand imbalances have contributed to elevated levels of inflation.
llm_embed <sep2021> <s21> dim=16
vdb_search FED_LANGUAGE <s21> <sep21_match> distance=0.60
llm_detokenize <sep21_match> <sep21_result>

## August 2022 Jackson Hole - Powell hawkish pivot speech
store <jh2022> Restoring price stability will require maintaining a restrictive policy stance for some time. The historical record cautions strongly against prematurely loosening policy. We must keep at it until the job is done. While higher interest rates will bring some pain to households and businesses these are the unfortunate costs of reducing inflation.
llm_embed <jh2022> <jh22> dim=16
vdb_search FED_LANGUAGE <jh22> <jh22_match> distance=0.55
llm_detokenize <jh22_match> <jh22_result>

## June 2019 - Dovish insurance cuts era
store <jun2019> The Committee will closely monitor the implications of incoming information for the economic outlook and will act as appropriate to sustain the expansion. Uncertainties about this outlook have increased. In light of these uncertainties the case for somewhat more accommodative policy has strengthened.
llm_embed <jun2019> <j19> dim=16
vdb_search FED_LANGUAGE <j19> <jun19_match> distance=0.60
llm_detokenize <jun19_match> <jun19_result>

## Calculate hawkish/dovish scores for each
store <hawk_ref> hawkish restrictive vigilant inflation risks higher longer tightening
llm_embed <hawk_ref> <href> dim=16
store <dove_ref> dovish easing accommodation cuts support lower rates
llm_embed <dove_ref> <dref> dim=16

matcosim <d23> <href> <dec23_hawk>
matcosim <d23> <dref> <dec23_dove>
matcosim <j23> <href> <jul23_hawk>
matcosim <j23> <dref> <jul23_dove>
matcosim <m20> <href> <mar20_hawk>
matcosim <m20> <dref> <mar20_dove>
matcosim <jh22> <href> <jh22_hawk>
matcosim <jh22> <dref> <jh22_dove>
matcosim <j19> <href> <jun19_hawk>
matcosim <j19> <dref> <jun19_dove>

## Build historical Fed stance timeline
store <timeline> DEC2023: <dec23_result> Hawk=<dec23_hawk> Dove=<dec23_dove> | JUL2023: <jul23_result> Hawk=<jul23_hawk> Dove=<jul23_dove> | MAR2020: <mar20_result> Hawk=<mar20_hawk> Dove=<mar20_dove> | SEP2021: <sep21_result> | AUG2022_JH: <jh22_result> Hawk=<jh22_hawk> Dove=<jh22_dove> | JUN2019: <jun19_result> Hawk=<jun19_hawk> Dove=<jun19_dove>

ret <timeline>