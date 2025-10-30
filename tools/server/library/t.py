## Conversational Memory AI - Learns facts and answers questions

vdb_instance MEMORY dim=48 max_elements=200 M=24 ef_construction=300

## Initialize the LLM properly
store <init> You are a helpful assistant that remembers personal information
# llm_instance <init> chatbot n_predict=50 temperature=0.7

## Store some conversational facts
store <fact1> My favorite color is blue and I love ocean sunsets
llm_embed <fact1> <e1> dim=48
align <e1> 48
vdb_add MEMORY <e1> User likes blue color and ocean sunsets

store <fact2> I work as a software engineer at a startup in San Francisco
llm_embed <fact2> <e2> dim=48
align <e2> 48
vdb_add MEMORY <e2> User is software engineer at SF startup

store <fact3> My hobbies include rock climbing and playing guitar
llm_embed <fact3> <e3> dim=48
align <e3> 48
vdb_add MEMORY <e3> User enjoys rock climbing and guitar

store <fact4> I have a golden retriever named Max who is 3 years old
llm_embed <fact4> <e4> dim=48
align <e4> 48
vdb_add MEMORY <e4> User has 3 year old golden retriever named Max

store <fact5> I studied computer science at Stanford from 2015 to 2019
llm_embed <fact5> <e5> dim=48
align <e5> 48
vdb_add MEMORY <e5> User studied CS at Stanford 2015-2019

## Now ask questions that require connecting different memories
store <q1> What do I do for work and where
llm_embed <q1> <qe1> dim=48
align <qe1> 48
vdb_search MEMORY <qe1> <m1> distance=0.6
llm_detokenize <m1> <answer1>

store <q2> Tell me about my pets
llm_embed <q2> <qe2> dim=48
align <qe2> 48
vdb_search MEMORY <qe2> <m2> distance=0.6
llm_detokenize <m2> <answer2>

## Calculate semantic similarity between different life aspects
store <aspect1> professional career and education
store <aspect2> personal hobbies and interests
llm_embed <aspect1> <ea1> dim=16
llm_embed <aspect2> <ea2> dim=16
matcosim <ea1> <ea2> <life_similarity>

## Use LLM to generate personalized response based on memories
store <prompt> Based on this info: <answer1> and <answer2>, write a short bio
llm_instance <prompt> biowriter n_predict=80 temperature=0.6
llm_instancestatus biowriter <generated_bio>

## Create a weighted scoring system for memory relevance
mat8 <importance_weights> 0.3 0.25 0.2 0.15 0.05 0.03 0.01 0.01
align <importance_weights> 8

## Score different query types
store <career_q> What is my professional background
llm_embed <career_q> <cqe> dim=48
vdb_search MEMORY <cqe> <career_match> distance=0.5
llm_detokenize <career_match> <career_info>

store <hobby_q> What do I like to do for fun
llm_embed <hobby_q> <hqe> dim=48
vdb_search MEMORY <hqe> <hobby_match> distance=0.5
llm_detokenize <hobby_match> <hobby_info>

## Multi-hop reasoning: Find related memories
store <color_q> What colors might I like based on my interests
llm_embed <color_q> <colqe> dim=48
vdb_search MEMORY <colqe> <color_match> distance=0.7
llm_detokenize <color_match> <color_inference>

## Calculate memory coherence score
llm_embed <answer1> <ans1_emb> dim=16
llm_embed <answer2> <ans2_emb> dim=16
matcosim <ans1_emb> <ans2_emb> <coherence>

store <final> Memory 1: <answer1> | Memory 2: <answer2> | Coherence: <coherence> | Bio: <generated_bio> | Career: <career_info> | Hobbies: <hobby_info> | Color insight: <color_inference>

ret <final>