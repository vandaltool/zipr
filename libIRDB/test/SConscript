import os



Import('env')
myenv=env.Clone()
myenv.Replace(SECURITY_TRANSFORMS_HOME=os.environ['SECURITY_TRANSFORMS_HOME'])

installed=[]

if 'build_tools' not in myenv or myenv['build_tools'] is None or int(myenv['build_tools']) == 1:

	cpppath=''' 
		 $SECURITY_TRANSFORMS_HOME/include 
		 $SECURITY_TRANSFORMS_HOME/libIRDB/include 
		 $SECURITY_TRANSFORMS_HOME/libMEDSannotation/include 
		 $SECURITY_TRANSFORMS_HOME/libEXEIO/include 
		'''

	LIBPATH="$SECURITY_TRANSFORMS_HOME/lib"
	LIBS=Split( 'IRDB-cfg IRDB-util ' + env.subst('$BASE_IRDB_LIBS')+ " MEDSannotation")

	myenv=myenv.Clone(CPPPATH=Split(cpppath))

	myenv.Append(CCFLAGS=" -std=c++11 -Wall")

	ehframe=myenv.Object("read_ehframe.cpp");
	split_eh_frame=myenv.Object("split_eh_frame.cpp");

	pgm=myenv.Program("fill_in_indtargs.exe",  ehframe+split_eh_frame+Split("fill_in_indtargs.cpp check_thunks.cpp"), LIBPATH=LIBPATH, LIBS=LIBS)
	install=myenv.Install("$SECURITY_TRANSFORMS_HOME/bin/", pgm)
	Default(install)
	installed=installed+install

	pgm=myenv.Program("fill_in_cfg.exe",  split_eh_frame+Split("fill_in_cfg.cpp"), LIBPATH=LIBPATH, LIBS=LIBS)
	install=myenv.Install("$SECURITY_TRANSFORMS_HOME/bin/", pgm)
	Default(install)
	installed=installed+install

	pgm=myenv.Program("fix_calls.exe",  ehframe+Split("fix_calls.cpp"), LIBPATH=LIBPATH, LIBS=LIBS)
	install=myenv.Install("$SECURITY_TRANSFORMS_HOME/bin/", pgm)
	Default(install)
	installed=installed+install

	# most programs go to $sectrans/bin
	pgms='''clone 
		generate_spri 
		find_strings 
		mark_functions_safe
		'''
	for i in Split(pgms):
		# print "Registering pgm: "+ i
		pgm=myenv.Program(target=i+".exe",  source=Split(i+".cpp"), LIBPATH=LIBPATH, LIBS=LIBS)
		install=myenv.Install("$SECURITY_TRANSFORMS_HOME/bin/", pgm)
		Default(install)
		installed=installed+install


	# ilr goes to $sectrans/plugins_install
	pgm=myenv.Program(target="ilr.exe",  source=Split("ilr.cpp"), LIBPATH=LIBPATH, LIBS=LIBS)
	install=myenv.Install("$SECURITY_TRANSFORMS_HOME/plugins_install/", pgm)
	Default(install)
	installed=installed+install

Return('installed')

