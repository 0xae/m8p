## Advanced Macro Event & Geopolitical Risk Analysis
## Decodes Fed speak, tracks geopolitical tensions, and monitors economic surprises

vdb_instance FED_LANGUAGE dim=16 max_elements=600 M=24 ef_construction=300
vdb_instance GEOPOLITICAL_EVENTS dim=16 max_elements=500 M=24 ef_construction=300
vdb_instance ECONOMIC_SURPRISES dim=16 max_elements=400 M=20 ef_construction=250
vdb_instance POLICY_SHIFTS dim=16 max_elements=400 M=20 ef_construction=250
vdb_instance CRISIS_PATTERNS dim=16 max_elements=300 M=16 ef_construction=200

## Fed speak decoder - every word matters
store <fed1> Fed Chair signals data dependent approach monitoring inflation closely
llm_embed <fed1> <f1> dim=16
align <f1> 16
vdb_add FED_LANGUAGE <f1> Data dependent monitor inflation hawkish

store <fed2> Committee comfortable with current policy stance patient approach warranted
llm_embed <fed2> <f2> dim=16
align <f2> 16
vdb_add FED_LANGUAGE <f2> Comfortable patient neutral hold steady

store <fed3> Significant progress on inflation but premature to declare victory
llm_embed <fed3> <f3> dim=16
align <f3> 16
vdb_add FED_LANGUAGE <f3> Progress inflation premature cautious dovish tilt

store <fed4> Balance sheet runoff continues at measured pace no changes anticipated
llm_embed <fed4> <f4> dim=16
align <f4> 16
vdb_add FED_LANGUAGE <f4> QT runoff measured neutral balance sheet

store <fed5> Upside risks to inflation require vigilance may need restrictive stance longer
llm_embed <fed5> <f5> dim=16
align <f5> 16
vdb_add FED_LANGUAGE <f5> Upside risks vigilance restrictive hawkish higher longer

store <fed6> Labor market remains robust but showing signs of gradual cooling
llm_embed <fed6> <f6> dim=16
align <f6> 16
vdb_add FED_LANGUAGE <f6> Labor robust cooling gradual balanced

store <fed7> Financial conditions have tightened considerably weighing on outlook
llm_embed <fed7> <f7> dim=16
align <f7> 16
vdb_add FED_LANGUAGE <f7> Tightened considerably weighing dovish easing signal

store <fed8> Committee prepared to adjust policy stance as appropriate based on data
llm_embed <fed8> <f8> dim=16
align <f8> 16
vdb_add FED_LANGUAGE <f8> Adjust appropriate data optionality flexible

## Geopolitical risk events
store <geo1> Trade tensions escalate with new tariffs announced supply chains disrupted
llm_embed <geo1> <g1> dim=16
align <g1> 16
vdb_add GEOPOLITICAL_EVENTS <g1> Trade tariffs escalate supply disruption

store <geo2> Military conflict in Middle East oil shipping routes threatened
llm_embed <geo2> <g2> dim=16
align <g2> 16
vdb_add GEOPOLITICAL_EVENTS <g2> Military conflict Middle East oil shipping

store <geo3> Currency intervention by major central bank signals competitive devaluation
llm_embed <geo3> <g3> dim=16
align <g3> 16
vdb_add GEOPOLITICAL_EVENTS <g3> Currency intervention devaluation competitive

store <geo4> Diplomatic breakthrough reduces regional tensions de-escalation progress
llm_embed <geo4> <g4> dim=16
align <g4> 16
vdb_add GEOPOLITICAL_EVENTS <g4> Diplomatic breakthrough de-escalation positive

store <geo5> Sanctions imposed on major economy global trade flows redirecting
llm_embed <geo5> <g5> dim=16
align <g5> 16
vdb_add GEOPOLITICAL_EVENTS <g5> Sanctions economy trade redirect impact

store <geo6> Political instability in emerging market capital flight accelerates
llm_embed <geo6> <g6> dim=16
align <g6> 16
vdb_add GEOPOLITICAL_EVENTS <g6> Political instability EM capital flight

store <geo7> Strategic alliance formed shifting balance of power geopolitical realignment
llm_embed <geo7> <g7> dim=16
align <g7> 16
vdb_add GEOPOLITICAL_EVENTS <g7> Alliance formed power shift realignment

store <geo8> Energy infrastructure sabotage raises security concerns supply shock risk
llm_embed <geo8> <g8> dim=16
align <g8> 16
vdb_add GEOPOLITICAL_EVENTS <g8> Infrastructure sabotage energy security shock

## Economic surprise indices - actual vs expected
store <econ1> NFP beats expectations by 150k jobs stronger than forecast labor demand
llm_embed <econ1> <e1> dim=16
align <e1> 16
vdb_add ECONOMIC_SURPRISES <e1> NFP beat strong labor positive surprise

store <econ2> CPI comes in hot 0.4 percent above consensus inflation persists
llm_embed <econ2> <e2> dim=16
align <e2> 16
vdb_add ECONOMIC_SURPRISES <e2> CPI hot above consensus inflation negative

store <econ3> GDP growth disappoints at 1.2 percent below 2.0 forecast weakness
llm_embed <econ3> <e3> dim=16
align <e3> 16
vdb_add ECONOMIC_SURPRISES <e3> GDP disappoints below forecast weak negative

store <econ4> Retail sales surge past estimates consumer spending resilient
llm_embed <econ4> <e4> dim=16
align <e4> 16
vdb_add ECONOMIC_SURPRISES <e4> Retail sales surge consumer strong positive

store <econ5> Manufacturing PMI contracts unexpectedly factory activity slowing
llm_embed <econ5> <e5> dim=16
align <e5> 16
vdb_add ECONOMIC_SURPRISES <e5> PMI contracts unexpected manufacturing weak

store <econ6> Jobless claims drop sharply signaling tight labor market persists
llm_embed <econ6> <e6> dim=16
align <e6> 16
vdb_add ECONOMIC_SURPRISES <e6> Claims drop tight labor persistent positive

store <econ7> Housing starts collapse below expectations construction sector weakens
llm_embed <econ7> <e7> dim=16
align <e7> 16
vdb_add ECONOMIC_SURPRISES <e7> Housing collapse below construction weak

store <econ8> Consumer confidence index spikes optimism returning sentiment improves
llm_embed <econ8> <e8> dim=16
align <e8> 16
vdb_add ECONOMIC_SURPRISES <e8> Confidence spike optimism sentiment positive

## Policy shift patterns
store <pol1> Central bank pivots from tightening to pause after persistent weakness
llm_embed <pol1> <p1> dim=16
align <p1> 16
vdb_add POLICY_SHIFTS <p1> Pivot pause tightening dovish shift

store <pol2> Fiscal stimulus announced infrastructure spending government support
llm_embed <pol2> <p2> dim=16
align <p2> 16
vdb_add POLICY_SHIFTS <p2> Fiscal stimulus infrastructure spending support

store <pol3> Regulatory crackdown intensifies compliance costs rising business uncertainty
llm_embed <pol3> <p3> dim=16
align <p3> 16
vdb_add POLICY_SHIFTS <p3> Regulatory crackdown compliance uncertainty

store <pol4> Tax reform passed corporate rates reduced earnings boost expected
llm_embed <pol4> <p4> dim=16
align <p4> 16
vdb_add POLICY_SHIFTS <p4> Tax reform corporate rates earnings boost

## Crisis pattern recognition
store <crisis1> Banking sector stress spreads contagion concerns deposit flight
llm_embed <crisis1> <cr1> dim=16
align <cr1> 16
vdb_add CRISIS_PATTERNS <cr1> Banking stress contagion deposits systemic

store <crisis2> Sovereign debt crisis erupts default risk spreads widen sharply
llm_embed <crisis2> <cr2> dim=16
align <cr2> 16
vdb_add CRISIS_PATTERNS <cr2> Sovereign debt default spreads crisis

store <crisis3> Currency crisis triggers capital controls exchange rate collapse
llm_embed <crisis3> <cr3> dim=16
align <cr3> 16
vdb_add CRISIS_PATTERNS <cr3> Currency crisis capital controls collapse

store <crisis4> Liquidity freeze in credit markets funding stress emerges
llm_embed <crisis4> <cr4> dim=16
align <cr4> 16
vdb_add CRISIS_PATTERNS <cr4> Liquidity freeze credit funding stress

## Current events to analyze
store <current_fed> Fed officials emphasize need for sustained restrictive policy inflation stubborn
llm_embed <current_fed> <cf> dim=16
align <cf> 16

store <current_geo> Tensions escalate in South China Sea naval exercises increase frequency
llm_embed <current_geo> <cg> dim=16
align <cg> 16

store <current_econ> Jobs report shows 220k additions unemployment ticks up wage growth moderates
llm_embed <current_econ> <ce> dim=16
align <ce> 16

store <current_policy> Administration proposes major infrastructure bill debt ceiling debate heats up
llm_embed <current_policy> <cp> dim=16
align <cp> 16

store <fed_cut_var1> Lowering policy rate by quarter point to support economy
llm_embed <fed_cut_var1> <fcv1> dim=16
align <fcv1> 16
vdb_add FED_LANGUAGE <fcv1> Rate cut 25bp easing dovish accommodation

store <fed_cut_var2> Reducing federal funds rate 25 basis points accommodation
llm_embed <fed_cut_var2> <fcv2> dim=16
align <fcv2> 16
vdb_add FED_LANGUAGE <fcv2> Rate cut 25bp easing dovish accommodation

store <fed_cut_var3> Taking overnight lending rate down quarter percentage point
llm_embed <fed_cut_var3> <fcv3> dim=16
align <fcv3> 16
vdb_add FED_LANGUAGE <fcv3> Rate cut 25bp easing dovish accommodation

## Add actual rate cut language
store <fed_cut1> Federal Reserve cuts rates by 25 basis points citing economic headwinds
llm_embed <fed_cut1> <fc1> dim=16
align <fc1> 16
vdb_add FED_LANGUAGE <fc1> Rate cut 25bp easing dovish accommodation

store <fed_cut2> Quarter point reduction in federal funds rate supporting growth
llm_embed <fed_cut2> <fc2> dim=16
align <fc2> 16
vdb_add FED_LANGUAGE <fc2> Quarter point reduction funds rate growth

store <fed_hike1> Federal Reserve raises rates 25 basis points to combat inflation
llm_embed <fed_hike1> <fh1> dim=16
align <fh1> 16
vdb_add FED_LANGUAGE <fh1> Rate hike 25bp tightening hawkish restrictive

## Conditional/forward-looking statements
store <fed_cond1> Maintaining current rates but prepared to cut if data warrants accommodation
llm_embed <fed_cond1> <fcond1> dim=16
align <fcond1> 16
vdb_add FED_LANGUAGE <fcond1> Hold current dovish lean future cuts conditional

store <fed_cond2> No rate change today but forward guidance shifts more hawkish
llm_embed <fed_cond2> <fcond2> dim=16
align <fcond2> 16
vdb_add FED_LANGUAGE <fcond2> Hold current hawkish lean future hikes warning

store <fed_cond3> Policy unchanged but committee signals openness to easing if needed
llm_embed <fed_cond3> <fcond3> dim=16
align <fcond3> 16
vdb_add FED_LANGUAGE <fcond3> Pause dovish optionality insurance cuts ready

## Uncertainty/division statements
store <fed_div1> Committee divided on policy path significant disagreement dot plot dispersed
llm_embed <fed_div1> <fdiv1> dim=16
align <fdiv1> 16
vdb_add FED_LANGUAGE <fdiv1> Division uncertainty wide range no consensus unclear

store <fed_div2> Split views among members reflects data dependence high uncertainty
llm_embed <fed_div2> <fdiv2> dim=16
align <fdiv2> 16
vdb_add FED_LANGUAGE <fdiv2> Split divided data dependent uncertain mixed

## Hawkish hold vs dovish hold
store <fed_hawk_hold> Rates unchanged but inflation vigilance requires restrictive stance longer
llm_embed <fed_hawk_hold> <fhh> dim=16
align <fhh> 16
vdb_add FED_LANGUAGE <fhh> Hold hawkish higher longer restrictive vigilant

store <fed_dove_hold> Policy steady but watching for signs may need support accommodation
llm_embed <fed_dove_hold> <fdh> dim=16
align <fdh> 16
vdb_add FED_LANGUAGE <fdh> Hold dovish watching support ready easing bias

## Add missing critical Fed patterns

## Emergency dovish language
store <emergency1> Committee lowered target range to zero to 0.25 percent prepared to use full range of tools
llm_embed <emergency1> <em1> dim=16
align <em1> 16
vdb_add FED_LANGUAGE <em1> Emergency cut zero extreme dovish crisis QE unlimited

store <emergency2> Coronavirus outbreak disrupted economy support flow of credit
llm_embed <emergency2> <em2> dim=16
align <em2> 16
vdb_add FED_LANGUAGE <em2> Crisis pandemic emergency support dovish intervention

## Dovish pivot with hold
store <pivot1> Maintaining rates but prepared to adjust lower if risks emerge three cuts expected
llm_embed <pivot1> <piv1> dim=16
align <piv1> 16
vdb_add FED_LANGUAGE <piv1> Hold dovish pivot cuts coming easing bias ready

store <pivot2> Committee would be prepared to adjust stance lower risks emerged easing upcoming
llm_embed <pivot2> <piv2> dim=16
align <piv2> 16
vdb_add FED_LANGUAGE <piv2> Hold dovish lean future cuts prepared lower

## Ultra-hawkish rhetoric
store <ultra_hawk1> Restoring price stability requires maintaining restrictive stance some pain unfortunate costs keep at it until job done
llm_embed <ultra_hawk1> <uh1> dim=16
align <uh1> 16
vdb_add FED_LANGUAGE <uh1> Ultra hawkish pain restrictive job done higher longer aggressive

store <ultra_hawk2> Historical record cautions against prematurely loosening policy must stay course inflation fight priority
llm_embed <ultra_hawk2> <uh2> dim=16
align <uh2> 16
vdb_add FED_LANGUAGE <uh2> Hawkish no premature easing stay restrictive inflation priority

## Peak hawkish with final hike
store <peak_hawk1> Raising target range to 5.25 to 5.5 percent inflation remains elevated highly attentive to risks
llm_embed <peak_hawk1> <ph1> dim=16
align <ph1> 16
vdb_add FED_LANGUAGE <ph1> Rate hike peak hawkish inflation elevated attentive terminal

## Insurance cuts / mid-cycle adjustment
store <insurance1> Case for more accommodative policy strengthened uncertainties increased act as appropriate sustain expansion
llm_embed <insurance1> <ins1> dim=16
align <ins1> 16
vdb_add FED_LANGUAGE <ins1> Insurance cuts dovish accommodative uncertainties mid-cycle adjustment

## Taper language
store <taper1> Moderation in pace of asset purchases may soon be warranted progress continues QE tapering
llm_embed <taper1> <tap1> dim=16
align <tap1> 16
vdb_add FED_LANGUAGE <tap1> Taper QE reduction asset purchases moderating neutral to hawkish

## Now search again
store <query> The Federal Reserve announced a quarter percentage point or 25 basis point cut to its overnight lending rate
llm_embed <query> <q> dim=16
vdb_search FED_LANGUAGE <q> <result> distance=0.50
llm_detokenize <result> <interpretation>
ret <interpretation>

## Pattern matching across databases
vdb_search FED_LANGUAGE <cf> <fed_signal> distance=0.50
llm_detokenize <fed_signal> <fed_interpretation>

vdb_search GEOPOLITICAL_EVENTS <cg> <geo_precedent> distance=0.55
llm_detokenize <geo_precedent> <geo_interpretation>

vdb_search ECONOMIC_SURPRISES <ce> <econ_context> distance=0.50
llm_detokenize <econ_context> <econ_interpretation>

vdb_search POLICY_SHIFTS <cp> <policy_analog> distance=0.55
llm_detokenize <policy_analog> <policy_interpretation>

## Check for crisis patterns
vdb_search CRISIS_PATTERNS <cf> <crisis_check_fed> distance=0.65
vdb_search CRISIS_PATTERNS <cg> <crisis_check_geo> distance=0.60

## Build Fed hawkish/dovish sentiment spectrum
matadd <f1> <f5> <hawkish_cluster>
matadd <f3> <f7> <dovish_cluster>
matadd <f2> <f8> <neutral_cluster>

## Calculate current Fed stance position
matcosim <cf> <hawkish_cluster> <hawkish_score>
matcosim <cf> <dovish_cluster> <dovish_score>
matcosim <cf> <neutral_cluster> <neutral_score>

## Geopolitical risk aggregation
matadd <g1> <g2> <temp_risk1>
matadd <temp_risk1> <g5> <temp_risk2>
matadd <temp_risk2> <g6> <temp_risk3>
matadd <temp_risk3> <g8> <geopolitical_risk_aggregate>

## Positive geopolitical developments
matadd <g4> <g7> <geopolitical_positive_aggregate>

## Net geopolitical sentiment
matcosim <geopolitical_risk_aggregate> <geopolitical_positive_aggregate> <geo_sentiment_balance>

## Economic surprise momentum
matadd <e1> <e4> <positive_surprises>
matadd <e2> <e3> <negative_surprises>
matadd <positive_surprises> <e6> <temp_pos>
matadd <temp_pos> <e8> <total_positive_surprises>
matadd <negative_surprises> <e5> <temp_neg>
matadd <temp_neg> <e7> <total_negative_surprises>

## Calculate surprise index direction
matcosim <total_positive_surprises> <total_negative_surprises> <surprise_balance>

## Economic growth vs inflation surprises
matcosim <e1> <e2> <growth_inflation_tension>
matcosim <e4> <e2> <consumer_inflation_tension>

## Policy coordination analysis
matcosim <cf> <cp> <monetary_fiscal_alignment>

## Build macro regime vector
matadd <cf> <cg> <temp_macro1>
matadd <temp_macro1> <ce> <temp_macro2>
matadd <temp_macro2> <cp> <macro_regime_vector>

## Historical regime comparison
store <regime_2008> Financial crisis liquidity freeze risk aversion extreme
llm_embed <regime_2008> <r2008> dim=16
store <regime_2020> Pandemic shock fiscal monetary stimulus unprecedented
llm_embed <regime_2020> <r2020> dim=16
store <regime_2018> Rate hikes tightening growth slowdown trade tensions
llm_embed <regime_2018> <r2018> dim=16
store <regime_2013> Taper tantrum Fed communications emerging market stress
llm_embed <regime_2013> <r2013> dim=16

matcosim <macro_regime_vector> <r2008> <similarity_2008>
matcosim <macro_regime_vector> <r2020> <similarity_2020>
matcosim <macro_regime_vector> <r2018> <similarity_2018>
matcosim <macro_regime_vector> <r2013> <similarity_2013>

## Fed dot plot divergence (simulate member disagreement)
mat8 <fed_dots_2024> 5.50 5.25 5.00 5.50 5.75 5.25 5.00 5.50
align <fed_dots_2024> 16
mat8 <fed_dots_2025> 4.50 4.75 5.00 4.25 4.00 4.75 5.00 4.50
align <fed_dots_2025> 16

## Calculate consensus strength (low variance = strong consensus)
matdot <fed_dots_2024> <fed_dots_2024> <dots_2024_variance_proxy>
matdot <fed_dots_2025> <fed_dots_2025> <dots_2025_variance_proxy>

## Rate path expectations
f32set <current_rate> 5.50
f32set <expected_2025> 4.50
f32set <rate_cut_path> <current_rate>
f32sub <rate_cut_path> <expected_2025>

## Market pricing vs Fed guidance gap
f32set <market_implied_cuts> 1.50
f32set <fed_implied_cuts> <rate_cut_path>
f32set <market_fed_gap> <market_implied_cuts>
f32sub <market_fed_gap> <fed_implied_cuts>

## Geopolitical risk scoring by region
mat8 <region_risks> 0.75 0.45 0.60 0.85 0.40 0.55 0.70 0.50
align <region_risks> 16
mat8 <region_weights> 0.25 0.20 0.15 0.15 0.10 0.08 0.05 0.02
align <region_weights> 16

matdot <region_risks> <region_weights> <global_risk_score>

## Trade flow impact estimates
mat8 <trade_disruption_levels> 0.15 0.08 0.25 0.12 0.30 0.10 0.18 0.20
align <trade_disruption_levels> 16
mat8 <trade_importance> 0.30 0.15 0.20 0.12 0.10 0.08 0.03 0.02
align <trade_importance> 16

matdot <trade_disruption_levels> <trade_importance> <trade_impact_score>

## Economic surprise index calculation
mat8 <surprise_magnitudes> 0.12 -0.08 -0.15 0.20 -0.10 0.18 -0.12 0.22
align <surprise_magnitudes> 16
mat8 <data_importance> 0.30 0.25 0.20 0.12 0.08 0.03 0.01 0.01
align <data_importance> 16

matdot <surprise_magnitudes> <data_importance> <economic_surprise_index>

## Policy uncertainty index components
mat8 <uncertainty_factors> 0.65 0.72 0.58 0.80 0.55 0.70 0.63 0.68
align <uncertainty_factors> 16
mat8 <uncertainty_weights> 0.22 0.20 0.18 0.15 0.12 0.08 0.03 0.02
align <uncertainty_weights> 16

matdot <uncertainty_factors> <uncertainty_weights> <policy_uncertainty_index>

## Central bank communication clarity
matcosim <f2> <f8> <fed_consistency>
matcosim <f3> <f5> <fed_policy_confusion>

## Cross-asset risk transmission
store <transmission1> Geopolitical tensions drive flight to quality bonds rally stocks fall
llm_embed <transmission1> <trans1> dim=16
store <transmission2> Fed hawkish pivot strengthens dollar pressures commodities
llm_embed <transmission2> <trans2> dim=16

matcosim <cg> <trans1> <geo_asset_impact>
matcosim <cf> <trans2> <fed_asset_impact>

## Inflation expectations evolution
mat8 <inflation_expectations_series> 2.8 2.9 3.1 3.0 2.9 2.7 2.6 2.5
align <inflation_expectations_series> 16

matdot <inflation_expectations_series> <inflation_expectations_series> <inflation_trend_momentum>

## Real rate calculations
f32set <nominal_rate> 5.50
f32set <inflation_expectation> 2.70
f32set <real_rate> <nominal_rate>
f32sub <real_rate> <inflation_expectation>

## Policy effectiveness scoring
matcosim <f1> <e1> <fed_labor_effectiveness>
matcosim <f5> <e2> <fed_inflation_effectiveness>

## Tail risk indicators
mat8 <tail_risk_proxies> 0.35 0.42 0.38 0.50 0.33 0.45 0.40 0.48
align <tail_risk_proxies> 16

matdot <tail_risk_proxies> <tail_risk_proxies> <systemic_tail_risk>

## Event clustering - multiple risks simultaneously
matcosim <cf> <cg> <fed_geo_correlation>
matcosim <cg> <ce> <geo_econ_correlation>
matcosim <cf> <ce> <fed_econ_correlation>

## Crisis probability estimation
f32set <crisis_threshold> 0.70
f32set <current_crisis_score> 0.0

## Policy reaction function prediction
store <reaction1> When inflation rises Fed typically responds with rate hikes
llm_embed <reaction1> <react1> dim=16
matcosim <ce> <react1> <expected_fed_response>

## Market repricing risk
f32set <current_pricing> <market_implied_cuts>
f32set <fundamental_pricing> <fed_implied_cuts>
f32set <repricing_risk> <current_pricing>
f32sub <repricing_risk> <fundamental_pricing>

## Sentiment regime classification
f32set <risk_on_threshold> 0.60
f32set <current_sentiment> <surprise_balance>

## Generate comprehensive macro report
store <macro_report> FED_STANCE: Interpretation=<fed_interpretation> Hawkish=<hawkish_score> Dovish=<dovish_score> Neutral=<neutral_score> Consistency=<fed_consistency> Market_gap=<market_fed_gap> Real_rate=<real_rate> | GEOPOLITICAL: Precedent=<geo_interpretation> Risk_score=<global_risk_score> Sentiment_balance=<geo_sentiment_balance> Trade_impact=<trade_impact_score> Asset_impact=<geo_asset_impact> | ECONOMIC: Context=<econ_interpretation> Surprise_index=<economic_surprise_index> Balance=<surprise_balance> Growth_inflation_tension=<growth_inflation_tension> | POLICY: Analog=<policy_interpretation> Uncertainty=<policy_uncertainty_index> Mon_fiscal_align=<monetary_fiscal_alignment> | REGIME: 2008_similarity=<similarity_2008> 2020_similarity=<similarity_2020> 2018_similarity=<similarity_2018> 2013_similarity=<similarity_2013> | RISKS: Systemic_tail=<systemic_tail_risk> Fed_geo_corr=<fed_geo_correlation> Repricing_risk=<repricing_risk> | EXPECTATIONS: Rate_path=<rate_cut_path> Inflation_momentum=<inflation_trend_momentum> Fed_effectiveness=<fed_inflation_effectiveness>

ret <macro_report>