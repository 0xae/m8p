## Multi-Asset Correlation Matrix & Dislocation Detection
## Use actual price/spread levels and calculate proper changes

## S&P 500 levels and changes
mat8 <sp500_levels> 6890.59 6890.89 6875.16 6791.69 6791.69 6791.69 6738.44 6699.40

## Calculate S&P returns (day-over-day changes)
mat8 <sp500_changes> 0.30 -15.73 -83.47 0.00 0.00 -53.25 -39.04 0

## VIX changes (already have this)
mat8 <vix_changes> 3.05 3.99 -3.54 -5.38 -6.99 4.09 -1.97 -12.27

## Treasury 10Y yields (basis points)
mat8 <treasury_yields> 4.25 4.28 4.32 4.35 4.33 4.30 4.28 4.26
## Calculate yield changes
mat8 <treasury_changes> 0.03 0.04 0.03 -0.02 -0.03 -0.02 -0.02 0

## Credit spreads - IG corporate (basis points)
mat8 <credit_spreads> 95 98 102 108 112 115 118 122
## Calculate spread changes (widening = risk increasing)
mat8 <credit_changes> 3 4 6 4 3 3 4 0

## High Yield spreads (basis points)
mat8 <hy_spreads> 325 332 345 358 365 372 380 388
mat8 <hy_changes> 7 13 13 7 7 8 8 0

## Gold prices ($/oz)
mat8 <gold_prices> 2650 2655 2662 2670 2668 2665 2663 2661
mat8 <gold_changes> 5 7 8 -2 -3 -2 -2 0

## Dollar Index (DXY)
mat8 <dollar_levels> 106.2 106.5 106.8 107.1 107.0 106.8 106.6 106.5
mat8 <dollar_changes> 0.3 0.3 0.3 -0.1 -0.2 -0.2 -0.1 0

## Crude Oil (WTI $/barrel)
mat8 <oil_prices> 72.5 71.8 70.2 69.5 68.8 68.2 67.9 67.5
mat8 <oil_changes> -0.7 -1.6 -0.7 -0.7 -0.6 -0.3 -0.4 0

## Copper prices ($/lb - industrial bellwether)
mat8 <copper_prices> 4.15 4.12 4.08 4.05 4.03 4.01 3.99 3.97
mat8 <copper_changes> -0.03 -0.04 -0.03 -0.02 -0.02 -0.02 -0.02 0

## Bitcoin (as risk-on proxy)
mat8 <bitcoin_prices> 98500 98200 97100 96200 95800 95300 94900 94500
mat8 <bitcoin_changes> -300 -1100 -900 -400 -500 -400 -400 0

## COMPREHENSIVE CORRELATION MATRIX (16 dim calculations)

## Row 1: S&P vs Everything
matcosim <sp500_changes> <vix_changes> <sp_vix>
matcosim <sp500_changes> <treasury_changes> <sp_treasury>
matcosim <sp500_changes> <credit_changes> <sp_credit>
matcosim <sp500_changes> <hy_changes> <sp_hy>
matcosim <sp500_changes> <gold_changes> <sp_gold>
matcosim <sp500_changes> <dollar_changes> <sp_dollar>
matcosim <sp500_changes> <oil_changes> <sp_oil>
matcosim <sp500_changes> <copper_changes> <sp_copper>
matcosim <sp500_changes> <bitcoin_changes> <sp_btc>

## Row 2: VIX vs Risk Assets
matcosim <vix_changes> <credit_changes> <vix_credit>
matcosim <vix_changes> <hy_changes> <vix_hy>
matcosim <vix_changes> <treasury_changes> <vix_treasury>
matcosim <vix_changes> <gold_changes> <vix_gold>
matcosim <vix_changes> <dollar_changes> <vix_dollar>

## Row 3: Credit Market Internal Correlations
matcosim <credit_changes> <hy_changes> <ig_hy>
matcosim <credit_changes> <treasury_changes> <credit_treasury>
matcosim <credit_changes> <gold_changes> <credit_gold>

## Row 4: Safe Haven Correlations
matcosim <treasury_changes> <gold_changes> <treasury_gold>
matcosim <treasury_changes> <dollar_changes> <treasury_dollar>
matcosim <gold_changes> <dollar_changes> <gold_dollar>

## Row 5: Risk-On/Risk-Off Indicators
matcosim <oil_changes> <copper_changes> <oil_copper>
matcosim <bitcoin_changes> <sp500_changes> <btc_sp>
matcosim <copper_changes> <credit_changes> <copper_credit>

## EXPECTED vs ACTUAL CORRELATION ANALYSIS (using f32 properly)

## Expected correlations in normal markets
f32set <expected_sp_vix> -0.75
f32set <expected_vix_credit> 0.70
f32set <expected_sp_treasury> -0.40
f32set <expected_credit_hy> 0.90
f32set <expected_treasury_gold> 0.60
f32set <expected_sp_gold> -0.20
f32set <expected_oil_copper> 0.75

## Calculate divergences (how broken are relationships?)
f32set <div_sp_vix> <expected_sp_vix>
f32sub <div_sp_vix> <sp_vix>

f32set <div_vix_credit> <expected_vix_credit>
f32sub <div_vix_credit> <vix_credit>

f32set <div_sp_treasury> <expected_sp_treasury>
f32sub <div_sp_treasury> <sp_treasury>

f32set <div_credit_hy> <expected_credit_hy>
f32sub <div_credit_hy> <ig_hy>

f32set <div_treasury_gold> <expected_treasury_gold>
f32sub <div_treasury_gold> <treasury_gold>

f32set <div_oil_copper> <expected_oil_copper>
f32sub <div_oil_copper> <oil_copper>

## STRESS INDICATORS (using matrix ops properly)

## Credit stress: IG and HY both widening significantly
matadd <credit_changes> <hy_changes> <total_credit_stress>
matdot <total_credit_stress> <total_credit_stress> <credit_stress_magnitude>

## Flight to safety: Treasury + Gold both bid
matadd <treasury_changes> <gold_changes> <safe_haven_flows>
matdot <safe_haven_flows> <safe_haven_flows> <safety_demand>

## Growth concerns: Oil + Copper both falling
matadd <oil_changes> <copper_changes> <growth_proxy>
matdot <growth_proxy> <growth_proxy> <growth_concern_magnitude>

## DISLOCATION SEVERITY SCORING (using f32 properly)

## Z-scores for correlation breaks
f32set <sp_vix_zscore> <div_sp_vix>
f32mul <sp_vix_zscore> 3.33  ## Assume std dev of 0.3

f32set <vix_credit_zscore> <div_vix_credit>
f32mul <vix_credit_zscore> 2.5

## ARBITRAGE OPPORTUNITIES (f32 only)

## VIX-Credit arbitrage (VIX too low vs credit)
f32set <vix_credit_arb> <vix_credit>
f32sub <vix_credit_arb> <expected_vix_credit>
f32mul <vix_credit_arb> -1.0  ## Invert so positive = opportunity

## Treasury-Gold arbitrage (if diverging in flight to safety)
f32set <treasury_gold_arb> <treasury_gold>
f32sub <treasury_gold_arb> <expected_treasury_gold>

## Equity-Credit arbitrage (credit pricing more risk than stocks)
matcosim <sp500_changes> <credit_changes> <equity_credit_link>
f32set <equity_credit_arb> -0.60  ## Expected negative
f32sub <equity_credit_arb> <equity_credit_link>

## GENERATE COMPREHENSIVE REPORT
store <correlation_matrix> SP500: vs_VIX=<sp_vix> vs_Treasury=<sp_treasury> vs_Credit=<sp_credit> vs_HY=<sp_hy> vs_Gold=<sp_gold> vs_Dollar=<sp_dollar> vs_Oil=<sp_oil> vs_Copper=<sp_copper> vs_BTC=<sp_btc> | VIX: vs_Credit=<vix_credit> vs_HY=<vix_hy> vs_Treasury=<vix_treasury> vs_Gold=<vix_gold> vs_Dollar=<vix_dollar> | CREDIT: IG_HY=<ig_hy> vs_Treasury=<credit_treasury> vs_Gold=<credit_gold> | SAFE_HAVENS: Treasury_Gold=<treasury_gold> Treasury_Dollar=<treasury_dollar> Gold_Dollar=<gold_dollar> | GROWTH: Oil_Copper=<oil_copper> BTC_SP=<btc_sp> Copper_Credit=<copper_credit>

store <divergence_analysis> BREAKS: SP_VIX=<div_sp_vix> VIX_Credit=<div_vix_credit> SP_Treasury=<div_sp_treasury> Credit_HY=<div_credit_hy> Treasury_Gold=<div_treasury_gold> Oil_Copper=<div_oil_copper> | ZSCORES: SP_VIX_z=<sp_vix_zscore> VIX_Credit_z=<vix_credit_zscore>

store <stress_metrics> Credit_stress=<credit_stress_magnitude> Safety_demand=<safety_demand> Growth_concern=<growth_concern_magnitude> | ARBITRAGE: VIX_Credit_opp=<vix_credit_arb> Treasury_Gold_opp=<treasury_gold_arb> Equity_Credit_opp=<equity_credit_arb>

ret <correlation_matrix> <divergence_analysis> <stress_metrics>