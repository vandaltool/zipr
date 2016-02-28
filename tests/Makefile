all:
	./test_cmds.sh

clean:
	rm -fr tmp_test_area*

superclean:
	if [ ! "X" = "X"${PGUSER} ]; then sh ../tools/db/drop_my_tables.sh; sh ../tools/db/pdb_setup.sh; fi

