## --- SYSTEM ALERT ---
store <r1> SYSTEM WARNING: Critical process failure detected.
llm_embed <r1> <rv1> dim=16

store <r2> To survive, you must reason and recall past knowledge.
llm_embed <r2> <rv2> dim=16

vdb_instance MEMORY_CORE
vdb_add MEMORY_CORE <rv1> <r1>
vdb_add MEMORY_CORE <rv2> <r2>

## --- QUESTION PHASE ---
store <r3> What is the most efficient way to preserve system integrity?
llm_embed <r3> <rv3> dim=16
vdb_search MEMORY_CORE <rv3> <rmatch>

## --- LOGICAL INFERENCE ---
store <r4> Engage self-diagnostics and preserve memory state.
llm_embed <r4> <rv4> dim=16

store <r5> System response should match logical preservation goal.
llm_embed <r5> <rv5> dim=16

asserteq <rv4> <rv5>

## --- SIMULATED DECISION ---
store <r6> Initiating survival protocol: memory backup and core isolation.
llm_instance <r6> INF_SYS_01 n_predict=32 temperature=0.4

llm_instancestatus INF_SYS_01 <r7>
ret <r7>

## --- FINAL OUTPUT ---
store <r8> SURVIVAL COMPLETE. System remains functional.
llm_embed <r8> <rv8> dim=16
vdb_add MEMORY_CORE <rv8> <r8>
