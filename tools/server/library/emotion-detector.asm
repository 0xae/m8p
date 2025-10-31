## Emotional Tone Analyzer
vdb_instance EMOTIONS dim=16 max_elements=50 M=16 ef_construction=200

## Build emotion database
store <r1> I am so happy and excited today!
llm_embed <r1> <rv1> dim=16
vdb_add EMOTIONS <rv1> HAPPY

store <r1> This makes me really sad and disappointed
llm_embed <r1> <rv1> dim=16
vdb_add EMOTIONS <rv1> SAD

store <r1> I feel angry and frustrated about this situation
llm_embed <r1> <rv1> dim=16
vdb_add EMOTIONS <rv1> ANGRY

store <r1> I'm curious and interested to learn more
llm_embed <r1> <rv1> dim=16
vdb_add EMOTIONS <rv1> CURIOUS

## Analyze new text
store <r_input> i regret the days to come
llm_embed <r_input> <rv_input> dim=16
vdb_search EMOTIONS <rv_input> <rv_emotion> distance=0.25
llm_detokenize <rv_emotion> <result>

## Return emotion analysis
store <r_result> Detected emotion: <result>
ret <r_result>