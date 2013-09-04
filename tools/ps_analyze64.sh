#!/bin/sh 

$PEASOUP_HOME/tools/ps_analyze.sh $* 	\
	--step meds_static=off 		\
	--step concolic=off 		\
	--step p1transform=off 		\
	--step integertransform=off 	\
	--step preLoaded_ILR1=off 	\
	--step preLoaded_ILR2=off 	\
	--step fast_annot=off 		\
	--step ibtc=off 		\
	--step sieve=off 		\
	--step return_cache=off	 	\
	--step partial_inlining=off 	\
	--step watchdog=off 		\
	--step signconv_func_monitor=off\
	--step rekey=off	 	\




verify_peasoup_dir()
{
  	if [ ! -f "$1/ps_run.sh" ]; then
    		echo "Cannot valid peasoup directory"
		exit 1
  	fi
}

verify_peasoup_app()
{
  	grep ps_run $1   > /dev/null 2>&1
  	if [ ! $? -eq 0 ]; then
    		echo "$1 not a valid peasoup program"
		exit 1
  	fi
}



outfile=$2
verify_peasoup_app $outfile
peasoup_dir=`grep ps_run $outfile | cut -d' ' -f3`
verify_peasoup_dir $peasoup_dir

cat $peasoup_dir/ps_run.sh|grep -v STRATA_ANNOT_FILE > $peasoup_dir/ps_run2.sh
mv $peasoup_dir/ps_run2.sh $peasoup_dir/ps_run.sh
chmod +x $peasoup_dir/ps_run.sh

