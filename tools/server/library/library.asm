## <- this is a comment
## Basic Operations
f32set <rage> 12.2
i32set <r2> 5
store <r1> ..string... # will store ...string... in register <r1>, store does not support newlines
store <r3> My age is <rage> and i have <r2> friends # store supports interpolation
dup <r1> <r2> # will duplicate register <r1> to <r2>
store <r2> ulala # testing
ret <r1> <r2> # multiple returns

## Assertions
assertcontains <r1> ...string...
assertnotempty <r1>
assertempty <r1>
assertnil <r1>
asserteq <r1> <r2>

## Generating Embeedings
store <r1> Hello there
llm_embed <r1> <rv2> dim=16 ## stores the embeding of <r1> into <rv2> specifies the size of the returning embeedings, <r1> is now: [0.23, 0.232, 0.23, ...]

## Generate tokens (word dictionary positions)
llm_tokenize <r1> <r1tokens> 
llm_detokenize <r1tokens> <r4> 
ret <r4>

## Math operations
## All operations store the result in the first register, use dup if the value is need before changing the original register
f32add <r10> 23.44533
f32sub <r10> 23.44533
f32mul <r10> 23.44533
f32set <r10> 78
f32add <r10> 23.44533
f32sub <r10> 23.44533
i32set <r9> 123
i32add <r9> 123
i32mul <r9> 123

## Matrix operations
matn <r1> 1 376 306 626 263 8368 ... # creates a variable width matrix 
mat8 <r1> 10 20 30 40 50 60 70 89  # a matrix of 8 elements
mat8 <r2> 12.3 20.23 30.23 40.23 50.23 60.23 70 89 
matsub <r1> <r2> <r3> 
matadd <r1> <r2> <r3> 
matmul <r1> <r2> <r3> 
matdot <score_weights> <result_metrics> <result> ## dot product
matcosim <score_weights> <result_metrics> <result> ## cosine similarity
matl2d <score_weights> <result_metrics> <result> ## L2 distance (Euclidean distance)
ret <r3>


## Inference
store <r1> Tell me a joke
llm_instance <r1> instname n_predict=24 temperature=0.5 ## llm_instance will cache the response
llm_instance <r1> instname n_predict=24 temperature=0.5 force=true ## llm_instance will ignore the cache response and eval
llm_instancestatus instname <r3> ##llm_instancestatus will return the output of the llm call

## Vectordb operations
vdb_instance MYDB4 dim=16 max_elements=500 M=16 ef_construction=200
store <r1> DPR/XML Modelo IVA
llm_embed <r1> <rv1> dim=16 ## always set dim, dim by default is 1570
vdb_add MYDB4 <rv1> <r1> # third parameter (<r1>) is what is returned on search match (tokenized)
vdb_search MYDB4 <rv1> <rv37> distance=0.019 # set distance to -1 to bypass check
llm_detokenize <rv37> <result_text> ## will fail if no search is match
return <result_text>

mat8 <r1> 1 2 3 4 5 6 7 8
align <r1> 16
mat8 <r2> 10 20 30 40 50 60 70 80
align <r2> 16
mat8 <r3> 90 100 200 300 400 500 600 700
align <r3> 16
vdb_instance MYDB dim=16 max_elements=500 M=16 ef_construction=200
vdb_add MYDB <r1> 1 2 3 4 5 6 7 8
vdb_add MYDB <r2> 10 20 30 40 50 60 70 80
vdb_add MYDB <r3> 90 100 200 300 400 500 600 700
