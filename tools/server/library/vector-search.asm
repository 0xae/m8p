vdb_instance MYDB4 dim=16 max_elements=150 M=16 ef_construction=200

store <search_q> regularizacao do modelo iva 106 .
llm_embed <r1> <rv1> dim=16
vdb_search MYDB4 <rv5> <rv37> distance=0.1

ret <rv37>
