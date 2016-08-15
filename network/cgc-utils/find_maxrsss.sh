#!/bin/bash

type="maxrss"
if [ $# -gt 0 ]; then
	type=$1
fi
#echo $type

case $type in
	"maxrss") ;;
	"minflt") ;;
	"sw-cpu-clock") ;;
	"sw-task-clock") ;;
	*)
		echo "Unrecognized type."
		exit
		;;
esac

filenames=""
for i in `ls build/*.txt | sort -u`; do
j=${i%%.*t}
filenames="$filenames ${j#b*/}"
done

for i in `echo $filenames | tr ' ' '\n' | sort -u | grep -v _patched`; do
	output=""
	output="${i},${type},"
	for t in `echo default zipr-`; do
	#for t in `echo default coverage-`; do
		sum=0
		n=0
		avg=0
		tag=${t%%default}
		for test_type in `echo testing release`; do
			ms=`cat build/${i}.${t%%default}for-${test_type}.txt | grep ${type} | awk '{print $5;}' | sort -n`
			# > build/${i}.${type}.${t%%-}.stats`
			for m in $ms; do
				n=$((n+1))
				sum=$((sum+m)) 
			done
		done
		output+="${t%%-},"
		output+="n:${n}"
		output+=",sum:${sum}"
		output+=",avg:"
		output+=`echo "scale=2; ${sum}/${n}" | bc`
		output+=","
		#echo "n:" ${n} > build/${i}.${type}.${t%%-}.stats
		#echo "sum:" ${sum} >> build/${i}.${type}.${t%%-}.stats
		#echo "avg:" `echo "scale=2; ${sum}/${n}" | bc` \
		#	>> build/${i}.${type}.${t%%-}.stats
	done
	echo $output
done
