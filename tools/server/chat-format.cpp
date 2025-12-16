// std::stringstream ss_prompt;
// ss_prompt 
//     << "<start_of_turn>system\n"
//     << system_prompt 
//     << "<end_of_turn>\n"
//     << "<start_of_turn>user\n"
//     << last_question
//     << "<end_of_turn>\n"
//     << "<start_of_turn>assistant\n"
//     << last_msg
//     << "<start_of_turn>user\n"
//     << prompt
//     << "<end_of_turn>\n"
//     << "<start_of_turn>assistant";

// chat_prompt = ss_prompt.str();

// if (Ref.Status==1) { // last iteration was successful

// } else if (Ref.Status==0){ // last iteration was an error

// } else if (Ref.Status==2) { // in processing
//     // TODO
//     return std::make_pair(
//         m8p::M8_Err_nil,
//         M8->true_
//     );
// }
// LLMDB[ins_name].Status = 1; // success
// LLMDB[ins_name].arr = arr;
// return true;
// LLMDB[ins_name].Status = 0; // an error ocurred
// LLMDB[ins_name].arr = error_data;

// {"presence_penalty", 0},
// {"reasoning_format", "auto"},
// {"timings_per_token", false},
// {"frequency_penalty", 0},
// {"repeat_penalty", 1},
// {"top_k", 40},
// {"top_p", 0.95},
// {"preserved_tokens", json::array()},
// {"grammar_lazy", false},
// {"grammar_triggers", json::array()},
// {"thinking_forced_open", false},
// {"chat_format", json::array()}


// std::stringstream ss_prompt;
// ss_prompt 
//     << "<start_of_turn>user\n"
//     << system_prompt 
//     << "\n\n"
//     << prompt
//     << "<end_of_turn>\n"
//     << "<start_of_turn>model\n"
// ;
// chat_prompt = ss_prompt.str();
// data["prompt"] = chat_prompt;