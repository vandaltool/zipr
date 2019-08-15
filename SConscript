import shutil
import os
import tarfile

Import('env')

# build security transforms
irdbenv=env.Clone(); 

zipr=SConscript("src/SConscript")

pedi = Command( target = "./zipr-testoutput",
                source = zipr,
                action = "echo zipr; cd "+os.environ['ZIPR_INSTALL']+" ; " +os.environ['PEDI_HOME']+"/pedi -m manifest.txt ; cd -" )

ret=[zipr]
if Dir('.').abspath == Dir('#.').abspath:
	ret=pedi+zipr


Default(ret)
Return('ret')
