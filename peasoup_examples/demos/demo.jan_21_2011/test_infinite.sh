#!/bin/sh

echo "Invoking dumbledore_cmd.original with GraCE's suggested input.\n"

echo "./dumbledore_cmd.original \"\`cat dumbledore.exploits/grace_infinite.txt \`\"\n"

ulimit -t 20 
./dumbledore_cmd.original "`cat dumbledore.exploits/grace_infinite.txt`"

