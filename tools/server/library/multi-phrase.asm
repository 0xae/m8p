## Use multiple narrow embeddings and average the scores

store <h1> interest rate hike
llm_embed <h1> <h1_emb> dim=16
store <h2> tightening policy
llm_embed <h2> <h2_emb> dim=16
store <h3> restrictive stance
llm_embed <h3> <h3_emb> dim=16

store <d1> interest rate cut
llm_embed <d1> <d1_emb> dim=16
store <d2> easing policy
llm_embed <d2> <d2_emb> dim=16
store <d3> accommodative stance
llm_embed <d3> <d3_emb> dim=16

## Score JH2022 against each
matcosim <jh> <h1_emb> <jh_h1>
matcosim <jh> <h2_emb> <jh_h2>
matcosim <jh> <h3_emb> <jh_h3>
matcosim <jh> <d1_emb> <jh_d1>
matcosim <jh> <d2_emb> <jh_d2>
matcosim <jh> <d3_emb> <jh_d3>

## Average hawkish scores
f32set <jh_hawk_avg> <jh_h1>
f32add <jh_hawk_avg> <jh_h2>
f32add <jh_hawk_avg> <jh_h3>
f32mul <jh_hawk_avg> 0.333

## Average dovish scores
f32set <jh_dove_avg> <jh_d1>
f32add <jh_dove_avg> <jh_d2>
f32add <jh_dove_avg> <jh_d3>
f32mul <jh_dove_avg> 0.333

store <multi_phrase> JH2022: Hawk_avg=<jh_hawk_avg> Dove_avg=<jh_dove_avg> | Individual: h1=<jh_h1> h2=<jh_h2> h3=<jh_h3> d1=<jh_d1> d2=<jh_d2> d3=<jh_d3>

ret <multi_phrase>