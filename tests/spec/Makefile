progs=soplex_black.peasoup astar_black.peasoup

ps_analyze_flags=" --step ilr=off\
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
               --step spasm=on "

ps_analyze_concolic_flags="--step ilr=on --step integertransform=on --step manual_test=off --step concolic=on --step isr=off"


soplex_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/soplex.exe soplex_black.peasoup ${ps_analyze_flags} --manual_test_script  ${SECURITY_TRANSFORMS_HOME}/tests/spec/soplex_test.sh

astar_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/astar astar_black.peasoup ${ps_analyze_flags} --manual_test_script  ${SECURITY_TRANSFORMS_HOME}/tests/spec/astar_test.sh


clean:
	rm -f ${progs}
	rm -Rf peasoup_executable_directory.*
	${PEASOUP_HOME}/tools/db/drop_my_tables.sh
	${PEASOUP_HOME}/tools/db/pdb_setup.sh