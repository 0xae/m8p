store <tightening> raising rates hiking restrictive
llm_embed <tightening> <tight> dim=16

store <easing> cutting rates lowering accommodative  
llm_embed <easing> <ease> dim=16

store <neutral> maintaining rates unchanged steady
llm_embed <neutral> <neut> dim=16

## Score each statement
matcosim <jh_test> <tight> <jh_tightening>
matcosim <jh_test> <ease> <jh_easing>
matcosim <jh_test> <neut> <jh_neutral>

matcosim <m20_test> <tight> <m20_tightening>
matcosim <m20_test> <ease> <m20_easing>
matcosim <m20_test> <neut> <m20_neutral>

matcosim <d23_test> <tight> <d23_tightening>
matcosim <d23_test> <ease> <d23_easing>
matcosim <d23_test> <neut> <d23_neutral>

store <directional> JH2022: Tightening=<jh_tightening> Easing=<jh_easing> Neutral=<jh_neutral> | MAR2020: Tightening=<m20_tightening> Easing=<m20_easing> Neutral=<m20_neutral> | DEC2023: Tightening=<d23_tightening> Easing=<d23_easing> Neutral=<d23_neutral>

ret <directional>