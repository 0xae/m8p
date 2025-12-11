// if (REG.count(rsource)) {
//     M8_Obj *R = REG[rsource]; 
//     if (R==nullptr){
//         return std::make_pair(
//             errorf("NULL_REGISTER["+rsource+"]"),
//             M8->nilValue
//         );
//     }

//     if (R->Type==MP8_TRUE && Value=="true") {
//     } else if (R->Type==MP8_FALSE && Value=="false"){
//     } else if (R->Type==MP8_NIL && (Value=="nil"||Value=="<nil>")){
//     } else if (R->Type==MP8_ERR && Value=="error"){
//     } else if (R->Type==MP8_STRING && R->Value==Value){
//     } else if (R->Type==MP8_I32){
//         int32_t number;
//         if (Value.compare(0, 1, "<")==0) {
//             M8_Obj *R = REG[Value];
//             if (R==nullptr || is_nil(M8,R) || R->Type!=MP8_I32) {
//                 return std::make_pair(
//                     errorf("EXPECTING_INT32_REGISTER["+Value+"]"),
//                     M8->nilValue
//                 );
//             }
//             number = R->I32;

//         } else {
//             std::string str=Value;
//             auto [ptr, ec] = std::from_chars(str.data(), str.data()+str.size(), number);
//             if(ec == std::errc{}){
//             } else {
//                 return std::make_pair(
//                     errorf("EXPECTING_INT32["+Value+"]"),
//                     M8->nilValue
//                 );
//             }
//         }

//         if (number!=R->I32) {
//             return std::make_pair(
//                 errorf("ASSERTION_FAILED["+rsource+"]"),
//                 M8->nilValue
//             );
//         }

//     } else if (R->Type==MP8_F32){
//         float number=0;
//         if (Value.compare(0, 1, "<")==0) {
//             M8_Obj *R = REG[Value];
//             if (R==nullptr || is_nil(M8,R) || R->Type!=MP8_F32) {
//                 return std::make_pair(
//                     errorf("EXPECTING_FLOAT32_REGISTER["+Value+"]"),
//                     M8->nilValue
//                 );
//             }
//             number = R->F32;

//         } else {
//             std::string str=Value;
//             auto [ptr, ec] = std::from_chars(str.data(), str.data()+str.size(), number);
//             if(ec == std::errc{}){
//             } else {
//                 return std::make_pair(
//                     errorf("EXPECTING_FLOAT32["+Value+"]"),
//                     M8->nilValue
//                 );
//             }
//         }

//         if (number!=R->F32) {
//             return std::make_pair(
//                 errorf("ASSERTION_FAILED["+rsource+"]"),
//                 M8->nilValue
//             );
//         }

//     } else {
//         return std::make_pair(
//             errorf("ASSERTION_FAILED["+rsource+"]"),
//             M8->nilValue
//         );
//     }

//     // if (is_nil(M8, R)){
//     //     return std::make_pair(
//     //         errorf("NIL_REGISTER["+rsource+"]"),
//     //         M8->nilValue
//     //     );
//     // }
//     // REG[rdest] = m8_obj(M8, R->Type, R->Value);
//     // REG[rdest] = m8_obj_dup(M8, R);

//     return std::make_pair(
//         M8_Err_nil,
//         M8->true_
//     );
// }
// return std::make_pair(
//     errorf("REGISTER_NOT_FOUND["+rsource+"]"),
//     M8->nilValue
// );