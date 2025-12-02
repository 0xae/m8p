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