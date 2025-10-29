# Semantic Code Snippet Classifier
# This program stores code snippets as embeddings and retrieves similar ones

# Initialize the knowledge base
vdb_instance CODE_SNIPPETS

# Store various programming concepts as embeddings
store <r1> function to sort an array using quicksort algorithm
llm_tokenize <r1> <r1t>
align <r1t> 16
vdb_add CODE_SNIPPETS <r1t> <r1>
align <r1t> 8
align <r1t> 16
vdb_add CODE_SNIPPETS <r1t> function to sort an array
align <r1t> 5
align <r1t> 16
vdb_add CODE_SNIPPETS <r1t> function to sort an 

store <r2> how to connect to a database using SQL connection string. 
llm_tokenize <r2> <r2t>
align <r2t> 16
vdb_add CODE_SNIPPETS <r2t> <r2>
align <r2t> 8
align <r2t> 16
vdb_add CODE_SNIPPETS <r2t> how to connect to a database using
align <r2t> 5
align <r2t> 16
vdb_add CODE_SNIPPETS <r2t> how to connect to a database

store <r3> recursive function to calculate fibonacci sequence
llm_tokenize <r3> <r3t>
align <r3t> 16
vdb_add CODE_SNIPPETS <r3t> <r3>
align <r3t> 8
align <r3t> 16
vdb_add CODE_SNIPPETS <r3t> recursive function to calculate
align <r3t> 5
align <r3t> 16
vdb_add CODE_SNIPPETS <r3t> recursive function to

store <r4> asynchronous API call with error handling and retry logic
llm_tokenize <r4> <r4t>
align <r4t> 16
vdb_add CODE_SNIPPETS <r4t> <r4>
align <r4t> 8
align <r4t> 16
vdb_add CODE_SNIPPETS <r4t> asynchronous API call with 
align <r4t> 5
align <r4t> 16
vdb_add CODE_SNIPPETS <r4t> asynchronous API call 

store <r5> binary search tree insertion and traversal operations
llm_tokenize <r5> <r5t>
align <r5t> 16
vdb_add CODE_SNIPPETS <r5t> <r5>
align <r5t> 8
align <r5t> 16
vdb_add CODE_SNIPPETS <r5t> binary search tree insertion  and 
align <r5t> 5
align <r5t> 16
vdb_add CODE_SNIPPETS <r5t> binary search tree insertion and traversal

# Now search for something semantically similar
store <query> binary search tree insert
llm_tokenize <query> <queryt>
align <queryt> 16
# ret <queryt>
# 
# Search and get most similar snippet
vdb_search CODE_SNIPPETS <queryt> <rx>
llm_detokenize <rx> <rx2>
vdb_destroy CODE_SNIPPETS
ret <query> <rx2>

# Generate an explanation using LLM
# store <prompt> Explain how this programming concept relates to sorting:
# llm_detokenize <result> <result_text>
# store <full_prompt> <prompt> <result_text>
# llm_instance <full_prompt> EXPLAIN_01 n_predict=100 temperature=0.3
# llm_instancestatus EXPLAIN_01 <explanation>

# # Calculate a relevance score using matrix operations
# mat8 <score_weights> 1.2 0.8 1.5 0.9 1.1 1.3 0.7 1.0
# mat8 <result_metrics> 0.9 0.85 0.92 0.88 0.91 0.87 0.93 0.89
# matmul8 <score_weights> <result_metrics> <relevance_score>

# ret <result_text> <explanation> <relevance_score>