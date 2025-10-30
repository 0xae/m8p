// =========================================================================
// M8 MICROPROCESSOR C++ IMPLEMENTATION
// MODULES
//    - COMPILER
//    - VIRTUAL MACHINE
//    - INSTRUCTION SET
//    - CONCURRENCY
//    - LOCKING
//    - REGISTERS & MNEMONICS
//    - CORES & MODULE
//    - MEMORY SYSTEM
//    - BUS
// @date: 2024-09-07
// @date: 2025-10-28
// @author: Ayrton Gomes
// =========================================================================
//

// The M8 Processor is an execution engine to run the instructions
// and output the results into the registers

#ifndef M8_PLATFORM
#define M8_PLATFORM

#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <assert.h>
#include <stdexcept> 
#include <list>
#include <map>
#include <ctime>
#include <stdio.h>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>
#include <algorithm> 
#include <charconv> 
#include <cctype>
#include <locale>
#include <cmath>
#include <unistd.h>
#include <fstream>
#include <set>

#ifdef INCL_FAISS
#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFPQ.h>
#include <faiss/index_io.h>
#endif

#ifdef __AVX__
    #include <immintrin.h>
    #warning AVX support is AVAILABLE
    #if defined(__AVX512F__)
        #define AVX_V_SIZE 16   // 512/32 = 16 floats
        #define SIMD_SET_PS _mm512_set_ps
        #define SIMD_ADD_PS _mm512_add_ps
        #define SIMD_MUL_PS _mm512_mul_ps
        #define SIMD_SUB_PS _mm512_sub_ps
        #define SIMD_STORE_PS _mm512_storeu_ps
        #define SIMD_SETZERO_PS _mm512_setzero_ps
        #define SIMD_FMA_PS _mm512_fmadd_ps
        #define SIMD_LOAD_PS _mm512_loadu_ps
        #define SIMD_SQRT_PS _mm512_sqrt_ps
        #define simd_vec __m512

    #elif defined(__AVX2__) || defined(__AVX__)
        #define AVX_V_SIZE 8    // 256/32 = 8 floats
        #define SIMD_SET_PS _mm256_set_ps
        #define SIMD_ADD_PS _mm256_add_ps
        #define SIMD_SUB_PS _mm256_sub_ps
        #define SIMD_MUL_PS _mm256_mul_ps
        #define SIMD_STORE_PS _mm256_storeu_ps
        #define SIMD_SETZERO_PS _mm256_setzero_ps
        #define SIMD_LOAD_PS _mm256_loadu_ps
        #define SIMD_SQRT_PS _mm256_sqrt_ps
        #if defined(__FMA__)
            #define SIMD_FMA_PS _mm256_fmadd_ps
        #else
            #define SIMD_FMA_PS(a, b, c) _mm256_add_ps(_mm256_mul_ps(a, b), c)
        #endif
        // typedef __m256 simd_vec;
        #define simd_vec __m256
    #else
        #define AVX_V_SIZE 4    // 128/32 = 4 floats (SSE)
        #define SIMD_SET_PS _mm_set_ps
        #define SIMD_ADD_PS _mm_add_ps
        #define SIMD_MUL_PS _mm_mul_ps
        #define SIMD_SUB_PS _mm_sub_ps
        #define SIMD_SETZERO_PS _mm_setzero_ps
        #define SIMD_SQRT_PS _mm_sqrt_ps
        #define SIMD_FMA_PS(a, b, c) _mm_add_ps(_mm_mul_ps(a, b), c)
        #define SIMD_STORE_PS _mm_storeu_ps
        #define SIMD_LOAD_PS _mm_loadu_ps
        // typedef __m128 simd_vec;
        #define simd_vec __m128
    #endif
#else
  #warning No AVX support [matmul wont be available]
#endif

// #include "m8p-train.h"

namespace m8p {
    //using std::list;
    using std::map;
    using std::string;
    using std::vector;
    using std::set;
    using std::map;

    const int MP8_STRING = 1;
    const int MP8_OBJ = 2;
    const int MP8_INSTR = 3;
    const int MP8_TRUE = 4;
    const int MP8_FALSE = 5;
    const int MP8_NIL = 6;
    const int MP8_NUMBER = 7;
    const int MP8_INT = 8;
    const int MP8_I32 = 9;
    const int MP8_I64 = 10;
    const int MP8_F32 = 11;
    const int MP8_F64 = 12;

    const int MP8_DI32 = 13;
    const int MP8_DF32 = 14;

    const int MP8_ERR = 15;
    const int ERR_NIL = 16;
    const int RUN_ERR = 17; // runtime error

    // structured datatypes
    const int MP8_OLIST = 18;

    // typedef OPCALL;
    struct M8System;

    // NOTE: There are better ways to layout this struct
    // we leave it as an optimization for the future
    struct M8_Obj {
        int Type;
        std::string Value;
        int32_t I32;
        float F32;
        std::vector<int32_t> AR_I32;
        std::vector<float> AR_F32;
        std::vector<M8_Obj*> AR_OBJ;
    };

    // typedef std::pair<M8_Error,M8_Obj*> OPCALL(std::vector<std::string>);

    struct M8_Error {
        int Type;
        std::string Details;
    };

    M8_Error M8_Err_nil {ERR_NIL, ""};

    class VInstr {
    public:
        VInstr(){
        }

        virtual ~VInstr(){
        }

        virtual std::pair<M8_Error, M8_Obj*> Call(
            M8System* M8, 
            std::vector<std::string> params
        ) = 0;
    };

    struct M8System {
        std::string Name;
        map<string, M8_Obj*> Registers;
        map<string, M8_Obj*> LMemory;
        map<string, VInstr*> Virtual;
        std::vector<M8_Obj*> alloc;
        bool EnableStacktrace = false;
        M8_Obj *nilValue = nullptr;
        M8_Obj *true_ = nullptr;
        M8_Obj *emptyList_ = nullptr;
        M8_Obj *false_ = nullptr;
        M8_Obj *_0 = nullptr;
        M8_Obj *_1 = nullptr;
        M8_Obj *_2 = nullptr;
        M8_Obj *_3 = nullptr;
        M8_Obj *_4 = nullptr;
        M8_Obj *_5 = nullptr;
        M8_Obj *_6 = nullptr;
        M8_Obj *_7 = nullptr;
        M8_Obj *_8 = nullptr;
        M8_Obj *_9 = nullptr;
        M8_Obj *_10 = nullptr;
    };

    struct prompt_obj {
        string value;
        vector<int32_t> embed;
        vector<string> categories;
    };

    struct train_db {
        bool Valid = false;
        set<string> categories;
        vector<prompt_obj> prompts;
    };

    struct doc_def {
        int32_t count;
        vector<int32_t> tokens;
    };
    // struct train_memory {
    //     bool Valid=false;
    //     std::map<string, std::map<string, int32_t>> DistTable;
    //     std::map<string, int32_t> CatCounts;
    //     string name;
    //     string date;
    // };
    struct train_memory {
        bool Valid=false;
        std::map<string, std::map<string, int32_t>> DistTable;
        std::map<string, int32_t> CatCounts;
        std::map<string, int32_t> GCounts;
        set<string> groups;
        string name;
        string date;
    };

    // M8_Obj *errValue = nullptr;

    M8_Obj* m8_obj_list(M8System *M8, std::vector<M8_Obj*> AR_OBJ);
    M8_Obj* m8_obj(M8System *M8, int type, std::string value);
    M8_Obj* m8_obj(M8System *M8, int32_t I32);
    M8_Obj* m8_obj(M8System *M8, float F32);
    M8_Obj* m8_obj_dup(M8System *M8, M8_Obj *C);
    std::string to_string(M8_Obj* C);

    // std::string to_string(uint32_t);
    // std::string to_string(int32_t);
    // std::string to_string(int);
    // std::string to_string(float);
    // std::string to_string(double);

    void RegisterVirtual(M8System *M8, string name, VInstr* callable);
    bool is_error(M8System *M8, M8_Obj *V);
    bool is_nil(M8System *M8, M8_Obj *V);
    M8_Error errorf(std::string reason);
    void m8_obj_destroy(M8_Obj* mobj);
    M8_Obj* m8_error_obj(M8System *M8, std::string cause);
    std::string TypeStr(int Type);
    int ResolveStrType(std::string Type);

    M8System* M8P_Instance();
    M8System* M8P_Instance(std::string name);
    int InitMP8(M8System *m8);
    void DestroyMP8(M8System *m8);
    std::vector<std::string> __split(std::string s, std::string delimiter);

    std::pair<M8_Error, M8_Obj*> NO_OP(M8System* m8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> Store_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> Clear_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> Load_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> Ret_OP(M8System* M8, std::vector<std::string> params);
    // AVX SUPPORT
#ifdef __AVX__
    std::pair<M8_Error, M8_Obj*> MatMul_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> MatDotProd_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> MatNorm_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> MatCosim_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> MatL2Dist_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> MatMul3_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> Mat8_OP(std::string op, M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> MatN_OP(std::string op, M8System* M8, std::vector<std::string> params);
#endif
    // matrix api
    std::pair<M8_Error, M8_Obj*> Mat8Set_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> ALIGN8_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> ALIGN_OP(M8System* M8, std::vector<std::string> params);
    bool IsValid_DF32(M8System* M8, M8_Obj* R);
    bool IsValid_DF32_Dim (M8System* M8, M8_Obj* R, uint32_t dim);

    // XXXXXXXXXXXXX
    std::pair<M8_Error, M8_Obj*> I32Set_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> I32Add_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> Stall_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> UStall_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> I32Sub_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> I32Mul_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> F32Set_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> F32Add_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> F32Sub_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> F32Mul_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> AssertEQ_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> AssertEmpty_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> AssertNotEmpty_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> AssertContains_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> AssertNil_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> AssertError_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> PrintR_OP(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> Run(M8System* M8, std::string code_buf);

    void __ltrim(std::string &s);
    void __rtrim(std::string &s);
    void __trim(std::string &s);

    M8_Obj* m8_obj(M8System *M8, int type, std::string value) {
        M8_Obj *obj = new M8_Obj;
        if (obj!=nullptr) {        
            obj->Type = type;
            obj->Value = value;
            M8->alloc.push_back(obj);
        }
        return obj;
    }

    M8_Obj* m8_obj_list(M8System *M8, std::vector<M8_Obj*> AR_OBJ) {
        M8_Obj *obj = new M8_Obj;
        if (obj!=nullptr) {
            obj->Type = MP8_OLIST;
            obj->Value = "";
            obj->AR_OBJ = AR_OBJ;
            M8->alloc.push_back(obj);
        }
        return obj;        
    }

    void RegisterVirtual(M8System *M8, string name, VInstr* callable) {
        if (M8->Virtual[name]==nullptr) {
            M8->Virtual[name] = callable;
            // std::cout << "Virtual " << name << " is registered!\n";
        }
    }

    M8_Obj* m8_obj(M8System *M8, int32_t I32) {
        M8_Obj *obj = m8_obj(M8, MP8_I32, "");
        obj->I32 = I32;
        return obj;
    }
    M8_Obj* m8_obj(M8System *M8, float F32) {
        M8_Obj *obj = m8_obj(M8, MP8_F32, "");
        obj->F32 = F32;
        return obj;
    }

    M8_Obj* m8_obj_dup(M8System *M8, M8_Obj *C) {
        if (C==nullptr || C==M8->nilValue) {
            return M8->nilValue;
        }

        M8_Obj* obj=m8_obj(M8, C->Type, C->Value);
        obj->I32 = C->I32;
        obj->F32 = C->F32;
        obj->AR_F32 = C->AR_F32;
        obj->AR_OBJ = C->AR_OBJ;
        return obj;
    }

    bool is_error(M8System *M8, M8_Obj *V) {
        return V!=nullptr && V->Type!=M8->nilValue->Type && V->Type==MP8_ERR;
    }

    bool is_nil(M8System *M8, M8_Obj *V) {
        return V!=nullptr && V->Type==MP8_NIL;
    }

    M8_Error errorf(std::string reason) {
        M8_Error err;
        err.Type=RUN_ERR;
        err.Details=reason;
        return err;
    }

    void m8_obj_destroy(M8_Obj* mobj) {
        // m8_obj_destroy(mobj);
        if (mobj!=nullptr) {
            delete mobj;
        }
    }

    M8_Obj* m8_error_obj(M8System *M8, std::string cause) {
        return m8_obj(M8, MP8_ERR, cause);
    }

    // struct M8_Obj {
    //     int Type;
    //     std::string Value;
    //     int32_t I32;
    //     float F32;
    //     std::vector<int32_t> AR_I32;
    //     std::vector<float> AR_F32;
    //     std::vector<M8_Obj*> AR_OBJ;
    // };

    // std::string to_string(uint32_t val) {
    //     return std::to_string(val);
    // }

    // std::string to_string(size_t val) {
    //     return std::to_string(val);
    // }

    // std::string to_string(int32_t val) {
    //     return std::to_string(val);
    // }

    // std::string to_string(int val) {
    //     return std::to_string(val);
    // }

    // std::string to_string(float val) {
    //     return std::to_string(val);
    // }

    // std::string to_string(double val) {
    //     return std::to_string(val);
    // }

    std::string to_string(M8System *M8, M8_Obj* C) {
        if (C==nullptr || C==M8->nilValue) {
            return "nil";
        }

        int Type = C->Type;

        if (Type==MP8_FALSE) {
            return "false";

        } else if (Type==MP8_TRUE) {
            return "true";

        } else if (Type==MP8_STRING) {
            return C->Value;

        } else if (Type==MP8_I32) {
            std::stringstream ss; ss<<C->I32; return ss.str();

        } else if (Type==MP8_F32) {
            std::stringstream ss; ss<<C->F32; return ss.str();

        } else if (Type==MP8_DI32) {
            std::stringstream ss;
            ss << "[";
            for (std::vector<int32_t>::iterator i=C->AR_I32.begin(); i!=C->AR_I32.end() ; ++i) {
                ss << *i << ",";
            }
            ss << "]";
            return ss.str();

        } else if (Type==MP8_DF32) {
            std::stringstream ss;
            ss << "[";
            for (std::vector<float>::iterator i=C->AR_F32.begin(); i!=C->AR_F32.end() ; ++i) {
                ss << *i << ",";
            }
            ss << "]";
            return ss.str();

        } else {
            return "<unkn>";
        }
    }

    std::string TypeStr(int Type) {
        if (Type==MP8_FALSE) {
            return "<false>";

        } else if (Type==MP8_STRING) {
            return "<string>";

        } else if (Type==MP8_INSTR) {
            return "<instr>";

        } else if (Type==MP8_OLIST) {
            return "<dim:obj>";

        } else if (Type==MP8_I32) {
            return "<int32>";

        } else if (Type==MP8_DI32) {
            return "<dim:int32>";

        } else if (Type==MP8_DF32) {
            return "<dim:float32>";

        } else if (Type==MP8_I64) {
            return "<int64>";

        } else if (Type==MP8_F32) {
            return "<float32>";

        } else if (Type==MP8_F64) {
            return "<float64>";

        } else if (Type==MP8_TRUE) {
            return "<true>";

        } else if (Type==MP8_NIL) {
            return "<niltype>";

        } else if (Type==MP8_NUMBER) {
            return "<number>";
        } else {
            return "<unkn>";
        }
    }

    int ResolveStrType(std::string Type) {
        if (Type=="MP8_FALSE") {
            return MP8_FALSE;
        } else if (Type=="MP8_STRING") {
            return MP8_STRING;
        } else if (Type=="MP8_INSTR") {
            return MP8_INSTR;
        } else if (Type=="MP8_TRUE") {
            return MP8_TRUE;
        } else if (Type=="MP8_LIST" || Type=="MP8_OLIST") {
            return MP8_OLIST;
        } else if (Type=="MP8_NIL") {
            return MP8_NIL;
        } else if (Type=="MP8_NUMBER") {
            return MP8_NUMBER;
        } else {
            return -1;
        }
    }

    M8System* M8P_Instance() {
        M8System *m8 = new M8System();
        if (m8!=nullptr) {
            m8->Name = "M8";
            InitMP8(m8);
        }
        return m8;
    }

    M8System* M8P_Instance(string name) {
        M8System *m8 = new M8System();
        if (m8!=nullptr) {
            m8->Name = name;
            InitMP8(m8);
        }
        return m8;
    }

    int InitMP8(M8System *m8) {
        std::vector<M8_Obj*> EMPTY;
        M8_Obj *ELIST = m8_obj_list(m8, EMPTY);
        m8->emptyList_ = ELIST;
        m8->nilValue = m8_obj(m8, MP8_NIL, "<nil>");
        m8->true_ = m8_obj(m8, MP8_TRUE, "true");
        m8->false_ = m8_obj(m8, MP8_FALSE, "false");

        m8->_0 = m8_obj(m8, 0);
        m8->_1 = m8_obj(m8, 1);
        m8->_2 = m8_obj(m8, 2);
        m8->_3 = m8_obj(m8, 3);
        m8->_4 = m8_obj(m8, 4);
        m8->_5 = m8_obj(m8, 5);
        m8->_6 = m8_obj(m8, 6);
        m8->_7 = m8_obj(m8, 7);
        m8->_8 = m8_obj(m8, 8);
        m8->_9 = m8_obj(m8, 9);
        m8->_10 = m8_obj(m8, 10);
        return 0;
    }

    void DestroyMP8(M8System *m8) {
        if (m8!=nullptr) {
            // if (m8->nilValue!=nullptr) {
            //     m8_obj_destroy(m8->nilValue);
            //     std::cout << "Destroyed m8 nilValue mobjs;\n";
            // }
            int dc = 0;
            std::vector<M8_Obj*>::iterator it = m8->alloc.begin();
            for (; it!= m8->alloc.end(); ++it) {
                m8_obj_destroy(*it);
                dc += 1;
            }

            std::cout << "Destroyed " << dc << " mobjs;\n";
            delete m8;
        }

        std::cout << "=======\nM8 Instance Destroyed\n";
        std::cout << "[exit]\n";
    }

    std::vector<std::string> __split(std::string s, std::string delimiter) {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
            token = s.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back (token);
        }

        res.push_back (s.substr (pos_start));
        return res;
    }

    std::vector<std::string> __split_no_empty(std::string s, std::string delimiter) {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
            token = s.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            __trim(token);
            if (token.size()>0) {                
                res.push_back (token);
            }
        }

        res.push_back (s.substr (pos_start));
        return res;
    }

    void __ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    // trim from end (in place)
    void __rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    void __trim(std::string &s) {
        __rtrim(s);
        __ltrim(s);
    }

    int __abs(int32_t v) {
        return v>0? v : -v;
    }

    //
    // ALL API
    //

    //
    // TRAINING API
    //
    train_db create_train_db(string &filename) {
        train_db TRAIN;
        TRAIN.Valid=false;
        std::cout << "Reading: " 
                  << filename 
                  << "\n" 
                  << std::endl;

        std::ifstream file(filename.c_str());

        if (!file.is_open()) {
            fprintf(stderr, "Error: could not open file %s\n", filename.c_str());
            // check Va
            return TRAIN;
        }

        string Line;
        while (std::getline(file, Line)) {
            __trim(Line);
            if (Line=="") {
                continue;
            }

            if (Line.rfind("##",0) == 0) { 
                auto pos_1=Line.rfind("##",0);
                std::string buf = Line.substr(pos_1+2);
                __trim(buf);

                if (buf.find("{")!=string::npos) {
                    if (buf.find("}")==string::npos) {
                        std::cout << "Malformed category Instruction. Missing closing brace }\n"; 
                        continue;
                    }
                    
                    std::string catBuf = buf.substr(buf.find("{")+1, buf.find("}")-1);
                    auto RList = __split_no_empty(catBuf, ",");
                    prompt_obj PMT;
                    PMT.value = buf.substr(buf.find("}")+1);;

                    for (string &Token : RList) {
                        __trim(Token);
                        if (Token=="") continue;
                        PMT.categories.push_back(Token);
                        TRAIN.categories.insert(Token);
                    }
                    TRAIN.prompts.push_back(PMT);

                } else {
                    prompt_obj PMT;
                    PMT.value = buf;
                    TRAIN.prompts.push_back(PMT);
                }


            } else if (Line.rfind("[]",0) == 0) {
                // auto pos_1=Line.rfind("[]",0);
                // std::string buf = Line.substr(pos_1+2);
            } else {
                std::cout << "Invalid Instruction: " 
                          << "["
                          << Line
                          << "]\n"
                          << std::endl;
            }

        }

        TRAIN.Valid = true;
        return TRAIN;
    }

    std::set<string> make_tuples(std::vector<std::string> &WTokens) {
        size_t wtlen = WTokens.size();
        set<string> seq;

        if (wtlen==0) {
            return seq;
        } else if (wtlen == 1) {
            __trim(WTokens[0]);
            if (WTokens[0] != "") {                
                string Key=WTokens[0]+" <NOP>";
                string Key2="<NOP> "+ WTokens[0];
                seq.insert(Key);
                seq.insert(Key2);
            }

        } else if (wtlen==2) {
            string Key=WTokens[0]+" "+WTokens[1];
            string Key2=WTokens[1]+" "+WTokens[0];
            seq.insert(Key);
            seq.insert(Key2);

        } else if (wtlen==3) {
            string Key=WTokens[0]+" "+WTokens[1];
            string Key2=WTokens[1]+" "+WTokens[2];
            string Key3=WTokens[0]+" "+WTokens[2];
            seq.insert(Key);
            seq.insert(Key2);
            seq.insert(Key3);

        } else if (wtlen==4) {
            seq.insert(WTokens[0]+" "+WTokens[2]);
            seq.insert(WTokens[1]+" "+WTokens[2]);
            string Key=WTokens[0]+" "+WTokens[1];
            string Key2=WTokens[2]+" "+WTokens[3];
            seq.insert(Key);
            seq.insert(Key2);

        } else if (wtlen==5) {
            {
                string Key=WTokens[0]+" "+WTokens[1];
                string Key2=WTokens[1]+" "+WTokens[2];
                string Key3=WTokens[0]+" "+WTokens[2];
                seq.insert(Key);
                seq.insert(Key2);
                seq.insert(Key3);
            }
            string Key=WTokens[0]+" "+WTokens[1]+" "+WTokens[2];
            string Key2=WTokens[3]+" "+WTokens[4];
            seq.insert(Key);
            seq.insert(Key2);

        } else {
            {
                string Key=WTokens[0]+" "+WTokens[1];
                string Key2=WTokens[1]+" "+WTokens[2];
                string Key3=WTokens[0]+" "+WTokens[2];
                seq.insert(Key);
                seq.insert(Key2);
                seq.insert(Key3);
            }

            string Key=WTokens[0]+" "+WTokens[1]+" "+WTokens[2];
            string Key2=WTokens[1]+" "+WTokens[2]+" "+WTokens[3];
            string Key3=WTokens[2]+" "+WTokens[3]+" "+WTokens[4];
            seq.insert(Key);
            seq.insert(Key2);
            seq.insert(Key3);
        }

        return seq;
    }

    train_memory train_memory_words(train_db &TRAIN_DB) {
        // vector<doc_def> docList;
        train_memory Memory;
        // std::map<string, std::map<string, int32_t>> DistTable;
        std::map<string, int32_t> &GIndex = Memory.GCounts;
        bool hasGname = false;

        for (prompt_obj &PMT : TRAIN_DB.prompts) {
            string &strBuf = PMT.value;
            std::string ctx="Default";
            std::string gname="<NOP>";

            for (std::string &Cat : PMT.categories) {
                __trim(Cat);
                if (Cat.find("ctx=")!=string::npos && ctx=="Default") {
                    auto start=Cat.find("ctx=")+4;
                    ctx = Cat.substr(start);
                }

                if (Cat.find("gname=")!=string::npos && gname=="<NOP>") {
                    auto start=Cat.find("gname=")+6;
                    std::string copy = Cat.substr(start);
                    __trim(copy);
                    if (copy.size()>0) {
                        gname=copy;
                        hasGname=true;
                    }
                }

                if (ctx!="Default" && gname!="<NOP>") {
                    break;
                }
            }

            if (ctx=="Default" || ctx=="") {
                std::cout << "Warning: No context classification found on qa : "<< strBuf << " [skipping this line]\n";
                continue;
            }

            if (hasGname) {
                Memory.groups.insert(gname);
            }

            __trim(strBuf);
            vector<string> WTokens = __split_no_empty(strBuf, " ");
            size_t wtlen = WTokens.size();
            if (wtlen==0) {
                std::cout << "Warning: No words were detected in this list: "<< strBuf << " [skipping this line]\n";
                continue;
            }

            std::map<string, int32_t> stats;
            set<string> seq_tuples = make_tuples(WTokens);

            for (std::string TUPLE : seq_tuples) {
                if(stats.count(TUPLE)==0) {
                    stats[TUPLE]=0;
                }
                stats[TUPLE] += 1;
                if (hasGname) {
                    std::string gKey = TUPLE+"__##"+gname;
                    if (GIndex.count(gKey)==0) {                    
                        GIndex[gKey]=0;
                    }
                    GIndex[gKey] += 1;
                }
            }

            // debug_stats(stats);
            if (Memory.DistTable.count(ctx)==0) {
                Memory.DistTable[ctx] = stats;
                Memory.CatCounts[ctx] = 1;
            } else {
                Memory.CatCounts[ctx] += 1;
                std::map<string, int32_t> &CatStats = Memory.DistTable[ctx];
                for (std::pair<string,int32_t> P : stats) {
                    // std::cout << P.first << " = " << P.second << "\n";
                    if (CatStats.count(P.first)==0) {
                        CatStats[P.first] = P.second;
                    } else {
                        CatStats[P.first] += P.second;
                    }
                }
            }
        }

        // std::cout << std::endl;
        Memory.Valid = true;
        return Memory;
    }

    std::pair<set<string>,int32_t> sequence_message_call(std::string &question) {
        auto WTokens = __split_no_empty(question, " ");
        std::set<std::string> seq = make_tuples(WTokens);
        auto seq_len = seq.size();

        return std::make_pair(
            seq,
            seq_len
        );
        // if (wtlen==1) {
        //     // std::cout << "w words found on this line: "<< strBuf << " [skipping this line]\n";
        //     // string Key=WTokens[0];
        //     string Key=WTokens[0] + " " +"<NOP>";
        //     string Key2="<NOP> " + WTokens[0];
        //     seq.insert(Key);
        //     seq.insert(Key2);
        // } else if (wtlen==2) {
        //     string Key=WTokens[0]+" "+WTokens[1];
        //     string Key2=WTokens[1]+" "+WTokens[0];
        //     seq.insert(Key);
        //     seq.insert(Key2);
        // } else if (wtlen==3) {
        //     string Key=WTokens[0]+" "+WTokens[1];
        //     string Key2=WTokens[1]+" "+WTokens[2];
        //     string Key3=WTokens[0]+" "+WTokens[2];
        //     seq.insert(Key);
        //     seq.insert(Key2);
        //     seq.insert(Key3);
        // } else if (wtlen==4) {
        //     string Key=WTokens[0]+" "+WTokens[1];
        //     string Key2=WTokens[2]+" "+WTokens[3];
        //     string Key3=WTokens[1]+" "+WTokens[2];
        //     seq.insert(Key);
        //     seq.insert(Key2);
        //     seq.insert(Key3);
        // } else if (wtlen==5) {
        //     string Key=WTokens[0]+" "+WTokens[1]+" "+WTokens[2];
        //     string Key3=WTokens[0]+" "+WTokens[1];
        //     string Key5=WTokens[1]+" "+WTokens[2];
        //     string Key2=WTokens[3]+" "+WTokens[4];
        //     seq.insert(Key);
        //     seq.insert(Key2);
        //     seq.insert(Key3);
        //     seq.insert(Key5);
        // } else {
        //     string Key=WTokens[0]+" "+WTokens[1];
        //     string Key4=WTokens[0]+" "+WTokens[1]+" "+WTokens[2];
        //     string Key2=WTokens[1]+" "+WTokens[2]+" "+WTokens[3];
        //     string Key3=WTokens[2]+" "+WTokens[3]+" "+WTokens[4];

        //     seq.insert(Key);
        //     seq.insert(Key2);
        //     seq.insert(Key3);
        //     seq.insert(Key4);
        // }
    }

    map<string, int32_t> classify_qa(train_memory &MEM, std::string &question) {
        auto seq_message = sequence_message_call(question);
        map<string, int32_t> matches;

        if (seq_message.second==0) {
        } else {
            // std::cout << "Classify: " << question;
            auto DistTable = MEM.DistTable;
            int current_count = 0;

            for (string Seq : seq_message.first) {
                for (auto &Cat : DistTable) {
                    if (current_count>=1000) {
                        break;
                    }

                    string catName = Cat.first; // string (category name)
                    std::map<string, int32_t> &CatStats = Cat.second; // stats
                    if (CatStats.count(Seq)>0) {
                        // std::cout << "[X] Match: [count="  << CatStats[Seq] << "]\n";
                        if (matches.count(catName)==0) {
                            matches[catName]=0;
                        }
                        matches[catName] += CatStats[Seq]; // should by count
                        current_count += 1;
                    } else {
                        // std::cout << "SEq not found on catstats not found: " <<  Seq << "\n";
                    }
                }
            }
        }

        return matches;
    }

    std::map<std::string, int32_t> classify_qa_until(train_memory &MEM, std::string question) {
        map<string, int32_t> matches;
        int minSize = 5;
        int iterations = 50;

        do {
            if (iterations<=0) {
                break;
            } else if (iterations!=50) {
                auto fspace = question.find(" ");
                if (fspace!=string::npos) {
                    question = question.substr(fspace+1); // skip first word
                } else {
                    break;
                }
            }

            matches = classify_qa(MEM, question);
            iterations -= 1;
        } while (matches.size()==0);

        return matches;
    }

    //
    // MULTIVARIATE (Category) Classifier
    // Brings Categories & Sub Tags (groupname)
    // minSim: percentage limit to be considered on the results
    //
    // xxxxxxxxxxxxxxx
    // classify_qa_multiv(TRAINDB, MEM, prompt, 0.8)
    // xxxxxxxxxxxxxxx
    //

    struct MatchMV {
        int32_t cat_match_count = -1;
        std::map<std::string, int32_t> variates;
        // => groupname => 10 counts
        // => supplier_details => 10 counts
        // => customer_details => 10 counts
    };

    void debug_Matches(train_memory &MEM,  map<string, MatchMV> &Matches, std::string &question, float minSim) {
        std::cout << "==========================\n";
        std::cout << "CLASSIFY MULTI VARIATE\n";
        std::cout << "Candidate:  "  << question << "\n";

        auto DistTable = MEM.DistTable;
        std::map<string, int32_t> &GIndex = MEM.GCounts;

        if (Matches.size()==0) {
            std::cout << "Sorry no matches found\n";
            return;
        }

        for (auto Iter : Matches) {
            std::string catName = Iter.first;
            MatchMV &matchObj = Iter.second;
            int32_t rowCount=-1;
            if (MEM.CatCounts.count(catName)>0) {
                rowCount = MEM.CatCounts[catName];
            }

            std::cout << "[X] " 
                << " Match: [mcount="  << matchObj.cat_match_count << "] => "
                << "{" 
                << catName 
                << "[RowCount=" << rowCount
                << ", Variates=";


            for (auto VIter : matchObj.variates) {
                auto Gname = VIter.first;
                std::cout << Gname << ":" << VIter.second << ", ";
            }

            std::cout << "]\n";
        }

        std::cout << "==========================\n";
    }

    std::map<std::string, MatchMV> classify_qa_multiv(train_memory &MEM, std::string &question, float minSim) {
        map<string, MatchMV> Matches;
        std::map<string, int32_t> &GIndex = MEM.GCounts;

        float minCount = 1.0;
        auto seq_message = sequence_message_call(question);

        if (seq_message.second==0) {
        } else {
            auto DistTable = MEM.DistTable;

            for (string Seq : seq_message.first) {
                for (auto &Cat : DistTable) {
                    string catName = Cat.first; // string (category name)
                    std::map<string, int32_t> &CatStats = Cat.second; // stats
                    if (CatStats.count(Seq)>0) {
                        if (Matches.count(catName)==0) {
                            Matches[catName].cat_match_count = 0;
                        }

                        for (auto Gname : MEM.groups) {
                            __trim(Gname);
                            if (Gname=="" || Gname=="<NOP>") {
                                continue;
                            }

                            std::string key_try = Seq+"__##"+Gname;
                            if (GIndex.count(key_try)) {
                                if (Matches[catName].variates.count(Gname)==0) {
                                    Matches[catName].variates[Gname] = 0;
                                }
                                Matches[catName].variates[Gname] += 1;
                                // Matches[catName].variates[Gname] += GIndex[key_try];
                            }
                        }

                        // std::cout << "]}\n";
                        Matches[catName].cat_match_count += CatStats[Seq]; // should by count
                    }
                }
            }
        }

        debug_Matches(MEM, Matches, question, minSim);
        return Matches;
    }

    std::map<std::string, MatchMV> classify_qa_multiv_until(train_memory &MEM, std::string question, float minSim) {
        std::map<std::string, MatchMV> matches;
        int minSize = 5;
        int iterations = 50;

        do {
            if (iterations<=0) {
                break;
            } else if (iterations!=50) {
                auto fspace = question.find(" ");
                if (fspace!=string::npos) {
                    std::string pop = question.substr(0, fspace);
                    std::cout << "POP: " << pop << "\n";
                    question = question.substr(fspace+1); // skip first word
                } else {
                    break;
                }
            }

            matches = classify_qa_multiv(MEM, question, minSim);
            iterations -= 1;
        } while (matches.size()==0);

        return matches;
    }

    void printList(std::vector<string> &List) {
        std::cout << "[";
        for (string &Ref : List) {
            std::cout << Ref << ",";
        }
        std::cout << "]";
    }

    std::map<std::string, std::string> parseOptions(int start, std::vector<std::string> &params) {
        std::map<std::string, std::string> options;
        for (uint32_t i=start; i<params.size(); ++i) {
            std::string opt = params.at(i);
            __trim(opt);
            if (opt.find("=") != std::string::npos) {
                size_t pos_t = opt.find("=");
                std::string Key = opt.substr(0, pos_t);
                std::string Value = opt.substr(pos_t+1);
                options[Key]=Value;
            }
        }
        return options;
    }

    void debug_train_db(train_db &TRAIN_DB, bool extended, std::string &Name) {
        if (!TRAIN_DB.Valid) {
            std::cout << "INVALID TRAIN DB\n";
            return;
        }

        std::cout << "=================================\n";
        std::cout << " TRAINING DATABASE [" << Name << "] \n";
        std::cout << "=================================\n";
        std::cout << "# of prompts = " << TRAIN_DB.prompts.size() << "\n";
        std::cout << "# of categories = " << TRAIN_DB.categories.size() << "\n";
        std::map<std::string, int32_t> CatCounts;
        std::cout << "==============[CATEGORIES]============\n";
        for (prompt_obj &PMT : TRAIN_DB.prompts) {
            // std::cout << "P: %" << PMT.value << "%, C:"; printList(PMT.categories);
            // std::cout << "\n";
            for (std::string &catName : PMT.categories) {
                if (CatCounts.count(catName)==0) {
                    CatCounts[catName]=0;
                }
                CatCounts[catName] += 1;
            }
        }

        for (auto &Iter : CatCounts) {
            std::cout << "{" << Iter.first << "}, Count:" << Iter.second << "\n";
        }

        if (extended) {
            std::cout << "==============[PROMPTS]============\n";
            for (prompt_obj &PMT : TRAIN_DB.prompts) {
                std::cout << "P: %" << PMT.value << "%, C:"; printList(PMT.categories);
                std::cout << "\n";
            }
        }

        std::cout << "=================================\n";
    }

    void debug_mem_db(train_memory &MEM, bool extended, std::string &Name) {
        if (!MEM.Valid) {
            std::cout << ">>>>>>>>> INVALID MEMORY DB <<<<<<<<\n";
            return;
        }

        auto DistTable = MEM.DistTable;
        std::cout << "=================================\n";
        std::cout << " MEMORY DATABASE [" << Name << "] \n";
        std::cout << "=================================\n";

        for (auto &Cat : DistTable) {
            string catName = Cat.first; // string (category name)
            int32_t catCounts = -1;
            std::map<string, int32_t> &CatStats = Cat.second; // stats
            std::cout << "Category: "  << catName;
            if (MEM.CatCounts.count(catName)>0) {
                std::cout << "[CategoryCounts="  
                    << MEM.CatCounts[catName]
                    << ", RowCount="
                    << CatStats.size()
                    << "]";
                catCounts = MEM.CatCounts[catName];
            }

            std::cout << "\n";

            if (extended) {        
                int32_t i=0;
                for (auto P : CatStats) {
                    float percent = -1;
                    if (P.second>0 && catCounts>0) {
                        percent = P.second/catCounts;
                    }
                    std::cout <<"    " 
                            << i  
                            << " ["
                            << "counts=" <<  P.second  << ", "
                            << "%=" <<  P.second << "/" << catCounts
                        << "] "
                        << "'" << P.first << "'"
                        ;
                    std::cout << "\n";
                    i++;
                }
                std::cout << "==========================\n";
            }
        }

        std::cout << "=================================\n";
    }

    //
    // END ALL API
    //


    //
    // INSTRUCTION SET IMPLEMENTATION
    //
    std::pair<M8_Error, M8_Obj*> NO_OP(M8System* m8, std::vector<std::string> params){
        std::cout << "NO_OP\n";
        return std::make_pair(
            M8_Err_nil,
            m8->nilValue
        );
    }

    std::pair<M8_Error, M8_Obj*> Store_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("store requires two parameters"),
                M8->nilValue
            );
        }

        string rsource = params.at(1);// dont forget 0 is for the op_code
        string value = params.at(2);
        if (psize>2) {
            // read the rest of the line into value
            for (uint32_t i=3; i<params.size(); ++i) {
                string v = params.at(i);
                value = value + " " + v;
            }
        }

        size_t c_start = value.find('#');
        if (c_start!=std::string::npos) {
            value = value.substr(0, c_start);
        }

        __trim(rsource);

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        { // AI-GENERATED
            std::string interpolated;
            interpolated.reserve(value.size());
            for (size_t i = 0; i < value.size();) {
                if (value[i] == '<') {
                    size_t end = value.find('>', i + 1);
                    if (end != std::string::npos) {
                        std::string varname = value.substr(i + 1, end - i - 1);
                        auto it = REG.find("<"+varname+">");
                        // std::cout << "==> LOOKING [" << varname << "] ";
                        if (it != REG.end() && it->second && !m8p::is_nil(M8, it->second)) {
                            // std::cout << "FOUND VAR\n";
                            interpolated += to_string(M8, it->second);
                        } else {
                            // std::cout << "NOT FOUND\n";
                            // if not found, keep literal form
                            interpolated += "<" + varname + ">";
                        }
                        i = end + 1;
                        continue;
                    }
                }

                interpolated.push_back(value[i]);
                ++i;
            }
            value = interpolated;
        }

        if (value=="true") {
            REG[rsource] = M8->true_;

        } else if (value=="false") {
            // REG[rsource] = m8_obj(M8, MP8_FALSE, "");
            REG[rsource] = M8->false_;

        } else {
            m8p::M8_Obj *R = REG[rsource];
            if (R!=nullptr && !m8p::is_nil(M8, R) && R->Type==m8p::MP8_STRING) {
                R->Value = value;
            } else {
                REG[rsource] = m8_obj(M8, MP8_STRING, value);
            }
        }

        // std::cout << "STORE '" << value << "' INTO [" << rsource << "] \n";
        return std::make_pair(
            M8_Err_nil,
            REG[rsource]
        );
    }

    std::pair<M8_Error, M8_Obj*> Clear_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize!=1) {
            return std::make_pair(
                errorf("clr requires 1 parameter"),
                M8->nilValue
            );
        }
        string rsource = params.at(1);// dont forget 0 is for the op_code
        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        REG[rsource] = M8->nilValue;
        return std::make_pair(
            M8_Err_nil,
            M8->nilValue
        );
    }

    //
    // TODO: DOES STORE OTHER TYPES OR ONLY STRINGS
    //
    std::pair<M8_Error, M8_Obj*> Load_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("load requires two parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rsource = params.at(1);// dont forget 0 is for the op_code
        string rdest = params.at(2);

        if (REG.count(rsource)) {
            M8_Obj *R = REG[rsource]; 
            if (R==nullptr){
                return std::make_pair(
                    errorf("NULL_REGISTER["+rsource+"]"),
                    M8->nilValue
                );
            }
            if (is_nil(M8, R)){
                return std::make_pair(
                    errorf("NIL_REGISTER["+rsource+"]"),
                    M8->nilValue
                );
            }

            // REG[rdest] = m8_obj(M8, R->Type, R->Value);
            REG[rdest] = m8_obj_dup(M8, R);
            return std::make_pair(
                M8_Err_nil,
                REG[rdest]
            );

        } else {
            // std::cout << "LOAD '" << rsource << "' INTO [" << rdest << "] \n";
            return std::make_pair(
                errorf("REGISTER_NOT_FOUND["+rsource+"]"),
                M8->nilValue
            );
        }
    }

    std::pair<M8_Error, M8_Obj*> Ret_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        std::map<std::string, M8_Obj*> &REG = M8->Registers;

        if (psize>0) {
            std::vector<M8_Obj*> list;
            
            for (uint32_t i=1; i<params.size(); ++i) {
                string rsource = params.at(i);
                __trim(rsource);

                if (rsource=="") {
                    continue;
                }

                if (REG.count(rsource)) {
                    M8_Obj *R = REG[rsource]; 
                    if (R==nullptr){
                        return std::make_pair(
                            errorf("NULL_REGISTER["+rsource+"]"),
                            M8->nilValue
                        );
                    }
                    list.push_back(R);
                } else {
                    return std::make_pair(M8_Err_nil, M8->nilValue);
                }
            }

            M8_Obj *R;
            if (list.size()==1) {
                R = list.at(0);
            } else if (list.size()>1) {
                R = m8p::m8_obj_list(M8, list); // copies the list
            }

            return std::make_pair(M8_Err_nil, R);
        }

        return std::make_pair(
            M8_Err_nil,
            M8->nilValue
        );
    }

    //
    // I32
    //
    std::pair<M8_Error, M8_Obj*> I32Set_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("i32set requires two parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rdest = params.at(1);// dont forget 0 is for the op_code
        string Value = params.at(2);
        int32_t number;

        if (Value.rfind("<", 0)==0){ // seems to be a register lets look it up
            if (REG.count(Value)>0) {
                M8_Obj *R = REG[Value];
                if (R==nullptr || is_nil(M8,R) || R->Type!=MP8_I32) {
                    return std::make_pair(
                        errorf("EXPECTING_INT32_REGISTER["+Value+"]"),
                        M8->nilValue
                    );
                }
                number = R->I32;
            } else {
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );   
            }

        } else {
            auto [ptr, ec] = std::from_chars(Value.data(), Value.data()+Value.size(), number);
            if(ec == std::errc()){
            } else {
                return std::make_pair(
                    errorf("EXPECTING_INT32["+Value+"]"),
                    M8->nilValue
                );
            }
        }


        REG[rdest] = m8_obj(M8, number);
        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    std::pair<M8_Error, M8_Obj*> I32Add_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("i32add requires two parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rdest = params.at(1);// dont forget 0 is for the op_code
        string Value = params.at(2);
        int32_t number;

        if (Value.rfind("<", 0)==0){ // seems to be a register lets look it up
            if (REG.count(Value)>0) {
                M8_Obj *R = REG[Value];
                if (R==nullptr || is_nil(M8,R) || R->Type!=MP8_I32) {
                    return std::make_pair(
                        errorf("EXPECTING_INT32_REGISTER["+Value+"]"),
                        M8->nilValue
                    );
                }
                number = R->I32;
            } else {
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );   
            }

        } else {
            auto [ptr, ec] = std::from_chars(Value.data(), Value.data()+Value.size(), number);
            if(ec == std::errc()){
            } else {
                return std::make_pair(
                    errorf("EXPECTING_INT32["+Value+"]"),
                    M8->nilValue
                );
            }
        }

        // auto [ptr, ec] = std::from_chars(Value.data(), Value.data()+Value.size(), number);
        // if(ec == std::errc{}){
        // } else {
        //     return std::make_pair(
        //         errorf("EXPECTING_INT32["+Value+"]"),
        //         M8->nilValue
        //     );
        // }

        if (REG.count(rdest)) {
            M8_Obj *R = REG[rdest];
            if (R==nullptr){
                return std::make_pair(
                    errorf("NULL_REGISTER["+Value+"]"),
                    M8->nilValue
                );
            }
            if (is_nil(M8, R)){
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );
            }

            if (R->Type!=MP8_I32) {
                return std::make_pair(
                    errorf("EXPECTING_INT32_REGISTER["+rdest+"]"),
                    M8->nilValue
                );
            }

            int32_t total = R->I32 + number;
            R->I32 = total;
            return std::make_pair(M8_Err_nil, R);

        } else {
            REG[rdest] = m8_obj(M8, number);
        }

        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    std::pair<M8_Error, M8_Obj*> I32Sub_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("i32sub requires two parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rdest = params.at(1);// dont forget 0 is for the op_code
        string Value = params.at(2);
        int32_t number;
        if (Value.rfind("<", 0)==0){ // seems to be a register lets look it up
            if (REG.count(Value)>0) {
                M8_Obj *R = REG[Value];
                if (R==nullptr || is_nil(M8,R) || R->Type!=MP8_I32) {
                    return std::make_pair(
                        errorf("EXPECTING_INT32_REGISTER["+Value+"]"),
                        M8->nilValue
                    );
                }
                number = R->I32;
            } else {
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );   
            }

        } else {
            auto [ptr, ec] = std::from_chars(Value.data(), Value.data()+Value.size(), number);
            if(ec == std::errc()){
            } else {
                return std::make_pair(
                    errorf("EXPECTING_INT32["+Value+"]"),
                    M8->nilValue
                );
            }
        }
        // auto [ptr, ec] = std::from_chars(Value.data(), Value.data()+Value.size(), number);
        // if(ec == std::errc{}){
        // } else {
        //     return std::make_pair(
        //         errorf("EXPECTING_INT32["+Value+"]"),
        //         M8->nilValue
        //     );
        // }

        if (REG.count(rdest)) {
            M8_Obj *R = REG[rdest];
            if (R==nullptr){
                return std::make_pair(
                    errorf("NULL_REGISTER["+Value+"]"),
                    M8->nilValue
                );
            }
            if (is_nil(M8, R)){
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );
            }

            if (R->Type!=MP8_I32) {
                return std::make_pair(
                    errorf("EXPECTING_INT32_REGISTER["+rdest+"]"),
                    M8->nilValue
                );
            }

            int32_t total = R->I32-number;
            R->I32 = total;
            return std::make_pair(M8_Err_nil, R);
        } else {
            return std::make_pair(
                errorf("REGISTER_NOT_FOUND["+rdest+"]"),
                M8->nilValue
            );
        }
    }

    std::pair<M8_Error, M8_Obj*> I32Mul_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("i32mul requires two parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rdest = params.at(1);// dont forget 0 is for the op_code
        string Value = params.at(2);
        int32_t number;

        if (Value.rfind("<", 0)==0){ // seems to be a register lets look it up
            if (REG.count(Value)>0) {
                M8_Obj *R = REG[Value];
                if (R==nullptr || is_nil(M8,R) || R->Type!=MP8_I32) {
                    return std::make_pair(
                        errorf("EXPECTING_INT32_REGISTER["+Value+"]"),
                        M8->nilValue
                    );
                }
                number = R->I32;
            } else {
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );   
            }

        } else {
            auto [ptr, ec] = std::from_chars(Value.data(), Value.data()+Value.size(), number);
            if(ec == std::errc()){
            } else {
                return std::make_pair(
                    errorf("EXPECTING_INT32["+Value+"]"),
                    M8->nilValue
                );
            }
        }

        // auto [ptr, ec] = std::from_chars(Value.data(), Value.data()+Value.size(), number);
        // if(ec == std::errc{}){
        // } else {
        //     return std::make_pair(
        //         errorf("EXPECTING_INT32["+Value+"]"),
        //         M8->nilValue
        //     );
        // }

        if (REG.count(rdest)) {
            M8_Obj *R = REG[rdest];
            if (R==nullptr){
                return std::make_pair(
                    errorf("NULL_REGISTER["+Value+"]"),
                    M8->nilValue
                );
            }
            if (is_nil(M8, R)){
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );
            }

            if (R->Type!=MP8_I32) {
                return std::make_pair(
                    errorf("EXPECTING_INT32_REGISTER["+rdest+"]"),
                    M8->nilValue
                );
            }

            int32_t total = R->I32 * number;
            R->I32 = total;
            return std::make_pair(M8_Err_nil, R);

        } else {
            return std::make_pair(
                errorf("REGISTER_NOT_FOUND["+rdest+"]"),
                M8->nilValue
            );
        }
    }


    std::pair<M8_Error, M8_Obj*> Stall_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<1) {
            return std::make_pair(
                errorf("stall requires 1 parameter"),
                M8->nilValue
            );
        }

        string Value = params.at(1);
        uint32_t number;

        auto [ptr, ec] = std::from_chars(
            Value.data(), 
            Value.data()+Value.size(), 
            number
        );
        if(ec == std::errc{}){
        } else {
            return std::make_pair(
                errorf("EXPECTING_INT32["+Value+"]"),
                M8->nilValue
            );
        }

        std::cout << "sleep(" 
            << number 
            << ")" 
            << std::endl;
        sleep(number);
        return std::make_pair(M8_Err_nil, M8->nilValue);
    }

    std::pair<M8_Error, M8_Obj*> UStall_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<1) {
            return std::make_pair(
                errorf("stall requires 1 parameter"),
                M8->nilValue
            );
        }

        string Value = params.at(1);
        uint32_t number;

        auto [ptr, ec] = std::from_chars(
            Value.data(), 
            Value.data()+Value.size(), 
            number
        );
        if(ec == std::errc{}){
        } else {
            return std::make_pair(
                errorf("EXPECTING_INT32["+Value+"]"),
                M8->nilValue
            );
        }

        std::cout << "usleep(" 
            << number 
            << ")" 
            << std::endl;
        usleep(number);
        return std::make_pair(M8_Err_nil, M8->nilValue);
    }

    //
    // F32
    //

    std::pair<M8_Error, M8_Obj*> F32Set_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("f32set requires two parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rdest = params.at(1);// dont forget 0 is for the op_code
        string Value = params.at(2);
        float number=0;
        if (Value.rfind("<", 0)==0){ // seems to be a register lets look it up
            if (REG.count(Value)>0) {
                M8_Obj *R = REG[Value];
                if (R==nullptr || is_nil(M8,R) || R->Type!=MP8_F32) {
                    return std::make_pair(
                        errorf("EXPECTING_FLOAT32_REGISTER["+Value+"]"),
                        M8->nilValue
                    );
                }
                number = R->F32;
            } else {
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );   
            }

        } else {
            try {number=std::stof(Value);}
            catch (const std::invalid_argument& ia) {
                return std::make_pair(
                    errorf("EXPECTING_FLOAT32["+Value+"]"),
                    M8->nilValue
                );
            }            
        }

        // try {number=std::stof(Value);}
        // catch (const std::invalid_argument& ia) {
        //     return std::make_pair(
        //         errorf("EXPECTING_FLOAT32["+Value+"]"),
        //         M8->nilValue
        //     );
        // }

        // auto [ptr, ec] = std::from_chars(Value.data(), Value.data()+Value.size(), number);
        // if(ec == std::errc()){
        // } else {
        //     return std::make_pair(
        //         errorf("EXPECTING_FLOAT32["+Value+"]"),
        //         M8->nilValue
        //     );
        // }

        REG[rdest] = m8_obj(M8, number);
        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    std::pair<M8_Error, M8_Obj*> F32Add_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("f32add requires two parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rdest = params.at(1);// dont forget 0 is for the op_code
        string Value = params.at(2);
        float number=0;
        // float number;

        if (Value.rfind("<", 0)==0){ // seems to be a register lets look it up
            if (REG.count(Value)>0) {
                M8_Obj *R = REG[Value];
                if (R==nullptr || is_nil(M8,R) || R->Type!=MP8_F32) {
                    return std::make_pair(
                        errorf("EXPECTING_FLOAT32_REGISTER["+Value+"]"),
                        M8->nilValue
                    );
                }
                number = R->F32;
            } else {
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );   
            }

        } else {
            try {number=std::stof(Value);}
            catch (const std::invalid_argument& ia) {
                return std::make_pair(
                    errorf("EXPECTING_FLOAT32["+Value+"]"),
                    M8->nilValue
                );
            }            
        }

        // try {number=std::stof(Value);}
        // catch (const std::invalid_argument& ia) {
        //     return std::make_pair(
        //         errorf("EXPECTING_FLOAT32["+Value+"]"),
        //         M8->nilValue
        //     );
        // }
        // auto [ptr, ec] = std::from_chars(Value.data(), Value.data()+Value.size(), number);
        // if(ec == std::errc{}){
        // } else {
        //     return std::make_pair(
        //         errorf("EXPECTING_FLOAT32["+Value+"]"),
        //         M8->nilValue
        //     );
        // }

        if (REG.count(rdest)) {
            M8_Obj *R = REG[rdest];
            if (R==nullptr){
                return std::make_pair(
                    errorf("NULL_REGISTER["+Value+"]"),
                    M8->nilValue
                );
            }
            if (is_nil(M8, R)){
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );
            }

            if (R->Type!=MP8_F32) {
                return std::make_pair(
                    errorf("EXPECTING_FLOAT32_REGISTER["+rdest+"]"),
                    M8->nilValue
                );
            }

            float total = R->F32 + number;
            R->F32 = total;
            return std::make_pair(M8_Err_nil, R);

        } else {
            REG[rdest] = m8_obj(M8, number);
        }

        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    std::pair<M8_Error, M8_Obj*> F32Sub_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("f32sub requires two parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rdest = params.at(1);// dont forget 0 is for the op_code
        string Value = params.at(2);
        float number=0;
        __trim(Value);

        if (Value.rfind("<", 0)==0){ // seems to be a register lets look it up
            if (REG.count(Value)>0) {
                M8_Obj *R = REG[Value];
                if (R==nullptr || is_nil(M8,R) || R->Type!=MP8_F32) {
                    return std::make_pair(
                        errorf("EXPECTING_FLOAT32_REGISTER["+Value+"]"),
                        M8->nilValue
                    );
                }
                number = R->F32;
            } else {
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );   
            }

        } else {
            try {number=std::stof(Value);}
            catch (const std::invalid_argument& ia) {
                return std::make_pair(
                    errorf("EXPECTING_FLOAT32["+Value+"]"),
                    M8->nilValue
                );
            }            
        }

        if (REG.count(rdest)) {
            M8_Obj *R = REG[rdest];
            if (is_nil(M8, R)){
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );
            }

            if (R->Type!=MP8_F32) {
                return std::make_pair(
                    errorf("EXPECTING_FLOAT32_REGISTER["+rdest+"]"),
                    M8->nilValue
                );
            }

            float total = R->F32 - number;
            R->F32 = total;
            return std::make_pair(M8_Err_nil, R);

        } else {
            return std::make_pair(
                errorf("REGISTER_NOT_FOUND["+rdest+"]"),
                M8->nilValue
            );
        }

        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    std::pair<M8_Error, M8_Obj*> ALIGN8_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<1) {
            return std::make_pair(
                errorf("align8 requires (1) parameter"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string m1 = params.at(1);// dont forget 0 is for the op_code
        auto MR1 = REG[m1];

        if (!IsValid_DF32(M8, MR1)) {
            return std::make_pair(
                errorf("EXPECTING_FLOAT32_REGISTER["+m1+"]"),
                M8->nilValue
            );
        }

        // MR1->AR_I32.resize(8);
        // MR1->AR_F32.resize(8);
        std::vector<float> tokens;
        // std::vector<int32_t> tokens_i32;
        uint32_t count_items = 0;
        for (std::vector<float>::iterator i=MR1->AR_F32.begin(); 
                i!=MR1->AR_F32.end() && count_items<8; 
                ++i) {
            tokens.push_back(*i);
            // tokens_i32.push_back((int32_t)*i);
            count_items += 1;
        }

        if (count_items<8) {
            // tokens.size=5, i=8-5, i=3
            // 0,1,2,3,4       
            for (uint32_t i=count_items; i<8; ++i) {
                tokens.push_back(0);
                // tokens_i32.push_back(0);
            }
        }

        MR1->AR_F32 = tokens;

        return std::make_pair(
            M8_Err_nil,
            M8->nilValue
        );
    }

    std::pair<M8_Error, M8_Obj*> ALIGN_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("align requires (2) parameters"),
                M8->nilValue
            );
        }
        // matn <r1> 1 2 3 4 5 6 7 8 9 0
        // align <r1> 4 # <r1> is dim:4 matrix now congratulations
        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string m1 = params.at(1);// dont forget 0 is for the op_code
        string sizeVal = params.at(2);
        int32_t number=0;

        try {
            number=std::stof(sizeVal);
            if (number<=0 || number>2000) {
                return std::make_pair(
                    errorf("ALIGN SIZE MUST BE BETWEEN (0 and 2000)"),
                    M8->nilValue
                );
            }

        } catch (const std::invalid_argument& ia) {
            return std::make_pair(
                errorf("EXPECTING_INT32["+sizeVal+"]"),
                M8->nilValue
            );
        }

        int32_t align_size = number;
        auto MR1 = REG[m1];

        if (!IsValid_DF32(M8, MR1)) {
            return std::make_pair(
                errorf("EXPECTING_DF32_REGISTER["+m1+"]"),
                M8->nilValue
            );
        }

        if (MR1->AR_F32.size()==0) {
            return std::make_pair(
                errorf("EMPTY_DF32_REGISTER["+m1+"]"),
                M8->nilValue
            );            
        }

        if (MR1->AR_F32.size()==align_size) {
            return std::make_pair(
                M8_Err_nil,
                MR1
            );
        }

        // MR1->AR_I32.clear();
        // MR1->AR_F32.clear();

        std::vector<float> tokens;
        uint32_t count_items = 0;
        for (std::vector<float>::iterator i=MR1->AR_F32.begin(); 
                i!=MR1->AR_F32.end() && count_items<align_size; 
                ++i) {
            tokens.push_back(*i);
            count_items += 1;
        }

        if (tokens.size()<align_size) {
            int start = align_size-tokens.size();
            for (int i=tokens.size(); i<align_size; ++i) {
                tokens.push_back(0);
            }
        }

        MR1->AR_F32 = tokens;
        return std::make_pair(
            M8_Err_nil,
            MR1
        );
    }

    bool IsValid_DF32(M8System* M8, M8_Obj* R) {
        if (R==nullptr){
            return false;
        }
        if (is_nil(M8, R)){
            return false;
        }
        if (R->Type!=MP8_DF32) {
            return false;
        }
        return true;
    }

    bool IsValid_DF32_Dim(M8System* M8, M8_Obj* R, uint32_t dim) {
        if (R==nullptr){
            return false;
        }
        if (is_nil(M8, R)){
            return false;
        }
        if (R->Type!=MP8_DF32) {
            return false;
        }
        if (R->AR_F32.size()!=dim){
            return false;
        }
        return true;
    }

#ifdef __AVX__
    // std::pair<M8_Error, M8_Obj*> matmul(M8System* M8, std::vector<std::string> params);
    std::pair<M8_Error, M8_Obj*> Mat8_OP(std::string op, M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<3) {
            return std::make_pair(
                errorf("matop requires (3) three parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string m1 = params.at(1);// dont forget 0 is for the op_code
        string m2 = params.at(2);
        string rdest = params.at(3);

        auto MR1 = REG[m1];
        auto MR2 = REG[m2];

        if (!IsValid_DF32_Dim(M8, MR1, AVX_V_SIZE)) {
            return std::make_pair(
                errorf("EXPECTING_DIM_FLOAT32_REGISTER["+m1+","+ std::to_string(AVX_V_SIZE) + "]"),
                M8->nilValue
            );
        }
        if (!IsValid_DF32_Dim(M8, MR2, AVX_V_SIZE)) {
            return std::make_pair(
                    errorf("EXPECTING_DIM_FLOAT32_REGISTER["+m2+","+ std::to_string(AVX_V_SIZE) + "]"),
                    M8->nilValue
            );
        }

        std::vector<float> tokens = MR1->AR_F32;
        std::vector<float> mul_value = MR2->AR_F32;

        float matrix[AVX_V_SIZE];
        float matrix_val[AVX_V_SIZE];
        std::copy(tokens.begin(), tokens.end(), matrix);
        std::copy(mul_value.begin(), mul_value.end(), matrix_val);

        simd_vec a;
        simd_vec b;
        simd_vec c;

        #if defined(__AVX512F__)
            a = _mm512_set_ps(matrix[15], matrix[14], matrix[13], matrix[12], 
                              matrix[11], matrix[10], matrix[9], matrix[8], 
                              matrix[7], matrix[6], matrix[5], matrix[4], 
                              matrix[3], matrix[2], matrix[1], matrix[0]);
            b = _mm512_set_ps(matrix_val[15], matrix_val[14], matrix_val[13], matrix_val[12], 
                              matrix_val[11], matrix_val[10], matrix_val[9],  matrix_val[8], 
                              matrix_val[7],  matrix_val[6],  matrix_val[5],  matrix_val[4], 
                              matrix_val[3],  matrix_val[2],  matrix_val[1],  matrix_val[0]);

        #elif defined(__AVX2__) || defined(__AVX__)
            a = _mm256_set_ps(matrix[7], matrix[6], matrix[5], matrix[4], 
                              matrix[3], matrix[2], matrix[1], matrix[0]);
            b = _mm256_set_ps(matrix_val[7], matrix_val[6], matrix_val[5], matrix_val[4], 
                              matrix_val[3], matrix_val[2], matrix_val[1], matrix_val[0]);
        #else
            a = _mm_set_ps(matrix[3], matrix[2], matrix[1], matrix[0]);
            b = _mm_set_ps(matrix_val[3], matrix_val[2], matrix_val[1], matrix_val[0]);
        #endif

        // return std::make_pair(
        //     errorf("Unexpected AVX_V_SIZE ["+ std::to_string(AVX_V_SIZE) + "]"),
        //     M8->nilValue
        // );

        if (op=="mul") {
            c = SIMD_MUL_PS(a, b);
        } else if (op=="add") {
            c = SIMD_ADD_PS(a, b);
        } else if (op=="sub") {
            c = SIMD_SUB_PS(a, b);
        } else {
            return std::make_pair(
                errorf("mat8op Invalid op: ["+op+"]"),
                M8->nilValue
            );
        }

        float d[AVX_V_SIZE];
        SIMD_STORE_PS(d, c);

        // std::cout << "[" << op << "]result equals " << d[0] << "," << d[1]
        //           << "," << d[2] << "," << d[3] << ","
        //           << d[4] << "," << d[5] << "," << d[6] << ","
        //           << d[7] << std::endl;

        REG[rdest] = m8p::m8_obj(M8, m8p::MP8_DF32, "");
        for (int i=0; i<AVX_V_SIZE; i++) {
            REG[rdest]->AR_F32.push_back(d[i]);
        }

        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    std::pair<M8_Error, M8_Obj*> MatCosim_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<3) {
            return std::make_pair(
                errorf("matcosim requires (3) three parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string m1 = params.at(1);// dont forget 0 is for the op_code
        string m2 = params.at(2);
        string rdest = params.at(3);

        auto MR1 = REG[m1];
        auto MR2 = REG[m2];

        if (!IsValid_DF32_Dim(M8, MR1, AVX_V_SIZE)) {
            return std::make_pair(
                errorf("EXPECTING_DIM_FLOAT32_REGISTER["+m1+","+ std::to_string(AVX_V_SIZE) + "]"),
                M8->nilValue
            );
        }
        if (!IsValid_DF32_Dim(M8, MR2, AVX_V_SIZE)) {
            return std::make_pair(
                    errorf("EXPECTING_DIM_FLOAT32_REGISTER["+m2+","+ std::to_string(AVX_V_SIZE) + "]"),
                    M8->nilValue
            );
        }

        std::vector<float> tokens = MR1->AR_F32;
        std::vector<float> mul_value = MR2->AR_F32;

        float matrix[AVX_V_SIZE];
        float matrix_val[AVX_V_SIZE];
        std::copy(tokens.begin(), tokens.end(), matrix);
        std::copy(mul_value.begin(), mul_value.end(), matrix_val);

        simd_vec a;
        simd_vec b;
        simd_vec c;

        #if defined(__AVX512F__)
            a = _mm512_set_ps(matrix[15], matrix[14], matrix[13], matrix[12], 
                              matrix[11], matrix[10], matrix[9], matrix[8], 
                              matrix[7], matrix[6], matrix[5], matrix[4], 
                              matrix[3], matrix[2], matrix[1], matrix[0]);
            b = _mm512_set_ps(matrix_val[15], matrix_val[14], matrix_val[13], matrix_val[12], 
                              matrix_val[11], matrix_val[10], matrix_val[9],  matrix_val[8], 
                              matrix_val[7],  matrix_val[6],  matrix_val[5],  matrix_val[4], 
                              matrix_val[3],  matrix_val[2],  matrix_val[1],  matrix_val[0]);
        #elif defined(__AVX2__) || defined(__AVX__)
            a = _mm256_set_ps(matrix[7], matrix[6], matrix[5], matrix[4], 
                              matrix[3], matrix[2], matrix[1], matrix[0]);
            b = _mm256_set_ps(matrix_val[7], matrix_val[6], matrix_val[5], matrix_val[4], 
                              matrix_val[3], matrix_val[2], matrix_val[1], matrix_val[0]);
        #else
            a = _mm_set_ps(matrix[3], matrix[2], matrix[1], matrix[0]);
            b = _mm_set_ps(matrix_val[3], matrix_val[2], matrix_val[1], matrix_val[0]);
        #endif

        simd_vec dot_sum = SIMD_SETZERO_PS();
        simd_vec norm_a = SIMD_SETZERO_PS();
        simd_vec norm_b = SIMD_SETZERO_PS();

        dot_sum = SIMD_FMA_PS(a, b, dot_sum);  // dot_sum += va * vb
        norm_a = SIMD_FMA_PS(a, a, norm_a);    // norm_a += va * va
        norm_b = SIMD_FMA_PS(b, b, norm_b);    // norm_b += vb * vb

        float dot_result[AVX_V_SIZE], 
             norm_a_result[AVX_V_SIZE], 
             norm_b_result[AVX_V_SIZE];

        SIMD_STORE_PS(dot_result, dot_sum);
        SIMD_STORE_PS(norm_a_result, norm_a);
        SIMD_STORE_PS(norm_b_result, norm_b);

        float dot = 0.0f, na = 0.0f, nb = 0.0f;
        for (size_t i = 0; i < AVX_V_SIZE; i++) {
            dot += dot_result[i];
            na += norm_a_result[i];
            nb += norm_b_result[i];
        }
        
        float res_total = dot / (sqrtf(na) * sqrtf(nb));
        // // ::ALLOC::
        REG[rdest] = m8_obj(M8, res_total);

        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    std::pair<M8_Error, M8_Obj*> MatL2Dist_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<3) {
            return std::make_pair(
                errorf("matl2d requires (3) three parameters (input1, input2, result)"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string m1 = params.at(1);// dont forget 0 is for the op_code
        string m2 = params.at(2);
        string rdest = params.at(3);

        auto MR1 = REG[m1];
        auto MR2 = REG[m2];

        if (!IsValid_DF32_Dim(M8, MR1, AVX_V_SIZE)) {
            return std::make_pair(
                errorf("EXPECTING_DIM_FLOAT32_REGISTER["+m1+","+ std::to_string(AVX_V_SIZE) + "]"),
                M8->nilValue
            );
        }
        if (!IsValid_DF32_Dim(M8, MR2, AVX_V_SIZE)) {
            return std::make_pair(
                    errorf("EXPECTING_DIM_FLOAT32_REGISTER["+m2+","+ std::to_string(AVX_V_SIZE) + "]"),
                    M8->nilValue
            );
        }

        std::vector<float> tokens = MR1->AR_F32;
        std::vector<float> mul_value = MR2->AR_F32;

        float matrix[AVX_V_SIZE];
        float matrix_val[AVX_V_SIZE];
        std::copy(tokens.begin(), tokens.end(), matrix);
        std::copy(mul_value.begin(), mul_value.end(), matrix_val);

        simd_vec a;
        simd_vec b;

        #if defined(__AVX512F__)
            a = _mm512_set_ps(matrix[15], matrix[14], matrix[13], matrix[12], 
                              matrix[11], matrix[10], matrix[9], matrix[8], 
                              matrix[7], matrix[6], matrix[5], matrix[4], 
                              matrix[3], matrix[2], matrix[1], matrix[0]);
            b = _mm512_set_ps(matrix_val[15], matrix_val[14], matrix_val[13], matrix_val[12], 
                              matrix_val[11], matrix_val[10], matrix_val[9],  matrix_val[8], 
                              matrix_val[7],  matrix_val[6],  matrix_val[5],  matrix_val[4], 
                              matrix_val[3],  matrix_val[2],  matrix_val[1],  matrix_val[0]);
        #elif defined(__AVX2__) || defined(__AVX__)
            a = _mm256_set_ps(matrix[7], matrix[6], matrix[5], matrix[4], 
                              matrix[3], matrix[2], matrix[1], matrix[0]);
            b = _mm256_set_ps(matrix_val[7], matrix_val[6], matrix_val[5], matrix_val[4], 
                              matrix_val[3], matrix_val[2], matrix_val[1], matrix_val[0]);
        #else
            a = _mm_set_ps(matrix[3], matrix[2], matrix[1], matrix[0]);
            b = _mm_set_ps(matrix_val[3], matrix_val[2], matrix_val[1], matrix_val[0]);
        #endif

        simd_vec sum = SIMD_SETZERO_PS();
        simd_vec diff = SIMD_SUB_PS(a,b);
        sum = SIMD_FMA_PS(diff, diff, sum); // sum += diff * diff

        float result[AVX_V_SIZE];
        float total = 0.0f;
        SIMD_STORE_PS(result, sum);

        for (size_t i = 0; i < AVX_V_SIZE; i++) {
            total += result[i];
        }

        // ::ALLOC::
        REG[rdest] = m8_obj(M8, sqrtf(total));

        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    std::pair<M8_Error, M8_Obj*> MatNorm_OP(M8System* M8, std::vector<std::string> params) {
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (params.size() < 2) {
            return std::make_pair(errorf("matnorm requires input and output register"), M8->nilValue);
        }

        std::string src = params.at(1);
        std::string dst = params.at(2);

        auto &REG = M8->Registers;
        M8_Obj *A = REG[src];

        if (!A || A->Type != MP8_DF32) {
            return std::make_pair(errorf("matnorm: "+src+" is not a matrix"), M8->nilValue);
        }

        std::vector<float> &vec = A->AR_F32;
        float sumsq = 0.0f;
        for (float v : vec) sumsq += v * v;
        float norm = sqrtf(sumsq);

        if (norm == 0.0f) {
            return std::make_pair(errorf("matnorm: zero vector cannot be normalized"), M8->nilValue);
        }

        REG[dst] = m8p::m8_obj(M8, m8p::MP8_DF32, "");
        for (size_t i = 0; i < vec.size(); ++i) {
            REG[dst]->AR_F32.push_back(vec[i] / norm);
        }
        return std::make_pair(M8_Err_nil, REG[dst]);
    }

    std::pair<M8_Error, M8_Obj*> MatDotProd_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<3) {
            return std::make_pair(
                errorf("matdot requires (3) three parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string m1 = params.at(1);// dont forget 0 is for the op_code
        string m2 = params.at(2);
        string rdest = params.at(3);

        auto MR1 = REG[m1];
        auto MR2 = REG[m2];

        if (!IsValid_DF32_Dim(M8, MR1, AVX_V_SIZE)) {
            return std::make_pair(
                errorf("EXPECTING_DIM_FLOAT32_REGISTER["+m1+","+ std::to_string(AVX_V_SIZE) + "]"),
                M8->nilValue
            );
        }
        if (!IsValid_DF32_Dim(M8, MR2, AVX_V_SIZE)) {
            return std::make_pair(
                    errorf("EXPECTING_DIM_FLOAT32_REGISTER["+m2+","+ std::to_string(AVX_V_SIZE) + "]"),
                    M8->nilValue
            );
        }

        std::vector<float> tokens = MR1->AR_F32;
        std::vector<float> mul_value = MR2->AR_F32;

        float matrix[AVX_V_SIZE];
        float matrix_val[AVX_V_SIZE];
        std::copy(tokens.begin(), tokens.end(), matrix);
        std::copy(mul_value.begin(), mul_value.end(), matrix_val);

        simd_vec a;
        simd_vec b;
        simd_vec c;

        #if defined(__AVX512F__)
            a = _mm512_set_ps(matrix[15], matrix[14], matrix[13], matrix[12], 
                              matrix[11], matrix[10], matrix[9], matrix[8], 
                              matrix[7], matrix[6], matrix[5], matrix[4], 
                              matrix[3], matrix[2], matrix[1], matrix[0]);
            b = _mm512_set_ps(matrix_val[15], matrix_val[14], matrix_val[13], matrix_val[12], 
                              matrix_val[11], matrix_val[10], matrix_val[9],  matrix_val[8], 
                              matrix_val[7],  matrix_val[6],  matrix_val[5],  matrix_val[4], 
                              matrix_val[3],  matrix_val[2],  matrix_val[1],  matrix_val[0]);

        #elif defined(__AVX2__) || defined(__AVX__)
            a = _mm256_set_ps(matrix[7], matrix[6], matrix[5], matrix[4], 
                              matrix[3], matrix[2], matrix[1], matrix[0]);
            b = _mm256_set_ps(matrix_val[7], matrix_val[6], matrix_val[5], matrix_val[4], 
                              matrix_val[3], matrix_val[2], matrix_val[1], matrix_val[0]);
        #else
            a = _mm_set_ps(matrix[3], matrix[2], matrix[1], matrix[0]);
            b = _mm_set_ps(matrix_val[3], matrix_val[2], matrix_val[1], matrix_val[0]);
        #endif

        c = SIMD_MUL_PS(a, b);

        float d[AVX_V_SIZE];
        SIMD_STORE_PS(d, c);

        float total = 0.0f;
        for (size_t i=0; i<AVX_V_SIZE; i++) {
            total += d[i];
        }

        // ::ALLOC::
        REG[rdest] = m8_obj(M8, total);

        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    std::pair<M8_Error, M8_Obj*> MatMul_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("matmul requires two parameters"),
                M8->nilValue
            );
        }

        // std::cout << "AVX_V_SIZE: " << AVX_V_SIZE << std::endl;

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rsource = params.at(1);
        string rdest = params.at(2);// dont forget 0 is for the op_code

        std::vector<float> tokens{1.0, 1.0, 1.0, 1.0, 
                                  1.0, 1.0, 1.0, 1.0};

        if (tokens.size()!=AVX_V_SIZE) {
            return std::make_pair(
                errorf("tokens must contain "+std::to_string(AVX_V_SIZE)+" floats"),
                M8->nilValue
            );
        }

        float matrix[AVX_V_SIZE];
        std::copy(tokens.begin(), tokens.end(), matrix);

        __m256 a = _mm256_set_ps(matrix[0], matrix[1], matrix[2], matrix[3], 
                                 matrix[4], matrix[5], matrix[6], matrix[7]);
        __m256 b = _mm256_set_ps(18.0, 17.0, 16.0, 15.0, 
                                 14.0, 13.0, 12.0, 11.0);

        __m256 c = _mm256_add_ps(a, b);

        float d[AVX_V_SIZE];
        _mm256_storeu_ps(d, c);

        // std::cout << "result equals " << d[0] << "," << d[1]
        //           << "," << d[2] << "," << d[3] << ","
        //           << d[4] << "," << d[5] << "," << d[6] << ","
        //           << d[7] << std::endl;

        REG[rdest] = m8p::m8_obj(M8, m8p::MP8_DF32, "");
        for (int i=0; i<AVX_V_SIZE; i++) {
            REG[rdest]->AR_F32.push_back(d[i]);
        }

        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    std::pair<M8_Error, M8_Obj*> MatMul3_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<3) {
            return std::make_pair(
                errorf("matmul8 requires (3) three parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string m1 = params.at(1);// dont forget 0 is for the op_code
        string m2 = params.at(2);
        string rdest = params.at(3);

        auto MR1 = REG[m1];
        auto MR2 = REG[m2];

        if (!IsValid_DF32_Dim(M8, MR1, AVX_V_SIZE)) {
            return std::make_pair(
                errorf("EXPECTING_DIM_FLOAT32_REGISTER["+m1+"]"),
                M8->nilValue
            );
        }
        if (!IsValid_DF32_Dim(M8, MR2, AVX_V_SIZE)) {
            return std::make_pair(
                errorf("EXPECTING_DIM_FLOAT32_REGISTER["+m2+"]"),
                M8->nilValue
            );
        }

        std::vector<float> tokens = MR1->AR_F32;
        std::vector<float> mul_value = MR2->AR_F32;

        float matrix[AVX_V_SIZE];
        float matrix_val[AVX_V_SIZE];
        std::copy(tokens.begin(), tokens.end(), matrix);
        std::copy(mul_value.begin(), mul_value.end(), matrix_val);

        __m256 a = _mm256_set_ps(matrix[0], matrix[1], matrix[2], matrix[3], 
                                 matrix[4], matrix[5], matrix[6], matrix[7]);
        __m256 b = _mm256_set_ps(matrix_val[0], matrix_val[1], matrix_val[2], matrix_val[3], 
                                 matrix_val[4], matrix_val[5], matrix_val[6], matrix_val[7]);

        // __m256 c = _mm256_add_ps(a, b);
        __m256 c = _mm256_mul_ps(a, b);

        float d[AVX_V_SIZE];
        _mm256_storeu_ps(d, c);

        // std::cout << "result equals " << d[0] << "," << d[1]
        //           << "," << d[2] << "," << d[3] << ","
        //           << d[4] << "," << d[5] << "," << d[6] << ","
        //           << d[7] << std::endl;

        REG[rdest] = m8p::m8_obj(M8, m8p::MP8_DF32, "");
        for (int i=0; i<AVX_V_SIZE; i++) {
            REG[rdest]->AR_F32.push_back(d[i]);
        }

        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }
#endif

#ifdef INCL_FAISS
#endif

    std::pair<M8_Error, M8_Obj*> MatNSet_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("matn requires at least two parameters"),
                M8->nilValue
            );
        }

        std::vector<float> matrix;

        uint32_t t=0;
        for (uint32_t i=2; i<params.size(); ++i) {
            string floatVal = params.at(i);
            float number=0;
            try {
                number=std::stof(floatVal);
                matrix.push_back(number);

            } catch (const std::invalid_argument& ia) {
                return std::make_pair(
                    errorf("EXPECTING_FLOAT32["+floatVal+"]"),
                    M8->nilValue
                );
            }
        }

        if (matrix.size() < 1) {
            return std::make_pair(
                errorf("matrix must contain as least 1 float"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rdest = params.at(1);// dont forget 0 is for the op_code
        REG[rdest] = m8p::m8_obj(M8, m8p::MP8_DF32, "");
        REG[rdest]->AR_F32.clear();

        for (int i=0; i<matrix.size(); i++) {
            REG[rdest]->AR_F32.push_back(matrix.at(i));
        }

        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    std::pair<M8_Error, M8_Obj*> Mat8Set_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("mat8 requires at least two parameters"),
                M8->nilValue
            );
        }

        string rdest = params.at(1);// dont forget 0 is for the op_code

        float matrix[8] = {
            0,0,0,0,
            0,0,0,0
        };

        uint32_t t=0;
        for (uint32_t i=2; i<params.size(); ++i) {
            string floatVal = params.at(i);
            float number=0;
            try {
                number=std::stof(floatVal);
                if (t<8) {
                    matrix[t]=number;
                    t++;
                } else {
                    break;
                }

            } catch (const std::invalid_argument& ia) {
                return std::make_pair(
                    errorf("EXPECTING_FLOAT32["+floatVal+"]"),
                    M8->nilValue
                );
            }
        }

        if (t != 8) {
            return std::make_pair(
                errorf("["+rdest+"] matrix.size != 8"), // TODO: replace with [AVX_V_SIZE]
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;

        REG[rdest] = m8p::m8_obj(M8, m8p::MP8_DF32, "");
        for (int i=0; i<8; i++) {
            REG[rdest]->AR_F32.push_back(matrix[i]);
        }

        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    std::pair<M8_Error, M8_Obj*> F32Mul_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("f32mul requires two parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rdest = params.at(1);// dont forget 0 is for the op_code
        string Value = params.at(2);
        // float number;
        // auto [ptr, ec] = std::from_chars(Value.data(), Value.data()+Value.size(), number);
        // if(ec == std::errc{}){
        // } else {
        //     return std::make_pair(
        //         errorf("EXPECTING_FLOAT32["+Value+"]"),
        //         M8->nilValue
        //     );
        // }
        float number=0;
        try {number=std::stof(Value);}
        catch (const std::invalid_argument& ia) {
            return std::make_pair(
                errorf("EXPECTING_FLOAT32["+Value+"]"),
                M8->nilValue
            );
        }

        if (REG.count(rdest)) {
            M8_Obj *R = REG[rdest];
            if (R==nullptr){
                return std::make_pair(
                    errorf("NULL_REGISTER["+Value+"]"),
                    M8->nilValue
                );
            }
            if (is_nil(M8, R)){
                return std::make_pair(
                    errorf("NIL_REGISTER["+Value+"]"),
                    M8->nilValue
                );
            }

            if (R->Type!=MP8_F32) {
                return std::make_pair(
                    errorf("EXPECTING_FLOAT32_REGISTER["+rdest+"]"),
                    M8->nilValue
                );
            }

            float total = R->F32 * number;
            R->F32 = total;
            return std::make_pair(M8_Err_nil, R);

        } else {
            return std::make_pair(
                errorf("REGISTER_NOT_FOUND["+rdest+"]"),
                M8->nilValue
            );
        }

        return std::make_pair(
            M8_Err_nil,
            REG[rdest]
        );
    }

    //
    // ASSERT
    //
    std::pair<M8_Error, M8_Obj*> AssertEQ_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("asserteq requires two parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rsource = params.at(1);// dont forget 0 is for the op_code
        string Value = params.at(2);

        if (Value.compare(0, 2, "<r")==0) {
            // the value refers to a register
            if (REG.count(Value)) {
                M8_Obj *R = REG[Value];
                if (R==nullptr){
                    return std::make_pair(
                        errorf("NULL_REGISTER["+Value+"]"),
                        M8->nilValue
                    );
                }

                if (is_nil(M8, R)){
                    Value = "";
                } else {
                    Value = R->Value;
                }

                __trim(Value);

            } else {
                return std::make_pair(
                    errorf("REGISTER_NOT_FOUND["+Value+"]"),
                    M8->nilValue
                );
            }
        }

        if (REG.count(rsource)) {
            M8_Obj *R = REG[rsource]; 
            if (R==nullptr){
                return std::make_pair(
                    errorf("NULL_REGISTER["+rsource+"]"),
                    M8->nilValue
                );
            }

            if (R->Type==MP8_TRUE && Value=="true") {
            } else if (R->Type==MP8_FALSE && Value=="false"){
            } else if (R->Type==MP8_NIL && (Value=="nil"||Value=="<nil>")){
            } else if (R->Type==MP8_ERR && Value=="error"){
            } else if (R->Type==MP8_STRING && R->Value==Value){
            } else if (R->Type==MP8_I32){
                std::string str=Value;
                int32_t number;
                auto [ptr, ec] = std::from_chars(str.data(), str.data()+str.size(), number);
                if(ec == std::errc{}){
                    if (number!=R->I32) {
                        return std::make_pair(
                            errorf("ASSERTION_FAILED["+rsource+"]"),
                            M8->nilValue
                        );
                    }
                } else {
                    return std::make_pair(
                        errorf("EXPECTING_INT32["+Value+"]"),
                        M8->nilValue
                    );
                }
            } else if (R->Type==MP8_F32){
                std::string str=Value;
                // float number;
                // auto [ptr, ec] = std::from_chars(str.data(), str.data()+str.size(), number);
                float number=0;
                try {number=std::stof(Value);}
                catch (const std::invalid_argument& ia) {
                    return std::make_pair(
                        errorf("EXPECTING_FLOAT32["+Value+"]"),
                        M8->nilValue
                    );
                }

                if (number!=R->F32) {
                    return std::make_pair(
                        errorf("ASSERTION_FAILED["+rsource+"]"),
                        M8->nilValue
                    );
                }

            } else {
                return std::make_pair(
                    errorf("ASSERTION_FAILED["+rsource+"]"),
                    M8->nilValue
                );
            }

            // if (is_nil(M8, R)){
            //     return std::make_pair(
            //         errorf("NIL_REGISTER["+rsource+"]"),
            //         M8->nilValue
            //     );
            // }
            // REG[rdest] = m8_obj(M8, R->Type, R->Value);
            // REG[rdest] = m8_obj_dup(M8, R);

            return std::make_pair(
                M8_Err_nil,
                M8->true_
            );

        }

        return std::make_pair(
            errorf("REGISTER_NOT_FOUND["+rsource+"]"),
            M8->nilValue
        );
    }

    std::pair<M8_Error, M8_Obj*> AssertEmpty_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<1) {
            return std::make_pair(
                errorf("assertempty requires 1 parameter"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rsource = params.at(1);// dont forget 0 is for the op_code

        if (REG.count(rsource)) {
            M8_Obj *R = REG[rsource]; 
            if (R==nullptr){
                return std::make_pair(
                    errorf("NULL_REGISTER["+rsource+"]"),
                    M8->nilValue
                );
            }

            if (R->Type==MP8_STRING && R->Value==""){
                return std::make_pair(
                    M8_Err_nil,
                    R
                );
            } else {
                return std::make_pair(
                    errorf("ASSERTION_FAILED["+rsource+"]"),
                    M8->nilValue
                );
            }
        }

        return std::make_pair(
            errorf("REGISTER_NOT_FOUND["+rsource+"]"),
            M8->nilValue
        );
    }

    std::pair<M8_Error, M8_Obj*> AssertNotEmpty_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<1) {
            return std::make_pair(
                errorf("assertnotempty requires 1 parameter"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rsource = params.at(1);// dont forget 0 is for the op_code

        if (REG.count(rsource)) {
            M8_Obj *R = REG[rsource]; 
            if (R==nullptr){
                return std::make_pair(
                    errorf("NULL_REGISTER["+rsource+"]"),
                    M8->nilValue
                );
            }

            if (R->Type==MP8_STRING && R->Value.size()>0){
                return std::make_pair(
                    M8_Err_nil,
                    R
                );
            } else {
                return std::make_pair(
                    errorf("ASSERTION_FAILED["+rsource+"]"),
                    M8->nilValue
                );
            }
        }

        return std::make_pair(
            errorf("REGISTER_NOT_FOUND["+rsource+"]"),
            M8->nilValue
        );
    }

    std::pair<M8_Error, M8_Obj*> AssertContains_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<2) {
            return std::make_pair(
                errorf("assertcontains requires at least 2 parameters"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rsource = params.at(1);// dont forget 0 is for the op_code
        string query = params.at(2);
        if (psize>2) {
            for (uint32_t i=3; i<params.size(); ++i) {
                string v = params.at(i);
                query = query + " " + v;
            }
        }

        if (REG.count(rsource)) {
            M8_Obj *R = REG[rsource]; 
            if (R==nullptr){
                return std::make_pair(
                    errorf("NULL_REGISTER["+rsource+"]"),
                    M8->nilValue
                );
            }

            if (R->Type==MP8_STRING && R->Value.find(query)!=std::string::npos){
                return std::make_pair(
                    M8_Err_nil,
                    R
                );

            } else {
                return std::make_pair(
                    errorf("ASSERTION_FAILED["+rsource+"]"),
                    M8->nilValue
                );
            }
        }

        return std::make_pair(
            errorf("REGISTER_NOT_FOUND["+rsource+"]"),
            M8->nilValue
        );
    }

    std::pair<M8_Error, M8_Obj*> AssertNil_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<1) {
            return std::make_pair(
                errorf("assertnil requires 1 parameter"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rsource = params.at(1);// dont forget 0 is for the op_code

        if (REG.count(rsource)) {
            M8_Obj *R = REG[rsource]; 
            if (R==nullptr){
                return std::make_pair(
                    errorf("NULL_REGISTER["+rsource+"]"),
                    M8->nilValue
                );
            }

            if (is_nil(M8,R)){
                return std::make_pair(
                    M8_Err_nil,
                    R
                );

            } else {
                return std::make_pair(
                    errorf("ASSERTION_FAILED["+rsource+"]"),
                    M8->nilValue
                );
            }
        }

        return std::make_pair(
            errorf("REGISTER_NOT_FOUND["+rsource+"]"),
            M8->nilValue
        );
    }

    std::pair<M8_Error, M8_Obj*> AssertError_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<1) {
            return std::make_pair(
                errorf("asserterror requires 1 parameter"),
                M8->nilValue
            );
        }

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        string rsource = params.at(1);// dont forget 0 is for the op_code

        if (REG.count(rsource)) {
            M8_Obj *R = REG[rsource]; 
            if (R==nullptr){
                return std::make_pair(
                    errorf("NULL_REGISTER["+rsource+"]"),
                    M8->nilValue
                );
            }

            if (is_error(M8,R)){
                return std::make_pair(
                    M8_Err_nil,
                    R
                );

            } else {
                return std::make_pair(
                    errorf("ASSERTION_FAILED["+rsource+"]"),
                    M8->nilValue
                );
            }
        }

        return std::make_pair(
            errorf("REGISTER_NOT_FOUND["+rsource+"]"),
            M8->nilValue
        );
    }

    //
    // STRING OPERATIONS
    //

    //
    // LOG & STDOUT
    //
    std::pair<M8_Error, M8_Obj*> PrintR_OP(M8System* M8, std::vector<std::string> params){
        int psize = __abs(params.size()-1); // -1 accounts for the opcode itself
        if (psize<1) {
            return std::make_pair(
                errorf("print_r requires at least 1 parameter"),
                M8->nilValue
            );
        }

        string rsource = params.at(1); // dont forget 0 is for the op_code

        std::map<std::string, M8_Obj*> &REG = M8->Registers;
        // std::map<string, M8_Obj*>::iterator iter = REG.begin();
        // for(; iter != REG.end(); ++iter) {
        //     string k = iter->first;
        //     std::cout << k << "\n";
        // }

        if (REG.count(rsource)) {
            // std::cout << REGISTER
            M8_Obj *R = REG[rsource]; 
            if (R==nullptr){ 
                std::cout << "REGISTER[" 
                    << rsource
                    << "] = null"
                    << "\n";

            } else if (is_nil(M8, R)){
                std::cout << "REGISTER[" 
                    << rsource
                    << "] = nil"
                    << "\n";

            } else if (R->Type==MP8_I32){
                std::cout << "REGISTER[" 
                    << rsource
                    << "] = "
                    << R->I32 
                    << "\n";

            } else if (R->Type==MP8_F32){
                std::cout << "REGISTER[" 
                    << rsource
                    << "] = "
                    << R->F32 
                    << "\n";

            } else if (R->Type==MP8_DI32){
                std::cout << "REGISTER[" 
                    << rsource
                    << "] = ";

                std::cout << "[";
                for (std::vector<int32_t>::iterator i=R->AR_I32.begin(); i!=R->AR_I32.end(); ++i) {
                    std::cout << *i << ",";
                }
                std::cout << "]\n";

            } else if (R->Type==MP8_DF32){
                std::cout << "REGISTER[" 
                    << rsource
                    << "] = ";

                std::cout << "[";
                for (std::vector<float>::iterator i=R->AR_F32.begin(); i!=R->AR_F32.end(); ++i) {
                    std::cout << *i << ",";
                }
                std::cout << "]\n";

            } else {
                std::cout << "REGISTER[" 
                    << rsource
                    << "] = "
                    << R->Value
                    << "\n";
            }

        } else {
            std::cout << "REGISTER_NOT_FOUND[" 
                << rsource 
                << "]"
                << "\n" ; 
        }

        return std::make_pair(
            M8_Err_nil,
            M8->nilValue
        );
    }



    //
    // EVAL LOOP || INTEPRETER
    //
    std::pair<M8_Error, M8_Obj*> Run(M8System* M8, std::string code_buf) {
        // RETURN std::pair<M8_Error, M8_Obj*>
        std::pair<M8_Error, M8_Obj*> lastRet = std::make_pair(
            M8_Err_nil,
            M8->nilValue
        );

        if (code_buf.size()==0) {
            std::cout << "EMPTY BUFFER: ERROR\n";
            return std::make_pair(
                errorf("empty buffer"),
                M8->nilValue
            );
        }

        std::vector<string> Lines = __split(code_buf, "\n");
        int64_t instr_count = Lines.size();

        if (instr_count==0) {
            std::cout << "EMPTY LINE BUFFER: ERROR(instr_count==0)\n";
            return std::make_pair(
                errorf("empty_line_buffer(instr_count==0)"),
                M8->nilValue
            );
        }

        // const int LOOP_MAX_ITERATIONS = 1000;

        for (int i=0; i<instr_count; ) {
            const int inst_pointer = i;
            std::string Line = Lines.at(i);
            __trim(Line);

            if (Line.size()==0 || Line.rfind("#",0)==0) { // this is comment/empty line
                i++;
                continue;
            }

            size_t clinex = Line.find("#");
            if (clinex!=std::string::npos) {
                Line = Line.substr(0, clinex);
            }

            std::vector<string> instr_tokens = __split(Line, " ");
            if (instr_tokens.size()==0) {
                i++;
                continue;                
            }

            string opCode = instr_tokens.at(0);

            if (opCode=="store" || opCode=="set") {
                lastRet = Store_OP(M8, instr_tokens);

            } else if (opCode=="clr") {
                lastRet = Clear_OP(M8, instr_tokens);

            } else if (opCode=="load"||opCode=="dup") {// on cpp load and dup does the same
                lastRet = Load_OP(M8, instr_tokens);

            } else if (opCode=="asserteq" || opCode=="assert") {
                lastRet = AssertEQ_OP(M8, instr_tokens);

            } else if (opCode=="assertempty") {
                lastRet = AssertEmpty_OP(M8, instr_tokens);

            } else if (opCode=="assertnotempty") {
                lastRet = AssertNotEmpty_OP(M8, instr_tokens);

            } else if (opCode=="assertcontains") {
                lastRet = AssertContains_OP(M8, instr_tokens);

            } else if (opCode=="assertnil") {
                lastRet = AssertNil_OP(M8, instr_tokens);

            } else if (opCode=="asserterror") {
                lastRet = AssertError_OP(M8, instr_tokens);

            // } else if (opCode=="resetr") {

            // } else if (opCode=="llmcall") {
            // } else if (opCode=="ifeq") {
            // } else if (opCode=="ifneq") {

            // } else if (opCode=="loopeq") {
            // } else if (opCode=="loopgt") {
            // } else if (opCode=="loopgte") {
            // } else if (opCode=="stall") {

            // } else if (opCode=="scontains") {
            // } else if (opCode=="sprefix") {
            // } else if (opCode=="sreplace") {
            // } else if (opCode=="ssufix") {
            // } else if (opCode=="slen") {
            // } else if (opCode=="sminlen") {
            // } else if (opCode=="smaxlen") {
            // } else if (opCode=="strim") {
            // } else if (opCode=="supper") {
            // } else if (opCode=="slower") {
            // } else if (opCode=="sslice") {
            // } else if (opCode=="sextract") {
            // } else if (opCode=="sconcat") {

            } else if (opCode=="ret"||opCode=="return") {
                lastRet = Ret_OP(M8, instr_tokens);
 
             } else if (opCode=="i32mul") {
                lastRet = I32Mul_OP(M8, instr_tokens);

            } else if (opCode=="i32set") {
                lastRet = I32Set_OP(M8, instr_tokens);

            } else if (opCode=="i32add") {
                lastRet = I32Add_OP(M8, instr_tokens);

            } else if (opCode=="i32sub") {
                lastRet = I32Sub_OP(M8, instr_tokens);

            } else if (opCode=="stall") {
                lastRet = Stall_OP(M8, instr_tokens);

            } else if (opCode=="ustall") {
                lastRet = UStall_OP(M8, instr_tokens);


            } else if (opCode=="mat8") {
                lastRet = Mat8Set_OP(M8, instr_tokens);
            } else if (opCode=="matn") {
                lastRet = MatNSet_OP(M8, instr_tokens);

            } else if (opCode=="align8") {
                lastRet = ALIGN8_OP(M8, instr_tokens);
            } else if (opCode=="align") {
                lastRet = ALIGN_OP(M8, instr_tokens);
#ifdef __AVX__
            } else if (opCode=="matadd") {
                lastRet = Mat8_OP("add", M8, instr_tokens);

            } else if (opCode=="matsub") {
                lastRet = Mat8_OP("sub", M8, instr_tokens);

            } else if (opCode=="matmul") {
                lastRet = Mat8_OP("mul", M8, instr_tokens);

            } else if (opCode=="matdot") {
                lastRet = MatDotProd_OP(M8, instr_tokens);

            } else if (opCode=="matnorm") {
                lastRet = MatNorm_OP(M8, instr_tokens);

            } else if (opCode=="matcosim") {
                lastRet = MatCosim_OP(M8, instr_tokens);

            } else if (opCode=="matl2d") {
                lastRet = MatL2Dist_OP(M8, instr_tokens);

            // } else if (opCode=="matmul8") {
            //     lastRet = MatMul3_OP(M8, instr_tokens);
#endif

            } else if (opCode=="f32set") {
                lastRet = F32Set_OP(M8, instr_tokens);

            } else if (opCode=="f32add") {
                lastRet = F32Add_OP(M8, instr_tokens);

            } else if (opCode=="f32sub") {
                lastRet = F32Sub_OP(M8, instr_tokens);

            } else if (opCode=="f32mul") {
                lastRet = F32Mul_OP(M8, instr_tokens);

            // } else if (opCode=="jfield") {
            // } else if (opCode=="set") {
            // } else if (opCode=="del") {
            // } else if (opCode=="get") {
            // } else if (opCode=="putrm") {
            // } else if (opCode=="putmr") {
            // } else if (opCode=="resetm") {
            // } else if (opCode=="print") {

            } else if (opCode=="print_r") {
                PrintR_OP(M8, instr_tokens);

            } else {
                VInstr* virtual_call = M8->Virtual[opCode];
                if (virtual_call!=nullptr) {
                    // std::cout << "BUILTIN:: " << opCode << "\n";
                    lastRet = virtual_call->Call(M8, instr_tokens);
                } else {
                    VInstr* __all__ = M8->Virtual["__all__"];
                    if (__all__!=nullptr) {
                        lastRet = __all__->Call(M8, instr_tokens);
                    } else {
                        lastRet = std::make_pair(
                            errorf("INVALID_INSTRUCTION["+opCode+"]"),
                            M8->nilValue
                        );
                    }
                }
            }

            if (lastRet.first.Type!=M8_Err_nil.Type) {
                std::cout << "Error returned at Line " << (inst_pointer+1) << "  "
                    << "Near: " << Line
                    << "\n";

                std::cout << "    ERROR: " << lastRet.first.Details 
                    << "\n";
                std::cout << std::endl;
                return lastRet;

            } else {
                if (opCode=="ret"||opCode=="return") {
                    return lastRet;
                }                
            }

            i++;
        }

        return lastRet;
    }



    // COMPILER_MODE = "normal"
    // COMPILER_NAME = "jensen"
    // COMPILER_VERSION = "0.0.1-2024-09-07"    
}


#endif