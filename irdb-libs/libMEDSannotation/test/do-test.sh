#!/bin/bash

g++ test.cpp -I ../include -fmax-errors=2 -L../../lib -lMEDSannotation -g

./a.out ls.annot
./a.out ls.infoannot
./a.out ls.STARSxrefs
