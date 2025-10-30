vdb_instance MYDB4 dim=16 max_elements=1500 M=16 ef_construction=200

## add entry
store <r1> First fact
llm_embed <r1> <rv1> dim=16
vdb_add MYDB4 <rv1> <r1>

vdb_search MYDB4 <rv1> <rv37> distance=0.1
return <rv37>
