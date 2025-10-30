vdb_instance MYDB5
store <search_q> regularizacao do modelo iva 106 .
llm_tokenize <search_q> <emb>
align <emb> 16
vdb_search MYDB5 <emb> <rv37>
llm_detokenize <rv37> <result>
return <result>
