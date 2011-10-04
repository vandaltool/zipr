progs=cat.peasoup ls.peasoup bzip2.peasoup

ps_analyze_flags="--step ilr=off --step integertransform=off --step manual_test=on --step concolic=off --step isr=off"

all:	${progs}
	
ls.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${PEASOUP_HOME}/stock_examples/coreutils-7.4.shared/ls ls.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/ls_manual_tests.sh

cat.peasoup:
	${PEASOUP_HOME}/tools/ps_analyze.sh ${PEASOUP_HOME}/stock_examples/coreutils-7.4.shared/cat cat.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/cat_manual_tests.sh

# doesn't yet work in this version
bzip2.peasoup:
	echo `pwd`
	${PEASOUP_HOME}/tools/ps_analyze.sh `which bzip2` bzip2.peasoup ${ps_analyze_flags} --manual_test_script `pwd`/bzip2_manual_tests.sh

clean:
	rm -f ${progs}
	rm -Rf peasoup_executable_directory.*
	${PEASOUP_HOME}/tools/db/drop_my_tables.sh
	${PEASOUP_HOME}/tools/db/pdb_setup.sh
