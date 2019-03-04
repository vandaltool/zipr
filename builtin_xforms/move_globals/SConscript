import os



Import('irdb_env')

# import and create a copy of the environment so we don't screw up anyone elses env.
myenv=irdb_env.Clone()
myenv.Replace(SECURITY_TRANSFORMS_HOME=os.environ['SECURITY_TRANSFORMS_HOME'])
myenv.Replace(ZIPR_HOME=os.environ['ZIPR_HOME'])
myenv.Replace(ZIPR_SDK=os.environ['ZIPR_SDK'])
myenv.Replace(ZIPR_INSTALL=os.environ['ZIPR_INSTALL'])
myenv.Append(CXXFLAGS = " -std=c++11 -Wall ")

cpppath=myenv['IRDB_INC'] + " $SECURITY_TRANSFORMS_HOME/third_party/elfio-code "

files=Glob( Dir('.').srcnode().abspath+"/*.cpp")

pgm="move_globals.so"

LIBPATH="$SECURITY_TRANSFORMS_HOME/lib"
LIBS=Split("stars "+ myenv.subst('$BASE_IRDB_LIBS')+ " irdb-cfg irdb-util irdb-transform irdb-deep StructDiv EXEIO ") 
myenv=myenv.Clone(CPPPATH=Split(cpppath))
pgm=myenv.SharedLibrary(pgm,  files,  LIBPATH=LIBPATH, LIBS=LIBS)
install=myenv.Install("$SECURITY_TRANSFORMS_HOME/plugins_install/", pgm)
Default(install)

Return('install')
