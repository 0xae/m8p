## Create ultra-simple, focused reference vectors

## Pure hawkish actions and outcomes
store <pure_hawk> rate hikes tightening restrictive higher
llm_embed <pure_hawk> <ph> dim=16

## Pure dovish actions and outcomes  
store <pure_dove> rate cuts easing accommodation lower
llm_embed <pure_dove> <pd> dim=16

## Now test the problematic cases
store <jh2022_test> Restoring price stability requires maintaining restrictive stance some time keep at it job done some pain costs reducing inflation
llm_embed <jh2022_test> <jh_test> dim=16

matcosim <jh_test> <ph> <jh_hawk_pure>
matcosim <jh_test> <pd> <jh_dove_pure>

store <mar2020_test> The Committee decided to lower the target range for the federal funds rate to 0 to 0.25 percent
llm_embed <mar2020_test> <m20_test> dim=16

matcosim <m20_test> <ph> <m20_hawk_pure>
matcosim <m20_test> <pd> <m20_dove_pure>

store <dec2023_test> The Committee decided to maintain the target range at 5.25 to 5.5 percent prepared to adjust if risks emerge inflation has eased
llm_embed <dec2023_test> <d23_test> dim=16

matcosim <d23_test> <ph> <d23_hawk_pure>
matcosim <d23_test> <pd> <d23_dove_pure>

## Also test with individual concept embeddings
store <concept_hike> interest rate increase
llm_embed <concept_hike> <c_hike> dim=16
store <concept_cut> interest rate decrease
llm_embed <concept_cut> <c_cut> dim=16

matcosim <jh_test> <c_hike> <jh_vs_hike>
matcosim <jh_test> <c_cut> <jh_vs_cut>
matcosim <m20_test> <c_hike> <m20_vs_hike>
matcosim <m20_test> <c_cut> <m20_vs_cut>

store <purity_test> JH2022 vs pure_hawk=<jh_hawk_pure> vs pure_dove=<jh_dove_pure> vs hike=<jh_vs_hike> vs cut=<jh_vs_cut> | MAR2020 vs pure_hawk=<m20_hawk_pure> vs pure_dove=<m20_dove_pure> vs hike=<m20_vs_hike> vs cut=<m20_vs_cut> | DEC2023 vs pure_hawk=<d23_hawk_pure> vs pure_dove=<d23_dove_pure>

ret <purity_test>