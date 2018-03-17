import shutil
import os
import tarfile

(sysname, nodename, release, version, machine)=os.uname()

Import('env')

# build security transforms
irdbenv=env.Clone(); 

zipr=SConscript("src/SConscript", variant_dir='scons_build/zipr')

if sysname  != "SunOS":
	SConscript("test/SConscript")

pedi = Command( target = "./testoutput",
                source = "./SConscript",
                action = "cd "+os.environ['ZIPR_INSTALL']+" ; " +os.environ['PEDI_HOME']+"/pedi -m manifest.txt ; cd -" )

Depends(pedi,zipr)
Default( pedi )

