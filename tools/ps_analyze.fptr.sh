# temporary until we get full STARS analysis
# for now we generate annotation file by hand
# and put this file in a well-known location
cp $1 /tmp/a.ncexe.fptrannot

$PEASOUP_HOME/tools/ps_analyze.sh $2 $3 --step ilr=off --step integertransform=off --step concolic=off --step toctou=off --step twitchertransform=off --step p1transform=off --step fptr_shadow=on --step deadlock=off --step schedperturb=off
