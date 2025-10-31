## Futures Market Analysis & Prediction Engine
vdb_instance FUTURES_SIGNALS dim=160 max_elements=300 M=16 ef_construction=200
vdb_instance MARKET_REGIMES dim=160 max_elements=200 M=16 ef_construction=200

## Build Futures Signals Database
store <r1> contango futures price higher than spot normal market upward sloping
llm_embed <r1> <rv1> dim=160
vdb_add FUTURES_SIGNALS <rv1> CONTANGO

store <r1> backwardation futures price lower than spot inverted market downward sloping
llm_embed <r1> <rv1> dim=160
vdb_add FUTURES_SIGNALS <rv1> BACKWARDATION

store <r1> roll yield positive contango benefits futures holders
llm_embed <r1> <rv1> dim=160
vdb_add FUTURES_SIGNALS <rv1> POSITIVE_ROLL

store <r1> roll yield negative backwardation costs futures holders
llm_embed <r1> <rv1> dim=160
vdb_add FUTURES_SIGNALS <rv1> NEGATIVE_ROLL

## Build Market Regimes Database
store <r1> trending market strong directional movement momentum persistence
llm_embed <r1> <rv1> dim=160
vdb_add MARKET_REGIMES <rv1> TRENDING

store <r1> ranging market sideways movement support resistance consolidation
llm_embed <r1> <rv1> dim=160
vdb_add MARKET_REGIMES <rv1> RANGING

store <r1> volatile market large price swings uncertainty high VIX
llm_embed <r1> <rv1> dim=160
vdb_add MARKET_REGIMES <rv1> VOLATILE

store <r1> low volatility calm market compressed prices VIX low
llm_embed <r1> <rv1> dim=160
vdb_add MARKET_REGIMES <rv1> CALM

## Analyze Current Market Conditions
store <r_market_data> Crude oil futures in contango with steep forward curve, market showing strong trending behavior with high volatility
llm_embed <r_market_data> <rv_market> dim=160

## Detect Futures Structure
vdb_search FUTURES_SIGNALS <rv_market> <rv_structure> distance=0.15
llm_detokenize <rv_structure> <futures_structure>

## Detect Market Regime
vdb_search MARKET_REGIMES <rv_market> <rv_regime> distance=0.15
llm_detokenize <rv_regime> <market_regime>

## Generate Trading Strategy
store <r_strategy> FUTURES STRATEGY: Structure=<futures_structure>, Regime=<market_regime>
store <r_recommendation> RECOMMENDATION: Consider calendar spreads in contango during trending regimes
ret <r_strategy> <r_recommendation>