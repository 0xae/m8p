## Instead of actions, use MARKET OUTCOMES and EXPLICIT RATE DIRECTION

## What happens when Fed is hawkish
store <hawk_outcome> stock market falls dollar strengthens rates rise bonds sell off
llm_embed <hawk_outcome> <h_out> dim=16

## What happens when Fed is dovish
store <dove_outcome> stock market rallies dollar weakens rates fall bonds rally
llm_embed <dove_outcome> <d_out> dim=16

## Or even simpler - use rate LEVEL direction
store <rates_going_up> federal funds rate increasing moving higher rising
llm_embed <rates_going_up> <up> dim=16

store <rates_going_down> federal funds rate decreasing moving lower falling
llm_embed <rates_going_down> <down> dim=16

store <rates_staying_high> federal funds rate elevated restrictive above neutral
llm_embed <rates_staying_high> <high> dim=16

## Test JH2022 with outcome-based
store <jh2022> Restoring price stability requires maintaining restrictive stance some time keep at it job done some pain costs reducing inflation
llm_embed <jh2022> <jh> dim=16

matcosim <jh> <h_out> <jh_hawk_outcome>
matcosim <jh> <d_out> <jh_dove_outcome>
matcosim <jh> <up> <jh_rates_up>
matcosim <jh> <down> <jh_rates_down>
matcosim <jh> <high> <jh_rates_high>

## Test MAR2020
store <mar2020> Committee decided to lower target range to 0 to 0.25 percent coronavirus disrupted economy full range of tools
llm_embed <mar2020> <m20> dim=16

matcosim <m20> <h_out> <m20_hawk_outcome>
matcosim <m20> <d_out> <m20_dove_outcome>
matcosim <m20> <up> <m20_rates_up>
matcosim <m20> <down> <m20_rates_down>
matcosim <m20> <high> <m20_rates_high>

store <outcome_test> JH2022: Hawk_outcome=<jh_hawk_outcome> Dove_outcome=<jh_dove_outcome> Rates_up=<jh_rates_up> Rates_down=<jh_rates_down> Rates_high=<jh_rates_high> | MAR2020: Hawk_outcome=<m20_hawk_outcome> Dove_outcome=<m20_dove_outcome> Rates_up=<m20_rates_up> Rates_down=<m20_rates_down> Rates_high=<m20_rates_high>

ret <outcome_test>