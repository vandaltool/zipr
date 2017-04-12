#!/bin/bash




pgms="e_c17_p1 e_c17_p2 e_c17_p3 e_c17_p4  e_c17_p6 except5"
opt_lvls="-O0 -O1 -O2 -O3 -Os -Og"


build_pgm()
{
	file=$1
	shift
	comp_opts=$*

	if [ $file = "except5"  ]; then 
		gnat compile $comp_opts except5.adb  
		gnat compile $comp_opts stuff.adb
		gnat bind $file
		gnat link $file
		mv $file $file.exe
	else
		echo "inputfiles are: $file"
		gnat compile $comp_opts $file.ada
		gnat bind $file
		gnat link $file
		mv $file $file.exe
	fi

}

testit()
{
	$1 > good.txt 2>&1 
	$2 > xform.txt 2>&1 

	if ! cmp good.txt xform.txt; then
		echo "****************************************"
		echo "Test failed"
		diff good.txt xform.txt
		echo "****************************************"
		exit 1
	fi
}

doit()
{
	pgm=$1
	options="$2"
	ps_opts="$3"
	build_pgm  $1 $2
	$PSZ $pgm.exe $pgm.prot $ps_opts

	testit $pgm.exe $pgm.prot

	
}


main()
{
	for pgm in $pgms
	do
		for opt_lvl in $opt_lvls
		do
			doit $pgm "$opt_lvl" "" 
			doit $pgm "$opt_lvl -fPIC " "" 
			doit $pgm "$opt_lvl -fPIC -pie " "" 

			doit $pgm "$opt_lvl " "--step-option fill_in_indtargs:--split-eh-frame" 
			doit $pgm "$opt_lvl -fPIC" "--step-option fill_in_indtargs:--split-eh-frame" 
			doit $pgm "$opt_lvl -fPIC -pie" "--step-option fill_in_indtargs:--split-eh-frame" 
		done
	done
}



main "$@"
