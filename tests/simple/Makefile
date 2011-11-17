progs=discrete_xform.peasoup integration_simp.peasoup sieve.peasoup knapsack.peasoup dumbledore_cmd_nop.peasoup dumbledore_cmd_O3.peasoup quick_sort.peasoup hanoi.peasoup merge_sort.peasoup k1.peasoup banner_nop.peasoup banner_O3.peasoup banner_O3_concolic.peasoup cal_nop.peasoup cal_O3.peasoup cal_O3_concolic.peasoup print_file_concolic.peasoup cal_nop_concolic.peasoup sudoku_black.peasoup

ps_analyze_flags="--step ilr=off --step integertransform=off --step manual_test=on --step concolic=off --step isr=off"

ps_analyze_concolic_flags="--step ilr=on --step integertransform=on --step manual_test=off --step concolic=on --step isr=off"

discrete_xform.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/discrete_fourier_transform.exe discrete_xform.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/discrete_xform_test.sh

integration_simp.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/integration_simp.exe integration_simp.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/integration_simp_test.sh

sieve.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/sieve.exe sieve.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/sieve_test.sh

sieve_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/sieve.exe sieve.peasoup ${ps_analyze_concolic_flags}

knapsack.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/knapsack_problem.exe knapsack.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/knapsack_test.sh

knapsack_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/knapsack_problem.exe knapsack_concolic.peasoup ${ps_analyze_concolic_flags}


dumbledore_cmd_nop.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/dumbledore_cmd_nop.exe dumbledore_cmd_nop.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/dumbledore_test.sh

dumbledore_cmd_nop_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/dumbledore_cmd_nop.exe dumbledore_cmd_nop.peasoup ${ps_analyze_concolic_flags} 


dumbledore_cmd_O3.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/dumbledore_cmd_O3.exe dumbledore_cmd_O3.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/dumbledore_test.sh

sudoku_black.peasoup:
	g++  sudoku.cpp -o sudoku.exe
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/sudoku.exe sudoku_black.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/sudoku_test.sh


dumbledore_cmd_O3_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/dumbledore_cmd_O3.exe dumbledore_cmd_O3.peasoup ${ps_analyze_concolic_flags}

factor_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh /usr/bin/factor factor_concolic.peasoup ${ps_analyze_concolic_flags}

quick_sort.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/quick_sort.exe quick_sort.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/quick_sort_test.sh

quick_sort_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/quick_sort.exe quick_sort.peasoup ${ps_analyze_concolic_flags} 


hanoi.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/hanoi.exe hanoi.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/hanoi_test.sh

hanoi_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/hanoi.exe hanoi_concolic.peasoup ${ps_analyze_concolic_flags}

merge_sort.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/merge_sort.exe merge_sort.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/merge_sort_test.sh

merge_sort_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/merge_sort.exe merge_sort.peasoup ${ps_analyze_concolic_flags} 
k1.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/k1.exe k1.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/k1_test.sh

banner_nop.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/banner_nop.exe banner_nop.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/banner_test.sh

banner_O3.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/banner_O3.exe banner_O3.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/banner_test.sh

banner_O3_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/banner_O3.exe banner_O3_concolic.peasoup ${ps_analyze_concolic_flags}

banner_nop_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/banner_nop.exe banner_nop_concolic.peasoup ${ps_analyze_concolic_flags}

cal_O3.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/cal_O3.exe cal_O3.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/cal_test.sh

cal_O3_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/cal_O3.exe cal_O3_concolic.peasoup ${ps_analyze_concolic_flags}

cal_nop_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/cal_nop.exe cal_nop_concolic.peasoup ${ps_analyze_concolic_flags}

cal_nop.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/cal_nop.exe cal_nop.peasoup ${ps_analyze_flags} --manual_test_script ${SECURITY_TRANSFORMS_HOME}/tests/simple/cal_test.sh

print_file2_concolic.peasoup:
	g++ print_file2.cpp -o print_file2.exe
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/print_file2.exe print_file2_concolic.peasoup ${ps_analyze_concolic_flags}

print_file1_concolic.peasoup:
	g++ print_file1.cpp -o print_file1.exe
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/simple/print_file1.exe print_file1_concolic.peasoup ${ps_analyze_concolic_flags}


clean:
	rm -f ${progs}
	rm -Rf peasoup_executable_directory.*
	${PEASOUP_HOME}/tools/db/drop_my_tables.sh
	${PEASOUP_HOME}/tools/db/pdb_setup.sh