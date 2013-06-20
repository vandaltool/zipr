
test:
	g++ -fno-stack-protector -lxqilla XQuery_Injection.cpp -o XQuery_Injection -lxqilla -lxerces-c-3.1

peasoup: test
	$(PEASOUP_HOME)/tools/ps_analyze.sh XQuery_Injection XQuery_Injection.peasoup --step concolic=off --step ilr=off --step p1transform=off --step integertransform=off --step determine_program=off

clean: 
	rm -Rf XQuery_Injection peasoup_executable*

