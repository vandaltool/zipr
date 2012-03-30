progs=cat_nop.peasoup cat_O3.peasoup ls_nop.peasoup ls_O3.peasoup bzip2_nop.peasoup bzip2_O2.peasoup bzip2_O3_black.peasoup  bzip2_O3_white.peasoup ls_O3_black.peasoup rm_O3_black.peasoup tail_O3_black.peasoup rmdir_O3_black.peasoup readlink_O3_black.peasoup pr_O3_black.peasoup mkdir_O3_black.peasoup ln_O3_black.peasoup install_O3_black.peasoup du_O3_black.peasoup df_O3_black.peasoup dd_O3_black.peasoup cp_O3_black.peasoup chown_O3_black.peasoup chmod_O3_black.peasoup chgrp_O3_black.peasoup touch_O3_black.peasoup mv_O3_black.peasoup

ps_analyze_flags="--step ilr=off --step integertransform=off --step manual_test=on --step concolic=off --step isr=off"

ps_analyze_concolic_flags="--step ilr=off --step integertransform=off --step manual_test=off --step concolic=off --step isr=off"

all:	${progs}
	
ls_nop.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/ls_nop.exe ls_nop.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/ls_manual_tests.sh

ls_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/ls_O3.exe ls_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/ls_wrapper.sh

chgrp_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/chgrp_O3.exe chgrp_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/chgrp_wrapper.sh

chmod_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/chmod_O3.exe chmod_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/chmod_wrapper.sh

chown_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/chown_O3.exe chown_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/chown_wrapper.sh

cp_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/cp_O3.exe cp_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/cp_wrapper.sh

dd_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/dd_O3.exe dd_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/dd_wrapper.sh

rm_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/rm_O3.exe rm_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/rm_wrapper.sh

tail_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/tail_O3.exe tail_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/tail_wrapper.sh

rmdir_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/rmdir_O3.exe rmdir_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/rmdir_wrapper.sh

readlink_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/readlink_O3.exe readlink_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/readlink_wrapper.sh

pr_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/pr_O3.exe pr_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/pr_wrapper.sh

mv_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/mv_O3.exe mv_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/mv_wrapper.sh

mkdir_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/mkdir_O3.exe mkdir_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/mkdir_wrapper.sh

ln_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/ln_O3.exe ln_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/ln_wrapper.sh

install_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/install_O3.exe install_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/install_wrapper.sh

du_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/du_O3.exe du_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/du_wrapper.sh

df_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/df_O3.exe df_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/df_wrapper.sh

ls_O3_white.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/ls_O3.exe ls_O3_white.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/ls_manual_tests.sh

touch_O3_black.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/touch_O3.exe touch_O3_black.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/touch_wrapper.sh

ls.peasoup.static:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${PEASOUP_HOME}/stock_examples/coreutils-7.4.static/ls ls.peasoup.static ${ps_analyze_flags} --manual_test_script `pwd`/ls_manual_tests.sh

cat_nop.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/cat_nop.exe cat_nop.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/cat_manual_tests.sh

cat_O3.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/cat_O3.exe cat_O3.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/cat_manual_tests.sh

bzip2_nop.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/bzip2_nop.exe bzip2_nop.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/bzip2_manual_tests.sh

bzip2_O2.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/bzip2_O2.exe bzip2_O2.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/bzip2_manual_tests.sh

bzip2_O3.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/bzip2_O3.exe bzip2_O3.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/bzip2_manual_tests.sh

bzip2_O3_concolic.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${SECURITY_TRANSFORMS_HOME}/tests/coreutils/bzip2_O3.exe bzip2_O3.peasoup ${ps_analyze_concolic_flags}

clean:
	rm -f ${progs}
	rm -Rf peasoup_executable_directory.*
	${PEASOUP_HOME}/tools/db/drop_my_tables.sh
	${PEASOUP_HOME}/tools/db/pdb_setup.sh
