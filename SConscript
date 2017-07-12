import shutil
import os
import tarfile

Import('env')

(sysname, nodename, release, version, machine)=os.uname()



#print 'env='
#print env.Dump()



myenv=env
myenv.Replace(SECURITY_TRANSFORMS_HOME=os.environ['SECURITY_TRANSFORMS_HOME'])
myenv.Replace(ZIPR_HOME=os.environ['ZIPR_HOME'])
myenv.Replace(ZIPR_SDK=os.environ['ZIPR_SDK'])
myenv.Replace(ZIPR_INSTALL=os.environ['ZIPR_INSTALL'])
myenv.Replace(do_cgc=ARGUMENTS.get("do_cgc",0))

if 'do_cgc' in env and int(env['do_cgc']) == 1:
        myenv.Append(CFLAGS=" -DCGC ")
        myenv.Append(CCFLAGS=" -DCGC ")




files=  '''
	unpin.cpp
	'''

# ELFIO needs to be first so we get the zipr version instead of the sectrans version.  the zipr version is modified to include get_offset.
cpppath=''' 
	.
	$ZIPR_HOME/third_party/ELFIO/elfio-2.2	
	$SECURITY_TRANSFORMS_HOME/include/
	$SECURITY_TRANSFORMS_HOME/libIRDB/include/
	$SECURITY_TRANSFORMS_HOME/beaengine/include
	$SECURITY_TRANSFORMS_HOME/beaengine/beaengineSources/Includes/
	$SECURITY_TRANSFORMS_HOME/tools/transforms
	$ZIPR_HOME/include/
	$ZIPR_SDK/include/
	'''

libs='''
	'''

libpath='''
	$SECURITY_TRANSFORMS_HOME/lib
	'''

if sysname != "SunOS":
	myenv.Append(CCFLAGS=" -Wall ")

myenv.Append(CXXFLAGS=" -std=c++11 ")
myenv=myenv.Clone(CPPPATH=Split(cpppath), LIBS=Split(libs), LIBPATH=Split(libpath), SHLIBSUFFIX=".zpi", SHLIBPREFIX="")
lib=myenv.SharedLibrary("unpin", Split(files))

install=myenv.Install("$ZIPR_INSTALL/plugins/", lib)
Default(install)

pedi = Command( target = "./testoutput",
                source = "./SConscript",
                action = "cd "+os.environ['ZIPR_INSTALL']+" ; " +os.environ['PEDI_HOME']+"/pedi -m manifest.txt ; cd -" )
Default( pedi )




