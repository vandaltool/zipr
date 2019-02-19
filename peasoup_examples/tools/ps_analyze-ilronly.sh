#!/bin/sh


TVHEADLESS=1 $PEASOUP_HOME/tools/ps_analyze.sh $* \
		--step ilr=on\
		--step preLoaded_ILR=on\
		--step stratafy_with_pc_confine=on\
		--step create_binary_script=on\
		--step heaprand=off\
		--step concolic=off\
		--step double_free=off\
		--step pc_confine=off\
		--step isr=off\
		--step meds_static=on\
		--step pdb_register=on\
		--step pdb_create_tables=on\
		--step meds2pdb=on\
		--step fill_in_cfg=on\
		--step fill_in_indtargs=on\
		--step clone=on\
		--step fix_calls=on\
		--step p1transform=off\
		--step integertransform=off\
		--step generate_spri=on\
		--step spasm=on\
		--step appfw=off\
		--step watchdog=off\
        	--step signconv_func_monitor=off


