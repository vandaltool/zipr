progs=mplayer.peasoup

ps_analyze_flags="--step ilr=off --step integertransform=off --step manual_test=on --step concolic=off --step isr=off --step fix_calls=off --step heaprand=off --step pc_confine=off "

mplayer.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/mplayer/mplayer mplayer.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/mplayer_wrapper.sh

clean:
	rm -f ${progs}
	rm -Rf peasoup_executable_directory.*
	${PEASOUP_HOME}/tools/db/drop_my_tables.sh
	${PEASOUP_HOME}/tools/db/pdb_setup.sh