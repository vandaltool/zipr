progs=soplex_black.peasoup

ps_analyze_flags="--step ilr=off --step integertransform=off --step manual_test=on --step concolic=off --step isr=off --step pc_confine=off --step heaprand=off --step p1transform=off"

ps_analyze_concolic_flags="--step ilr=on --step integertransform=on --step manual_test=off --step concolic=on --step isr=off"


soplex_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/soplex.exe soplex_black.peasoup ${ps_analyze_flags} --manual_test_script  ${SECURITY_TRANSFORMS_HOME}/tests/spec/soplex_test.sh


clean:
	rm -f ${progs}
	rm -Rf peasoup_executable_directory.*
	${PEASOUP_HOME}/tools/db/drop_my_tables.sh
	${PEASOUP_HOME}/tools/db/pdb_setup.sh