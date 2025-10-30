## Semantic Code Snippet Search Engine
## Store programming snippets with natural language descriptions

vdb_instance CODE_SEARCH dim=32 max_elements=100 M=16 ef_construction=200

## Add various code snippets with descriptions
store <s1> Python function to read CSV file and parse data
llm_embed <s1> <e1> dim=32
align <e1> 32
vdb_add CODE_SEARCH <e1> Python: df = pd.read_csv('data.csv')

store <s2> JavaScript async function to fetch API data
llm_embed <s2> <e2> dim=32
align <e2> 32
vdb_add CODE_SEARCH <e2> JavaScript: const data = await fetch(url).then(r => r.json())

store <s3> SQL query to join two tables on foreign key
llm_embed <s3> <e3> dim=32
align <e3> 32
vdb_add CODE_SEARCH <e3> SQL: SELECT * FROM users JOIN orders ON users.id = orders.user_id

store <s4> Python decorator for timing function execution
llm_embed <s4> <e4> dim=32
align <e4> 32
vdb_add CODE_SEARCH <e4> Python: @timeit def my_func(): ...

store <s5> Regular expression to validate email addresses
llm_embed <s5> <e5> dim=32
align <e5> 32
vdb_add CODE_SEARCH <e5> Regex: ^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$

## Now search with natural language queries
store <query> how do I measure how long my function takes
llm_embed <query> <qembed> dim=32
align <qembed> 32
vdb_search CODE_SEARCH <qembed> <match1> distance=0.5

llm_detokenize <match1> <result1>
store <output1> <query>: <result1>

# ret <output1>

## Try another query
store <query2> combine data from multiple database tables
llm_embed <query2> <qembed2> dim=32
align <qembed2> 32
vdb_search CODE_SEARCH <qembed2> <match2> distance=0.5

llm_detokenize <match2> <result2>
store <output2> <query2>: <result2>

# ret <output1> <output2>
## Try semantic similarity between two descriptions
# store <desc1> loading tabular data from files
# store <desc2> parsing spreadsheet information
llm_embed <output1> <ed1> dim=16
llm_embed <output2> <ed2> dim=16
matcosim <ed1> <ed2> <similarity>
store <sim_result> Similarity between descriptions: <similarity>

ret <output1> <output2> <sim_result>