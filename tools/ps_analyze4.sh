#!/bin/sh

in=$1
shift
out=$1
shift
rest=$*

variants=4

for i in $(seq 0 $(expr $variants - 1)); do
	$PEASOUP_HOME/tools/ps_analyze.sh $in $out.$i $rest
done

echo "#!/bin/bash" > $out
chmod +x $out
echo "var=\$RANDOM" >> $out
echo "let \"var %= $variants\"" >> $out
echo "exec `realpath $out`.\$var \"\$@\"" >> $out



