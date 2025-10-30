## Advanced Recommendation System with Multi-Factor Scoring
## Combines content similarity, user preferences, and popularity metrics

vdb_instance ITEMS dim=16 max_elements=400 M=20 ef_construction=250
vdb_instance USER_HISTORY dim=16 max_elements=400 M=20 ef_construction=250

## Store item catalog with rich descriptions
store <item1> Sci-fi thriller with time travel and parallel universes
llm_embed <item1> <i1> dim=16
align <i1> 16
vdb_add ITEMS <i1> SciFi Thriller TimeTravel

store <item2> Romantic comedy set in Paris with quirky characters
llm_embed <item2> <i2> dim=16
align <i2> 16
vdb_add ITEMS <i2> RomCom Paris Quirky

store <item3> Action movie with espionage and global conspiracy
llm_embed <item3> <i3> dim=16
align <i3> 16
vdb_add ITEMS <i3> Action Espionage Conspiracy

store <item4> Space opera with alien civilizations and epic battles
llm_embed <item4> <i4> dim=16
align <i4> 16
vdb_add ITEMS <i4> SpaceOpera Aliens Epic

store <item5> Mystery detective story in Victorian London
llm_embed <item5> <i5> dim=16
align <i5> 16
vdb_add ITEMS <i5> Mystery Victorian Detective

store <item6> Cyberpunk dystopia with AI rebellion themes
llm_embed <item6> <i6> dim=16
align <i6> 16
vdb_add ITEMS <i6> Cyberpunk Dystopia AI

store <item7> Heartwarming drama about family and forgiveness
llm_embed <item7> <i7> dim=16
align <i7> 16
vdb_add ITEMS <i7> Drama Family Heartwarming

store <item8> Horror anthology with supernatural elements
llm_embed <item8> <i8> dim=16
align <i8> 16
vdb_add ITEMS <i8> Horror Supernatural Anthology

## User interaction history
store <h1> User loved sci-fi thriller with time travel
llm_embed <h1> <u1> dim=16
align <u1> 16
vdb_add USER_HISTORY <u1> Loved SciFi TimeTravel

store <h2> User enjoyed space opera with aliens
llm_embed <h2> <u2> dim=16
align <u2> 16
vdb_add USER_HISTORY <u2> Enjoyed SpaceOpera

store <h3> User liked cyberpunk AI themes
llm_embed <h3> <u3> dim=16
align <u3> 16
vdb_add USER_HISTORY <u3> Liked Cyberpunk AI

## Build user preference profile by averaging history
matadd <u1> <u2> <temp_prof1>
matadd <temp_prof1> <u3> <user_profile>

## Calculate affinity scores for each item
matcosim <user_profile> <i1> <score1>
matcosim <user_profile> <i2> <score2>
matcosim <user_profile> <i3> <score3>
matcosim <user_profile> <i4> <score4>
matcosim <user_profile> <i5> <score5>
matcosim <user_profile> <i6> <score6>
matcosim <user_profile> <i7> <score7>
matcosim <user_profile> <i8> <score8>

## Item-to-item similarity matrix (for "similar items" feature)
matcosim <i1> <i4> <sci_space_sim>
matcosim <i1> <i6> <sci_cyber_sim>
matcosim <i3> <i6> <action_cyber_sim>
matcosim <i2> <i7> <rom_drama_sim>

## Diversity score (avoid echo chamber)
matcosim <i1> <i2> <diversity_1_2>
matcosim <i4> <i7> <diversity_4_7>
matcosim <i6> <i5> <diversity_6_5>

## Create weighted scoring system
## Weights: relevance, novelty, diversity, popularity, quality
mat8 <rec_weights> 0.45 0.25 0.15 0.10 0.05 0 0 0
align <rec_weights> 16

## Build feature vectors for hybrid scoring
mat8 <item1_features> 0.95 0.20 0.30 0.80 0.90 0.10 0.05 0.02
align <item1_features> 16
mat8 <item4_features> 0.88 0.30 0.25 0.75 0.85 0.15 0.08 0.03
align <item4_features> 16
mat8 <item6_features> 0.92 0.40 0.20 0.70 0.88 0.12 0.06 0.02
align <item6_features> 16

## Calculate weighted recommendation scores
matdot <rec_weights> <item1_features> <final_score1>
matdot <rec_weights> <item4_features> <final_score4>
matdot <rec_weights> <item6_features> <final_score6>

## Find items similar to user's favorite (item1)
vdb_search ITEMS <i1> <similar_to_fav> distance=0.6
llm_detokenize <similar_to_fav> <similar_item>

## Cold start problem - new user gets popular items
store <new_user_query> exciting content with adventure
llm_embed <new_user_query> <new_user_emb> dim=16
vdb_search ITEMS <new_user_emb> <cold_start_rec> distance=0.7
llm_detokenize <cold_start_rec> <cold_start_item>

## Serendipity engine - find unexpected matches
matl2d <user_profile> <i7> <surprise_distance>
matcosim <user_profile> <i7> <surprise_similarity>

## Genre cluster analysis
matadd <i1> <i4> <temp_sci>
matadd <temp_sci> <i6> <sci_cluster>
matadd <i2> <i7> <emotion_cluster>

matcosim <sci_cluster> <emotion_cluster> <genre_diversity>

## Exploration vs exploitation balance
matcosim <u1> <u2> <history_consistency>
matcosim <u2> <u3> <taste_stability>

## Build ranked recommendation list
store <rec_report> Top match score: <score6> for item6 | Item1 affinity: <score1> | Item4 affinity: <score4> | Similar to favorite: <similar_item> | Cold start rec: <cold_start_item> | Sci-fi cluster strength: <sci_space_sim> | Cross-genre link: <action_cyber_sim> | Diversity metric: <genre_diversity> | Surprise candidate distance: <surprise_distance> | Weighted item1: <final_score1> | Weighted item6: <final_score6> | Taste stability: <taste_stability>

ret <rec_report>