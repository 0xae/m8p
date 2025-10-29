# The M8 Microprocessor C++ Implementation


```
          |\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\|
          |/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/|
          |\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\|
          |_________________________________________|
__________|                                         |__________
__________|                                         |__________
__________|                                         |__________
__________|       /$$      /$$  /$$$$$$             |__________
__________|      | $$$    /$$$ /$$__  $$            |__________
__________|      | $$$$  /$$$$| $$  \ $$            |__________
__________|      | $$ $$/$$ $$|  $$$$$$/            |__________
__________|      | $$  $$$| $$ >$$__  $$            |__________
__________|      | $$\  $ | $$| $$  \ $$            |__________
__________|      | $$ \/  | $$|  $$$$$$/            |__________
__________|      |__/     |__/ \______/             |__________
__________|                                         |__________
__________|           LLM MICROPROCESSOR            |__________
__________|                                         |__________
__________|_________________________________________|__________
          |_________________________________________|
          |\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\|
          |/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/|
          |\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\|
```


This is the m8 c++ codebase implementation. Supporting the full instruction set
and developed in tandem with the golang codebase.

This implementations is based off llama.cpp and ships the whole llama runtime inside the M8 interpreter/VM.

The vm codebase is in [m8p core](./tools/server/m8p.h).
The server is [here](./tools/server)

## Build
BUILD README IS [here](./docs/build.md)
We use the same build toolchain as llama, then type:

```make llama-server```

# Run Server
```
./llama-server -m ~/models/mistral-7b-instruct-v0.2.Q4_K_M.gguf  -t 20 --port 8090 --host 0.0.0.0
```

## Instruction Set Library
View complete instruction set [here](./tools/server/library/library.asm)

### Basic Operations
```asm
## Basic Operations
f32set <rage> 12.2
i32set <r2> 5
store <r1> ..string... # will store ...string... in register <r1>, store does not support newlines
store <r3> My age is <rage> and i have <r2> friends # store supports interpolation
dup <r1> <r2> # will duplicate register <r1> to <r2>
store <r2> ulala # testing
ret <r1> <r2> # multiple returns
```

### Assertions
```asm
assertcontains <r1> ...string...
assertnotempty <r1>
assertempty <r1>
assertnil <r1>
asserteq <r1> <r2>
```
### Generating Embeedings
```asm
store <r1> Hello there
llm_embed <r1> <rv2> dim=16 ## stores the embeding of <r1> into <rv2> specifies the size of the returning embeedings, <r1> is now: [0.23, 0.232, 0.23, ...]
```

### Generate tokens (word dictionary positions)
```asm
changing the original register
llm_tokenize <r1> <r1tokens> 
llm_detokenize <r1tokens> <r4> 
ret <r4>
```

### Math operations
```asm
## All operations store the result in the first register, use dup if the value is need before 
f32add <r10> 23.44533
f32sub <r10> 23.44533
f32mul <r10> 23.44533
f32set <r10> 78
f32add <r10> 23.44533
f32sub <r10> 23.44533
i32set <r9> 123
i32add <r9> 123
i32mul <r9> 123
```
### Matrix operations
```asm
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
```
### Inference
```asm
store <r1> Tell me a joke
llm_instance <r1> instname n_predict=24 temperature=0.5 ## llm_instance will cache the response
llm_instance <r1> instname n_predict=24 temperature=0.5 force=true ## llm_instance will ignore the cache response and eval
llm_instancestatus instname <r3> ##llm_instancestatus will return the output of the llm call
```
### Vectordb operations (Uses HNSWLIB)
```asm
vdb_instance MYDB4 dim=16 max_elements=500 M=16 ef_construction=200
store <r1> DPR/XML Modelo IVA
llm_embed <r1> <rv1> dim=16 ## always set dim, dim by default is 1570
vdb_add MYDB4 <rv1> <r1> # third parameter (<r1>) is what is returned on search match (tokenized)
vdb_search MYDB4 <rv1> <rv37> distance=0.019 # set distance to -1 to bypass check
llm_detokenize <rv37> <result_text> ## will fail if no search is match
return <result_text>
```

```asm
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
```

Ships with these routes:
- POST ```/api/v1/m8/dry-run``` - Run a temporary execution (M8 instance is destroyed after request finished and all resources released)

- POST ```/api/v1/m8/session-create/:id_session``` - Create a persistent session.

- POST ```/api/v1/m8/session-check/:id_session``` - Conditionally create a persistent session.

- POST ```/api/v1/m8/session-destroy/:id_session``` - Destroys a persistent session.

- GET ```/api/v1/m8/session-stats/:id_session``` - Details on persistent session.

- GET ```/api/v1/m8/session-activity``` - active online sessions.

- POST ```/api/v1/m8/session-run/:id_session``` - Run inside a persistent session.

## Show Time

The M8 runtime is very fast. instructions typically run in the microseconds range.
given proper advantages to built performant abstractions and apis.

![M8 PREVIEW V1](./M8_PREVIEW_V1.png)


## Runtimes

Three implementations are under development:
- Golang (default, ref implementation)
- Native Python/C++ *(this repo)*
- JVM Implementation

## Goals

- Video ACCELERATION
- Native CPU Inference Support (x86_64)
- Native Support for IMR (Intermediate Memory) & MGR (MemoryGroups)
- CUDA 12.1-12.4 SUPPORT
- 3D GPU ACCELERATION
- LOW LATENCY VOICE CIRCUIT
- NervePublish (BrainVM Powered)
- LLMCORE Inference2 {Inference, Classification, Tokenization}

## Modules

Important work is carried out in these modules:
- COMPILER
- VIRTUAL MACHINE
- INSTRUCTION SET
- CONCURRENCY
- LOCKING
- REGISTERS & MNEMONICS
- CORES & MODULE
- MEMORY SYSTEM
- BUS

## EDITIONS OR VERSIONS
- stable: barren golang runtime, light, optimal for customization
- pyvm: Includes a python vm inside the golang runtime
- cppvm: Implements a native c/c++ engine  *(this repo)*
- brainvm: Includes the whole brain env

## EXAMPLE

```cpp
#include <iostream>
#include <string>
#include <ctime>
#include "m8p.h"

int main(int argc, char ** argv) {
    m8p::M8System *m8 = m8p::M8P_Instance("m8-instance");
    std::string P1 = "clr <r1>   \n"
                    "store <r1> <I000R1>  \n"
                    "store <r2> <I000R2_VALUE_OF_2>  \n"
                    "store <r3> <I000R3_VALUE_OF_3>  \n"
                    "store <r4> <VALUE OF 4 IS DYNAMIC NOW alaksdalksdj l asldkjalkd ajlkd aksldjaksldjaskdj lkasdad asd>  \n"
                    ;

    std::pair<m8p::M8_Error, m8p::M8_Obj*> Ret = m8p::Run(m8, P1);

    // looks like an error ocurred on execution
    if (Ret.first.Type!=m8p::M8_Err_nil.Type) {
        std::cout << "ERROR: " << Ret.first.Details;
    } else {
        std::string sType=0;
        int32_t i32Value=0;
        float f32Value=0;
        std::string sValue = "";

        if (Ret.second!=nullptr) {
            sType = m8p::TypeStr(Ret.second->Type);
            if (Ret.second->Type==m8p::MP8_I32) {
                i32Value = Ret.second->I32;
            } else if (Ret.second->Type==m8p::MP8_F32) {
                f32Value = Ret.second->F32;
            } else {
                sValue = Ret.second->Value;
            }
        } 
    }

    // destroy m8 instance and release resource
    // ATTENTION: DO NOT USE m8 before calling M8P_INSTANCE again
    m8p::DestroyMP8(m8);
    m8 = nullptr;
}
```

