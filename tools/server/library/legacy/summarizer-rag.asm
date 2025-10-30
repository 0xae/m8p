vdb_instance SUMMARY_PATTERNS

# Build pattern library
store <p1> multiple results found various items
store <p2> no results nothing found
store <p3> partial results some items
llm_tokenize <p1> <p1t>
align <p1t> 16
llm_tokenize <p2> <p2t>
align <p2t> 16
llm_tokenize <p3> <p3t>
align <p3t> 16
vdb_add SUMMARY_PATTERNS <p1t>
vdb_add SUMMARY_PATTERNS <p2t>
vdb_add SUMMARY_PATTERNS <p3t>

# Process input
store <input> found nothing
llm_tokenize <input> <input_t>
align <input_t> 16
vdb_search SUMMARY_PATTERNS <input_t> <pattern_match>
llm_detokenize <pattern_match> <pattern_text>

# Calculate similarity score
mat8 <input_vec> 0.2 0.1 0.9 0.8 0.1 0.2 0.1 0.15
mat8 <pattern_vec> 0.15 0.12 0.95 0.85 0.08 0.18 0.09 0.12
matmul8 <input_vec> <pattern_vec> <sim_matrix>

f32set <confidence> 0.87
f32set <threshold> 0.75

# Build dynamic prompt (single line)
store <prompt> Pattern detected: <pattern_text> | Confidence: <confidence> | Input text: <input> | Based on this pattern, generate a concise summary:

llm_instance <prompt> GEN_SUMMARY4 n_predict=25 temperature=0.3 force=1
llm_instancestatus GEN_SUMMARY4 <result>

ret <prompt> <result>