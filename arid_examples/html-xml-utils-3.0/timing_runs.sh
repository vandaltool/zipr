#!/bin/sh

. /home/jdh8d/peasoup/set_env

benchs="count wls xref"
links="dynamic static"
echo "Running NATIVE"

for l1 in $links
do
    for b1 in $benchs
    do
        scr_to_run="native_build.$l1.sh"
        echo "-------------------------------------------------"
        time -o native.$l1.$b1.time.out -f "%E real\n%U user\n%S sys\n" sh ${scr_to_run} $b1
        echo "-------------------------------------------------"
    done
done
echo "END NATIVE"

echo "Running PS"

for l in $links
do
    for b in $benchs
    do
        script_to_run="${l}_ps.sh"
        echo "-------------------------------------------------"
        echo "  BEGIN "
        echo " Running time sh ${l}_ps.sh $b"
        echo "-------------------------------------------------"
        echo 
        time -o ps_$l.$b.time.out -f "%E real\n%U user\n%S sys\n" sh ${script_to_run} $b
        echo "-------------------------------------------------"
        echo "Time for $l configure and ps_analyze for $b above"
        echo "-------------------------------------------------"
        echo "  END"
        echo "-------------------------------------------------"
    done
done

echo "END PS"


