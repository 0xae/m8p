vdb_instance CODE_SNIPPETS dim=16 max_elements=500 M=16 ef_construction=200

## Store various programming concepts as embeddings
store <r1> function to sort an array using quicksort algorithm
llm_embed <r1> <r1t> dim=16
vdb_add CODE_SNIPPETS <r1t> <r1>

store <r3> recursive function to calculate fibonacci sequence
llm_embed <r3> <r3t> dim=16
vdb_add CODE_SNIPPETS <r3t> <r3>

store <r4> asynchronous API call with error handling and retry logic
llm_embed <r4> <r4t> dim=16
vdb_add CODE_SNIPPETS <r4t> <r4>

store <r5> binary search tree insertion and traversal operations
llm_embed <r5> <r5t> dim=16
vdb_add CODE_SNIPPETS <r5t> <r5>

# Now search for something semantically similar
store <query>  search on binary tree
llm_embed <query> <queryt> dim=16
# ret <queryt>

# 
# Search and get most similar snippet
vdb_search CODE_SNIPPETS <queryt> <rx> distance=0.019 ## set distance to -1 to bypass distance check
llm_detokenize <rx> <result_text>
#vdb_destroy CODE_SNIPPETS
# ret <rx2> <queryt> 

# Generate an explanation using LLM
store <prompt> Explain how this programming concept relates to sorting:
store <full_prompt> <prompt> <result_text>
llm_instance <full_prompt> EXPLAIN_01 n_predict=100 temperature=0.3 force=1
llm_instancestatus EXPLAIN_01 <explanation>

# # Calculate a relevance score using matrix operations
# mat8 <score_weights> 1.2 0.8 1.5 0.9 1.1 1.3 0.7 1.0
# mat8 <result_metrics> 0.9 0.85 0.92 0.88 0.91 0.87 0.93 0.89
# matmul8 <score_weights> <result_metrics> <relevance_score>

# ret <result_text> <explanation> <relevance_score>