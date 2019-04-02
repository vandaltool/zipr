import os



Import('env')

# import and create a copy of the environment so we don't screw up anyone elses env.
myenv=env.Clone()

cpppath='''
	 $SECURITY_TRANSFORMS_HOME/third_party/elfio-code
	 $PEASOUP_HOME/irdb-libs/libEXEIO/include
         $IRDB_SDK/include
        '''



files=Glob( Dir('.').srcnode().abspath+"/*.cpp")

pgm="move_globals.so"

LIBPATH="$SECURITY_TRANSFORMS_HOME/lib"
LIBS=Split("irdb-core irdb-cfg irdb-util irdb-transform irdb-deep StructDiv EXEIO ") 
myenv.Append(CPPPATH=Split(cpppath))
pgm=myenv.SharedLibrary(pgm,  files,  LIBPATH=LIBPATH, LIBS=LIBS)
install=myenv.Install("$SECURITY_TRANSFORMS_HOME/plugins_install/", pgm)
Default(install)

Return('install')
