## Basic Operations
f32set <rage> 12.2
i32set <r2> 5
store <r1> ..string... # will store ...string... in register <r1>, store does not support newline
store <r3> My age is <rage> and i have <r2> friends # store supports interpolation
dup <r1> <r2> # will duplicate register <r1> to <r2>
store <r2> ulala # testing
ret <r1> <r2> # multiple returns

## Assertions
assertcontains <r1> ...string...
assertnotempty <r1>
assertempty <r1>
assertnil <r1>

## Generating Embeedings
store <r1> Hello there
store <r1c> Hello there
store <r2> My name is ayrton carlos correia gomes ... 
asserteq <r1> <r1c>
llm_tokenize <r1> <r1tokens> 
align <r1tokens> 8 # makes sure array contains 8 elements 
llm_detokenize <r1tokens> <r4> 
ret <r4>

## Math operations
f32add <r10> 23.44533
f32sub <r10> 23.44533
i32set <r9> 123
f32set <r10> 78
f32add <r10> 23.44533
f32sub <r10> 23.44533
f32mul <r10> 23.44533

## Matrix operations
matn <r1> 1 376 306 626 263 8368 ... # creates a variable width matrix 
mat8 <r1> 10 20 30 40 50 60 70 89  # a matrix of 8 elements
mat8 <r2> 12.3 20.23 30.23 40.23 50.23 60.23 70 89 
matsub <r1> <r2> <r3> 
matadd <r1> <r2> <r3> 
matmul <r1> <r2> <r3> 
ret <r3>


## Inference
store <r1> Tell me a joke
llm_instance <r1> instname n_predict=24 temperature=0.5 ## llm_instance will cache the response
llm_instance <r1> instname n_predict=24 temperature=0.5 force=true ## llm_instance will ignore the cache response and eval
llm_instancestatus instname <r3> ##llm_instancestatus will return the output of the llm call

## Vectordb operations
store <r1> DPR/XML Modelo IVA
llm_tokenize <r1> <rv1>
align <rv1> 16
vdb_instance MYDB4
vdb_add MYDB4 <rv1>
vdb_search MYDB4 <rv5> <rv37>
return <rv37>

mat8 <r1> 1 2 3 4 5 6 7 8
align <r1> 16
mat8 <r2> 10 20 30 40 50 60 70 80
align <r2> 16
mat8 <r3> 90 100 200 300 400 500 600 700
align <r3> 16
vdb_instance MYDB
vdb_add MYDB <r1>
vdb_add MYDB <r2>
vdb_add MYDB <r3>
