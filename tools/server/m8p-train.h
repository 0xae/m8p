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