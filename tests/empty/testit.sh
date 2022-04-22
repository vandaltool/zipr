


for bench in hello.c empty.c 
do
	for archflag in '-m32' '-m64' 
	do
		for cfflag in '-fcf-protection=none' '-fcf-protection=full'
		do
			(set -x ; gcc $archflag $cfflag $bench -w)
			./a.out
			(set -x ; $PSZ -c rida a.out a.out.zipr) || exit 1
			./a.out.zipr || exit 1
		done
	done
done
