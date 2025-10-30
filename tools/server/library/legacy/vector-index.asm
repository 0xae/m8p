store <r1> modelo de crescimento economico em 2017, 2018 e 2019
llm_tokenize <r1> <rv1>
align <rv1> 16

store <r2> modelo de previsao economica em 2017. , .
llm_tokenize <r2> <rv2>
align <rv2> 16

store <r3> modelo de previsao economica em 2019. , .
llm_tokenize <r3> <rv3>
align <rv3> 16

store <r4> modelo de previsao economica em 2020. , .
llm_tokenize <r4> <rv4>
align <rv4> 16

store <r5> relatorio de contas em 2020 . , . .
llm_tokenize <r5> <rv5>
align <rv5> 16

store <r6> relatorio de contas em 2010 . , . .
llm_tokenize <r6> <rv6>
align <rv6> 16

store <r7> folha de rosto modelo iva 106 . . . .
llm_tokenize <r7> <rv7>
align <rv7> 16

store <r8> folha de cliente modelo iva 106 . . . .
llm_tokenize <r8> <rv8>
align <rv8> 16

store <r9> folha de fornecedor modelo iva 106 . . . .
llm_tokenize <r9> <rv9>
align <rv9> 16

store <r10> folha de regularizacao do modelo iva 106 . . . .
llm_tokenize <r10> <rv10>
align <rv10> 16

vdb_instance MYDB5
vdb_add MYDB5 <rv1>
vdb_add MYDB5 <rv2>
vdb_add MYDB5 <rv3>
vdb_add MYDB5 <rv4>
vdb_add MYDB5 <rv5>
vdb_add MYDB5 <rv6>
vdb_add MYDB5 <rv7>
vdb_add MYDB5 <rv8>
vdb_add MYDB5 <rv9>
vdb_add MYDB5 <rv10>

