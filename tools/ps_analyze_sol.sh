#!/bin/sh 

$PEASOUP_HOME/tools/ps_analyze.sh $* 	   	\
	-s stratafy_with_pc_confine=off 	\
	-s detect_server=off 		\
	-s watchdog=off 			\
	-s is_so=off 			\
	-s signconv_func_monitor=off 	\
	-s gather_libraries=off 		\
	-s meds_static=off 			\
	-s concolic=off 			\
	-s fill_in_cfg=off 			\
	-s fill_in_indtargs=off 		\
	-s clone=off 			\
	-s fix_calls=off 			\
	-s find_strings=off 		\
	-s appfw=off 			\
	-s determine_program=off 		\
	-s manual_test=off 			\
	-s fast_annot=off 			\
	-s p1transform=off 			\
	-s integertransform=off 		\
	-s twitchertransform=off 		\
	-s ilr=off 				\
	-s generate_spri=off 		\
	-s spasm=off 			\
	-s fast_spri=off 			\
	-s preLoaded_ILR1=off 		\
	-s preLoaded_ILR2=off 		\
	\
	-s controlled_exit=off 		\
	\
	-s rekey=on 			\
	-s add_confinement_section=on 	\
	-s pc_confine=on 		 \
	-s create_binary_script=on 	\
	-s isr=on 				\
	-s heaprand=on 			\
	-s double_free=on 			\
	\
	-s pdb_register=on 		\
