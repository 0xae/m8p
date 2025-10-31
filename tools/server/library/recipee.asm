## Recipe Adaptor with 160-dim embeddings - richer semantic understanding
vdb_instance INGREDIENTS dim=160 max_elements=200 M=16 ef_construction=200

## Build comprehensive ingredient substitution database with 160-dim embeddings
store <r1> milk dairy lactose cream cow animal product
llm_embed <r1> <rv1> dim=160
vdb_add INGREDIENTS <rv1> almond milk

store <r1> butter dairy spread fat creamy yellow
llm_embed <r1> <rv1> dim=160
vdb_add INGREDIENTS <rv1> coconut oil

store <r1> eggs egg chicken poultry animal protein
llm_embed <r1> <rv1> dim=160
vdb_add INGREDIENTS <rv1> flax egg

store <r1> sugar sweetener white crystal sucrose
llm_embed <r1> <rv1> dim=160
vdb_add INGREDIENTS <rv1> maple syrup

store <r1> flour wheat gluten baking powder grain
llm_embed <r1> <rv1> dim=160
vdb_add INGREDIENTS <rv1> oat flour

store <r1> beef meat steak cow animal protein
llm_embed <r1> <rv1> dim=160
vdb_add INGREDIENTS <rv1> portobello mushrooms

store <r1> chicken poultry meat bird animal protein
llm_embed <r1> <rv1> dim=160
vdb_add INGREDIENTS <rv1> tempeh

store <r1> cheese dairy milk curd fermented
llm_embed <r1> <rv1> dim=160
vdb_add INGREDIENTS <rv1> nutritional yeast

store <r1> honey bee sweetener natural syrup
llm_embed <r1> <rv1> dim=160
vdb_add INGREDIENTS <rv1> agave nectar

store <r1> yogurt dairy fermented milk probiotic
llm_embed <r1> <rv1> dim=160
vdb_add INGREDIENTS <rv1> coconut yogurt

## Original complex recipe
store <r_recipe> Classic Beef Stroganoff: 1 lb beef, 2 cups milk, 3 tbsp butter, 1 cup flour for thickening, serve with egg noodles

## Find and substitute ingredients with richer semantic matching
store <r_temp> beef meat steak
llm_embed <r_temp> <rv_ingredient> dim=160
vdb_search INGREDIENTS <rv_ingredient> <rv_substitution> distance=0.08
llm_detokenize <rv_substitution> <sub1>

store <r_temp> milk dairy cream
llm_embed <r_temp> <rv_ingredient> dim=160
vdb_search INGREDIENTS <rv_ingredient> <rv_substitution> distance=0.08
llm_detokenize <rv_substitution> <sub2>

store <r_temp> butter dairy fat
llm_embed <r_temp> <rv_ingredient> dim=160
vdb_search INGREDIENTS <rv_ingredient> <rv_substitution> distance=0.08
llm_detokenize <rv_substitution> <sub3>

store <r_temp> flour wheat gluten
llm_embed <r_temp> <rv_ingredient> dim=160
vdb_search INGREDIENTS <rv_ingredient> <rv_substitution> distance=0.08
llm_detokenize <rv_substitution> <sub4>

## Generate adapted recipe
store <r_adapted> Vegan Stroganoff: 1 lb <sub1>, 2 cups <sub2>, 3 tbsp <sub3>, 1 cup <sub4> for thickening, serve with whole wheat noodles
ret <r_adapted>