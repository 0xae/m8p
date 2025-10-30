## Advanced Options Analytics & Volatility Surface Analysis
## Analyzes implied vol, skew, term structure, and unusual activity

vdb_instance VOL_REGIMES dim=16 max_elements=500 M=24 ef_construction=300
vdb_instance FLOW_PATTERNS dim=16 max_elements=400 M=20 ef_construction=250
vdb_instance EARNINGS_SIGNALS dim=16 max_elements=300 M=16 ef_construction=200

## VIX term structure states - critical for regime identification
store <vix1> VIX front month at 18 second month at 20 third month at 22 contango steep
llm_embed <vix1> <v1> dim=16
align <v1> 16
vdb_add VOL_REGIMES <v1> Contango steep bullish complacency

store <vix2> VIX front month at 35 second month at 30 third month at 26 backwardation panic
llm_embed <vix2> <v2> dim=16
align <v2> 16
vdb_add VOL_REGIMES <v2> Backwardation panic fear spike

store <vix3> VIX front month at 22 second month at 21.5 third month at 21 flat compression
llm_embed <vix3> <v3> dim=16
align <v3> 16
vdb_add VOL_REGIMES <v3> Flat compression uncertainty transition

store <vix4> VIX front month at 12 second month at 14 third month at 16 low vol grind
llm_embed <vix4> <v4> dim=16
align <v4> 16
vdb_add VOL_REGIMES <v4> Low volatility slow grind complacent

store <vix5> VIX front month at 28 second month at 32 third month at 35 inverted recovery
llm_embed <vix5> <v5> dim=16
align <v5> 16
vdb_add VOL_REGIMES <v5> Inverted curve recovery bottoming

## Options flow patterns - institutional behavior
store <flow1> Large put buying at 5 percent out of money high urgency hedging
llm_embed <flow1> <f1> dim=16
align <f1> 16
vdb_add FLOW_PATTERNS <f1> Put buying OTM hedge protection

store <flow2> Massive call sweeps at the money aggressive bullish positioning
llm_embed <flow2> <f2> dim=16
align <f2> 16
vdb_add FLOW_PATTERNS <f2> Call sweeps ATM bullish aggressive

store <flow3> Put selling combined with call buying synthetic long exposure
llm_embed <flow3> <f3> dim=16
align <f3> 16
vdb_add FLOW_PATTERNS <f3> Put sell call buy synthetic long

store <flow4> Straddle buying at strike suggesting big move expected
llm_embed <flow4> <f4> dim=16
align <f4> 16
vdb_add FLOW_PATTERNS <f4> Straddle buy volatility event expected

store <flow5> Iron condor selling expecting range bound price action
llm_embed <flow5> <f5> dim=16
align <f5> 16
vdb_add FLOW_PATTERNS <f5> Condor sell range bound low vol

store <flow6> Unusual put call ratio spike to 2.5 extreme bearish sentiment
llm_embed <flow6> <f6> dim=16
align <f6> 16
vdb_add FLOW_PATTERNS <f6> Put call ratio spike bearish extreme

store <flow7> Skew steepening puts more expensive crash protection demand
llm_embed <flow7> <f7> dim=16
align <f7> 16
vdb_add FLOW_PATTERNS <f7> Skew steepening downside protection

## Pre-earnings signals
store <earn1> Options volume surges with implied volatility expanding pre earnings
llm_embed <earn1> <e1> dim=16
align <e1> 16
vdb_add EARNINGS_SIGNALS <e1> Vol surge pre earnings anticipation

store <earn2> Unusual call activity suggesting positive earnings surprise leaked
llm_embed <earn2> <e2> dim=16
align <e2> 16
vdb_add EARNINGS_SIGNALS <e2> Call activity leak positive surprise

store <earn3> Put protection increased indicating earnings uncertainty risk
llm_embed <earn3> <e3> dim=16
align <e3> 16
vdb_add EARNINGS_SIGNALS <e3> Put protection earnings uncertainty

store <earn4> Straddle prices imply 8 percent expected move on earnings
llm_embed <earn4> <e4> dim=16
align <e4> 16
vdb_add EARNINGS_SIGNALS <e4> Straddle implies large move

## Current market conditions to analyze
store <current_vix> VIX at 24 second month at 23 third month at 22.5 slight backwardation concern
llm_embed <current_vix> <cvix> dim=16
align <cvix> 16

store <current_flow> Heavy put buying 10 percent OTM with increasing volume
llm_embed <current_flow> <cflow> dim=16
align <cflow> 16

store <current_earnings> Tech stock options show 150 percent IV rank pre announcement
llm_embed <current_earnings> <cearn> dim=16
align <cearn> 16

## Pattern matching against historical regimes
vdb_search VOL_REGIMES <cvix> <regime_match> distance=0.55
llm_detokenize <regime_match> <identified_regime>

vdb_search FLOW_PATTERNS <cflow> <flow_match> distance=0.50
llm_detokenize <flow_match> <flow_interpretation>

vdb_search EARNINGS_SIGNALS <cearn> <earn_match> distance=0.60
llm_detokenize <earn_match> <earnings_outlook>

## Implied volatility surface construction
## Strikes: 90, 95, 100(ATM), 105, 110 | Tenors: 30day, 60day, 90day
mat8 <iv_30day> 0.28 0.24 0.20 0.19 0.18 0 0 0
align <iv_30day> 16
mat8 <iv_60day> 0.26 0.23 0.19 0.18 0.17 0 0 0
align <iv_60day> 16
mat8 <iv_90day> 0.25 0.22 0.18 0.17 0.16 0 0 0
align <iv_90day> 16

## Calculate volatility skew (put side - call side)
f32set <skew_30> 0.28
f32sub <skew_30> 0.18
f32set <skew_60> 0.26
f32sub <skew_60> 0.17
f32set <skew_90> 0.25
f32sub <skew_90> 0.16

## Term structure slope (short - long vol)
f32set <term_slope> 0.20
f32sub <term_slope> 0.18

## VIX futures roll yield calculation
mat8 <vix_futures> 18.5 19.2 20.1 20.8 21.4 22.0 22.5 23.0
align <vix_futures> 16
mat8 <vix_weights> 0.30 0.25 0.20 0.12 0.08 0.03 0.01 0.01
align <vix_weights> 16

matdot <vix_weights> <vix_futures> <weighted_vix>

## Calculate contango/backwardation intensity
f32set <front_vix> 18.5
f32set <second_vix> 19.2
f32sub <second_vix> <front_vix>

## Options Greeks aggregation - portfolio level
mat8 <delta_exposure> 0.45 -0.30 0.65 0.20 -0.15 0.38 -0.22 0.50
align <delta_exposure> 16
mat8 <gamma_exposure> 0.08 0.12 -0.05 0.15 0.09 -0.03 0.11 0.07
align <gamma_exposure> 16
mat8 <vega_exposure> 120 -80 150 95 -60 110 -70 130
align <vega_exposure> 16
mat8 <theta_exposure> -45 -38 -52 -41 -35 -48 -40 -50
align <theta_exposure> 16

mat8 <position_sizes> 100 150 80 120 200 90 110 75
align <position_sizes> 16

## Calculate weighted portfolio Greeks
matdot <position_sizes> <delta_exposure> <portfolio_delta>
matdot <position_sizes> <gamma_exposure> <portfolio_gamma>
matdot <position_sizes> <vega_exposure> <portfolio_vega>
matdot <position_sizes> <theta_exposure> <portfolio_theta>

## Risk reversal analysis (OTM call IV - OTM put IV)
f32set <call_105_iv> 0.19
f32set <put_95_iv> 0.24
f32set <risk_reversal> <call_105_iv>
f32sub <risk_reversal> <put_95_iv>

## Butterfly spread analysis for convexity
f32set <atm_iv> 0.20
f32set <wing_iv_avg> 0.23
f32sub <wing_iv_avg> <atm_iv>

## Put-Call parity violations - arbitrage detection
mat8 <call_prices> 5.20 3.80 2.50 1.40 0.65 0 0 0
align <call_prices> 16
mat8 <put_prices> 0.35 0.95 2.15 3.90 6.20 0 0 0
align <put_prices> 16
mat8 <strikes> 90 95 100 105 110 0 0 0
align <strikes> 16

## Synthetic underlying price from parity: Call - Put + Strike
f32set <synth_price_100> 2.50
f32sub <synth_price_100> 2.15
f32add <synth_price_100> 100

## Volume-weighted IV calculation
mat8 <option_volumes> 1500 3200 8500 2800 1200 0 0 0
align <option_volumes> 16
mat8 <option_ivs> 0.28 0.24 0.20 0.19 0.18 0 0 0
align <option_ivs> 16

matdot <option_volumes> <option_ivs> <volume_weighted_iv_numerator>
matdot <option_volumes> <option_volumes> <total_volume_sq>

## Volatility smile curvature analysis
f32set <left_wing_iv> 0.28
f32set <right_wing_iv> 0.18
f32set <smile_asymmetry> <left_wing_iv>
f32sub <smile_asymmetry> <right_wing_iv>

## Open interest analysis - support/resistance levels
mat8 <call_oi> 1650 1200 4500 980 420 0 0 0
align <call_oi> 16
mat8 <put_oi> 320 980 3200 1800 2400 0 0 0
align <put_oi> 16

## Max pain calculation proxy (highest combined OI)
matadd <call_oi> <put_oi> <total_oi>

## Volatility risk premium (implied - realized)
f32set <implied_vol_30d> 0.20
f32set <realized_vol_30d> 0.15
f32set <vol_risk_premium> <implied_vol_30d>
f32sub <vol_risk_premium> <realized_vol_30d>

## Historical volatility percentiles
mat8 <hv_series> 0.12 0.15 0.18 0.14 0.16 0.20 0.17 0.15
align <hv_series> 16
mat8 <percentile_weights> 0.05 0.10 0.20 0.30 0.20 0.10 0.03 0.02
align <percentile_weights> 16

matdot <hv_series> <percentile_weights> <hv_weighted>

## Correlation between VIX and equity returns
store <corr_analysis> VIX spikes correlate negatively with equity selloffs fear gauge
llm_embed <corr_analysis> <corr> dim=16
align <corr> 16

matcosim <corr> <cvix> <current_fear_level>

## Volatility clustering detection
mat8 <recent_vol_changes> 0.02 0.04 0.06 0.05 0.08 0.12 0.10 0.09
align <recent_vol_changes> 16

matdot <recent_vol_changes> <recent_vol_changes> <vol_clustering_intensity>

## Smart money vs retail flow detection
mat8 <block_trade_flow> 2500 -1800 3200 1500 -2200 2800 -1600 3000
align <block_trade_flow> 16
mat8 <retail_flow> -800 1200 -1500 900 1800 -1100 1400 -1300
align <retail_flow> 16

matcosim <block_trade_flow> <retail_flow> <institutional_retail_divergence>

## Event risk premium extraction
store <event_risk> FOMC meeting next week options pricing in 2 percent move
llm_embed <event_risk> <event> dim=16
vdb_search EARNINGS_SIGNALS <event> <similar_events> distance=0.65
llm_detokenize <similar_events> <event_precedent>

## Volatility mean reversion signal
f32set <current_vix_level> 24.0
f32set <long_term_vix_mean> 18.5
f32set <vix_deviation> <current_vix_level>
f32sub <vix_deviation> <long_term_vix_mean>

## Z-score calculation proxy
f32set <vix_std_dev> 6.5
f32set <vix_zscore> <vix_deviation>
f32mul <vix_zscore> 0.15

## Dispersion trading opportunity (single stock vs index vol)
mat8 <single_stock_ivs> 0.35 0.42 0.38 0.45 0.40 0.36 0.44 0.39
align <single_stock_ivs> 16

matdot <single_stock_ivs> <vix_weights> <avg_single_stock_iv>
f32set <index_iv> 0.24
f32set <dispersion> <avg_single_stock_iv>
f32sub <dispersion> <index_iv>

## Tail hedging efficiency metric
mat8 <hedge_costs> 0.50 0.65 0.45 0.70 0.55 0.48 0.68 0.52
align <hedge_costs> 16
mat8 <hedge_payoffs> 16.5 12.3 7.8 15.2 10.1 8.0 14.5 9.8
align <hedge_payoffs> 16

matdot <hedge_costs> <hedge_costs> <total_hedge_cost>
matdot <hedge_payoffs> <hedge_payoffs> <total_hedge_payoff>

## Volatility carry strategy returns
f32set <short_vol_premium> 0.05
f32set <realized_vol_cost> 0.15
f32set <carry_pnl> <short_vol_premium>
f32sub <carry_pnl> <realized_vol_cost>

## Comprehensive volatility report
store <vol_report> VIX_REGIME: Identified=<identified_regime> Current_level=24.0 Term_slope=<term_slope> Zscore=<vix_zscore> | FLOW: Pattern=<flow_interpretation> Inst_retail_div=<institutional_retail_divergence> | SURFACE: Skew30d=<skew_30> Skew60d=<skew_60> Risk_reversal=<risk_reversal> Smile_asym=<smile_asymmetry> | GREEKS: Delta=<portfolio_delta> Gamma=<portfolio_gamma> Vega=<portfolio_vega> Theta=<portfolio_theta> | RISK_PREMIUM: IV_RV_spread=<vol_risk_premium> Dispersion=<dispersion> | EARNINGS: Outlook=<earnings_outlook> Event_risk=<event_precedent> | STRUCTURE: Weighted_VIX=<weighted_vix> Synthetic_price=<synth_price_100> | CLUSTERING: Vol_cluster_intensity=<vol_clustering_intensity> Fear_level=<current_fear_level> | CARRY: Strategy_PnL=<carry_pnl> Hedge_efficiency_ratio=0.65

ret <vol_report>