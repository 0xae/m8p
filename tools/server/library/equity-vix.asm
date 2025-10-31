mat8 <equity_returns> 0.02 0.05 -0.03 0.04 -0.01 0.03 -0.02 0.06
mat8 <bond_returns> -0.01 -0.02 0.03 -0.01 0.02 -0.02 0.03 -0.04
mat8 <credit_spreads> 0.08 0.12 -0.05 0.15 0.10 0.18 -0.03 0.20
mat8 <vix_levels> 0.12 0.14 0.18 0.13 0.15 0.11 0.10 0.09
mat8 <gold_returns> 0.01 0.02 0.04 0.01 0.03 0.00 0.02 0.01
mat8 <dollar_returns> 0.03 0.02 -0.01 0.04 0.02 0.05 0.01 0.03
mat8 <commodity_returns> 0.05 0.08 -0.02 0.10 0.06 0.12 0.03 0.09
mat8 <hy_spreads> 0.15 0.20 -0.08 0.25 0.18 0.30 -0.05 0.28

## Cross-asset correlation matrix using 16 dim vectors
matcosim <equity_returns> <bond_returns> <equity_bond_corr>
matcosim <equity_returns> <vix_levels> <equity_vix_corr>
matcosim <credit_spreads> <vix_levels> <credit_vix_corr>
matcosim <bond_returns> <gold_returns> <bond_gold_corr>
matcosim <dollar_returns> <commodity_returns> <dollar_commodity_corr>
matcosim <equity_returns> <gold_returns> <equity_gold_corr>
matcosim <credit_spreads> <hy_spreads> <ig_hy_corr>
matcosim <vix_levels> <hy_spreads> <vix_hy_corr>

## Detect divergences - normally correlated pairs breaking down
f32set <expected_equity_vix> -0.80
f32set <actual_equity_vix> <equity_vix_corr>
f32set <equity_vix_divergence> <expected_equity_vix>
f32sub <equity_vix_divergence> <actual_equity_vix>

f32set <expected_credit_vix> 0.75
f32set <actual_credit_vix> <credit_vix_corr>
f32set <credit_vix_divergence> <expected_credit_vix>
f32sub <credit_vix_divergence> <actual_credit_vix>

store <report> Equity_VIX=<equity_vix_corr>

ret <report>