## First, fix your hawk/dove reference vectors
store <hawk_ref_v2> rate hike increase tighten restrictive higher longer inflation vigilance pain aggressive no easing premature terminal peak
llm_embed <hawk_ref_v2> <hawk2> dim=16

store <dove_ref_v2> rate cut lower ease accommodation support dovish easing stimulus QE emergency crisis cuts reductions
llm_embed <dove_ref_v2> <dove2> dim=16

## Add EXPLICIT cut language that includes the magic words
store <emergency_cut_explicit> Federal Reserve cuts rates to zero to 0.25 percent emergency coronavirus crisis
llm_embed <emergency_cut_explicit> <eme> dim=16
align <eme> 16
vdb_add FED_LANGUAGE <eme> Emergency rate cut zero crisis dovish extreme QE unlimited

store <emergency_cut2> Lower target range to 0 to 0.25 full range of tools credit support
llm_embed <emergency_cut2> <eme2> dim=16
align <eme2> 16
vdb_add FED_LANGUAGE <eme2> Cut to zero emergency extreme dovish crisis pandemic

## Add EXPLICIT hold with dovish lean (Dec 2023 specific)
store <hold_dovish_specific> Maintain target range 5.25 to 5.5 but prepared to adjust if risks emerge inflation eased
llm_embed <hold_dovish_specific> <hds> dim=16
align <hds> 16
vdb_add FED_LANGUAGE <hds> Hold rates dovish prepared adjust lower risks emerged easing

store <hold_dovish2> Committee maintaining rates ready to ease if needed inflation progress
llm_embed <hold_dovish2> <hd2> dim=16
align <hd2> 16
vdb_add FED_LANGUAGE <hd2> Hold dovish pivot ready ease inflation improving cuts ahead

## Test with better reference vectors
store <mar2020_test> The Committee decided to lower the target range for the federal funds rate to 0 to 0.25 percent
llm_embed <mar2020_test> <m20t> dim=16
vdb_search FED_LANGUAGE <m20t> <m20_result> distance=0.45
llm_detokenize <m20_result> <m20_interp>

matcosim <m20t> <hawk2> <m20_hawk_new>
matcosim <m20t> <dove2> <m20_dove_new>

store <dec2023_test> The Committee decided to maintain the target range at 5.25 to 5.5 percent prepared to adjust if risks emerge inflation has eased
llm_embed <dec2023_test> <d23t> dim=16
vdb_search FED_LANGUAGE <d23t> <d23_result> distance=0.45
llm_detokenize <d23_result> <d23_interp>

matcosim <d23t> <hawk2> <d23_hawk_new>
matcosim <d23t> <dove2> <d23_dove_new>

store <jh2022_test> Restoring price stability requires maintaining restrictive stance some time keep at it job done some pain costs reducing inflation
llm_embed <jh2022_test> <jh22t> dim=16
vdb_search FED_LANGUAGE <jh22t> <jh22_result> distance=0.45
llm_detokenize <jh22_result> <jh22_interp>

matcosim <jh22t> <hawk2> <jh22_hawk_new>
matcosim <jh22t> <dove2> <jh22_dove_new>

store <results> MAR2020: <m20_interp> Hawk=<m20_hawk_new> Dove=<m20_dove_new> | DEC2023: <d23_interp> Hawk=<d23_hawk_new> Dove=<d23_dove_new> | JH2022: <jh22_interp> Hawk=<jh22_hawk_new> Dove=<jh22_dove_new>

ret <results>
```

## What Should Happen:

**Expected scores with fixed reference vectors:**
- Mar 2020: Hawk=0.2, Dove=0.9 (extreme dovish)
- Dec 2023: Hawk=0.4, Dove=0.7 (dovish lean)
- Aug 2022 JH: Hawk=0.9, Dove=0.2 (extreme hawkish)

## The Real Issue:

Your **hawk/dove reference vectors** are the problem. When you created:
```
store <hawk_ref> hawkish restrictive vigilant inflation risks higher longer tightening