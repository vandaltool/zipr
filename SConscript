import shutil
import os
import tarfile

(sysname, nodename, release, version, machine)=os.uname()

Import('env')

if 'do_cgc' in env and int(env['do_cgc']) == 1:
	env.Append(CFLAGS=" -DCGC ")
	env.Append(CCFLAGS=" -DCGC ")

# build security transforms
irdbenv=env.Clone(); 

sectrans_path=os.environ['SECURITY_TRANSFORMS_HOME']
sectrans_sconscript=os.path.join(sectrans_path,"SConscript");
env=irdbenv.Clone()
Export('env')   # for security_transforms.
SConscript(sectrans_sconscript, variant_dir='scons_build/irdb_libs')


#print 'env='
#print env.Dump()
SConscript("src/SConscript", variant_dir='scons_build/zipr')

if sysname  != "SunOS":
	SConscript("test/SConscript")
