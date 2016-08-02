#!/bin/bash

svn_top="daffy diablo_toolchain IdaProServer irdb_transforms irdb_vars peasoup_examples security_transforms strata stratafier zipr zipr_callbacks zipr_install zipr_relax_plugin zipr_scfi_plugin zipr_sdk"
for i in $svn_top
do
	if [ -d "$i" ]; then
		pushd "$i"
		svn diff -r head
		popd
	fi
done

