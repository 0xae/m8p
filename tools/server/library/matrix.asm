mat8 <iv_30day> 0.28 0.24 0.20 0.19 0.18 0 0 0
mat8 <iv_60day> 0.26 0.23 0.19 0.18 0.17 0 0 0
mat8 <iv_90day> 0.25 0.22 0.18 0.17 0.16 0 0 0
mat8 <vix_futures> 18.5 19.2 20.1 20.8 21.4 22.0 22.5 23.0
mat8 <vix_weights> 0.30 0.25 0.20 0.12 0.08 0.03 0.01 0.01
matdot <vix_weights> <vix_futures> <weighted_vix>

mat8 <delta_exposure> 0.45 -0.30 0.65 0.20 -0.15 0.38 -0.22 0.50
mat8 <gamma_exposure> 0.08 0.12 -0.05 0.15 0.09 -0.03 0.11 0.07
mat8 <vega_exposure> 120 -80 150 95 -60 110 -70 130
mat8 <theta_exposure> -45 -38 -52 -41 -35 -48 -40 -50

mat8 <position_sizes> 100 150 80 120 200 90 110 75

matdot <position_sizes> <delta_exposure> <portfolio_delta>
matdot <position_sizes> <gamma_exposure> <portfolio_gamma>
matdot <position_sizes> <vega_exposure> <portfolio_vega>
matdot <position_sizes> <theta_exposure> <portfolio_theta>

mat8 <call_prices> 5.20 3.80 2.50 1.40 0.65 0 0 0
mat8 <put_prices> 0.35 0.95 2.15 3.90 6.20 0 0 0
mat8 <strikes> 90 95 100 105 110 0 0 0

mat8 <option_volumes> 1500 3200 8500 2800 1200 0 0 0
mat8 <option_ivs> 0.28 0.24 0.20 0.19 0.18 0 0 0

matdot <option_volumes> <option_ivs> <volume_weighted_iv_numerator>
matdot <option_volumes> <option_volumes> <total_volume_sq>

mat8 <call_oi> 1650 1200 4500 980 420 0 0 0
mat8 <put_oi> 320 980 3200 1800 2400 0 0 0
matadd <call_oi> <put_oi> <total_oi>

mat8 <hv_series> 0.12 0.15 0.18 0.14 0.16 0.20 0.17 0.15
mat8 <percentile_weights> 0.05 0.10 0.20 0.30 0.20 0.10 0.03 0.02

matdot <hv_series> <percentile_weights> <hv_weighted>

mat8 <recent_vol_changes> 0.02 0.04 0.06 0.05 0.08 0.12 0.10 0.09
matdot <recent_vol_changes> <recent_vol_changes> <vol_clustering_intensity>

mat8 <block_trade_flow> 2500 -1800 3200 1500 -2200 2800 -1600 3000
mat8 <retail_flow> -800 1200 -1500 900 1800 -1100 1400 -1300

matcosim <block_trade_flow> <retail_flow> <institutional_retail_divergence>

store <report> institutional_retail_divergence=<institutional_retail_divergence>, retail_flow=<retail_flow>,block_trade_flow=<block_trade_flow>, total_oi=<total_oi>, weighted_vix=<weighted_vix>