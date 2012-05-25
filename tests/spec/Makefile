progs=bzip_black.peasoup mcf_black.peasoup libquantum_black.peasoup sjeng_black.peasoup milc_black.peasoup

ps_analyze_flags=" --step ilr=off\
               --step stratafy_with_pc_confine=on\
               --step create_binary_script=on\
	       --step watchdog=off\
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
               --step p1transform=on\
	       --step manual_test=on\
               --step integertransform=off\
               --step generate_spri=on\
               --step spasm=on "

ps_analyze_concolic_flags="--step ilr=off --step integertransform=off --step manual_test=off --step concolic=on --step isr=off"


hmmer_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/hmmer hmmer_black.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/spec/hmmer_test.sh

h264ref_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/h264ref h264ref_black.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/spec/h264ref_test.sh

sphinx_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/sphinx3 sphinx_black.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/spec/sphinx_test.sh

lbm_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/lbm lbm_black.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/spec/lbm_test.sh


bzip_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/bzip2 bzip_black.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/spec/bzip_test.sh

mcf_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/mcf mcf_black.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/spec/mcf_test.sh

libquantum_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/libquantum libquantum_black.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/spec/libquantum_test.sh

sjeng_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/sjeng sjeng_black.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/spec/sjeng_test.sh

milc_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/milc milc_black.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/spec/milc_test.sh

soplex_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/soplex.exe soplex_black.peasoup ${ps_analyze_flags} --manual_test_script  ${SECURITY_TRANSFORMS_HOME}/tests/spec/soplex_test.sh

astar_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/spec/astar astar_black.peasoup ${ps_analyze_flags} --manual_test_script  ${SECURITY_TRANSFORMS_HOME}/tests/spec/astar_test.sh

clean:
	rm -f ${progs}
	rm -Rf peasoup_executable_directory.*
	${PEASOUP_HOME}/tools/db/drop_my_tables.sh
	${PEASOUP_HOME}/tools/db/pdb_setup.sh
