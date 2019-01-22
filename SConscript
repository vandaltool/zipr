import shutil
import os
import tarfile

Import('env')

(sysname, nodename, release, version, machine)=os.uname()


if env.GetOption('clean'):
    if os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/targ-config.h"):
        print 'Removing include/targ-config.h'
    	os.remove(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/targ-config.h")
    os.chdir(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone")
    os.system("make clean")
    if os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone/zipr_unpack"):
    	shutil.rmtree(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone/zipr_unpack")
    os.chdir(os.environ['SECURITY_TRANSFORMS_HOME'])


else:

    # check/install targ-config.h
    #if not os.path.isfile(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/targ-config.h"):
	##print "uname=", sysname, " xx ", nodename, " xx ", release, " xx ", version, " xx ", machine
	#shutil.copy( os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'],"include",machine,"config.h"), 
		     #os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'],"include","targ-config.h"))

    os.chdir(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone")
    print "Rebuilding libcapstone."
    jobs=env.GetOption('num_jobs')
    os.system("make -j "+str(jobs))
    os.chdir(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone")
    if not os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone/zipr_unpack"):
	    os.mkdir(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone/zipr_unpack")
	    os.chdir(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone/zipr_unpack")
            print "Unpacking libcapstone.a for libIRDB-core."
	    os.system("ar x "+os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone/libcapstone.a")
    os.chdir(os.environ['SECURITY_TRANSFORMS_HOME'])



if "PEDI_HOME" in os.environ:
	pedi = Command( target = "./testoutput",
			source = "./SConscript",
			action = os.environ['PEDI_HOME']+"/pedi -m manifest.txt " )

env['BASE_IRDB_LIBS']="IRDB-core", "pqxx", "pq", "EXEIO"

if sysname != "SunOS":
	libPEBLISS=SConscript("pebliss/trunk/pe_lib/SConscript", variant_dir='scons_build/libPEBLISS')
	# setup libraries needed for linking
	env['BASE_IRDB_LIBS']=env['BASE_IRDB_LIBS']+("pebliss",)
	if "PEDI_HOME" in os.environ:
		Depends(pedi,libPEBLISS)

# pebliss requires iconv, which needs to be explicit on cygwin.
if "CYGWIN" in sysname:
	# add tuple of 1 item!
	env['BASE_IRDB_LIBS']=env['BASE_IRDB_LIBS']+("iconv",)

Export('env')

env.Install("$SECURITY_TRANSFORMS_HOME/lib/", "$SECURITY_TRANSFORMS_HOME/libcapstone/libcapstone.so.4")
env.Command(os.environ['SECURITY_TRANSFORMS_HOME']+"/lib/libcapstone.so", os.environ['SECURITY_TRANSFORMS_HOME']+"/lib/libcapstone.so.4", "ln -s $SOURCE.abspath $TARGET.abspath")
libcapstone=os.environ['SECURITY_TRANSFORMS_HOME']+"/lib/libcapstone.so"

libehp=env.SConscript("libehp/SConscript", variant_dir='scons_build/libehp')
libehp=env.Install("$SECURITY_TRANSFORMS_HOME/lib", libehp);
libtransform=SConscript("libtransform/SConscript", variant_dir='scons_build/libtransform')
libEXEIO=SConscript("libEXEIO/SConscript", variant_dir='scons_build/libEXEIO')
#libbea=SConscript("beaengine/SConscript", variant_dir='scons_build/beaengine')
libMEDSannotation=SConscript("libMEDSannotation/SConscript", variant_dir='scons_build/libMEDSannotation')
# libxform=SConscript("xform/SConscript", variant_dir='scons_build/libxform')
libIRDB=SConscript("libIRDB/SConscript", variant_dir='scons_build/libIRDB')
Depends(libIRDB,libcapstone)
libStructDiv=SConscript("libStructDiv/SConscript", variant_dir='scons_build/libStructDiv')
libElfDep=SConscript("libElfDep/SConscript", variant_dir='scons_build/libElfDep')
thanos=SConscript("thanos/SConscript", variant_dir='scons_build/thanos')
rida=SConscript("rida/SConscript", variant_dir='scons_build/rida')
meds2pdb=SConscript("meds2pdb/SConscript", variant_dir='scons_build/meds2pdb')
dump_map=SConscript("dump_map/SConscript", variant_dir='scons_build/dump_map')
dump_insns=SConscript("dump_insns/SConscript", variant_dir='scons_build/dump_insns')
ir_builders=SConscript("ir_builders/SConscript", variant_dir='scons_build/ir_builders')


tools=None
if 'build_tools' not in env or env['build_tools'] is None or int(env['build_tools']) == 1:
	tools=SConscript("tools/SConscript", variant_dir='scons_build/tools')
	if "PEDI_HOME" in os.environ:
		Depends(pedi,tools)

if "PEDI_HOME" in os.environ:
	Depends(pedi, (libehp,libtransform,libEXEIO,libMEDSannotation,libIRDB,libStructDiv,libElfDep, libcapstone, thanos, rida, meds2pdb, dump_map, dump_insns, ir_builders))
	Default( pedi )
else:

	Default(libehp,libtransform,libEXEIO,libMEDSannotation,libIRDB,libStructDiv,libElfDep, libcapstone, thanos, rida, meds2pdb, dump_map, dump_insns, ir_builders)
	if 'build_tools' not in env or env['build_tools'] is None or int(env['build_tools']) == 1:
		Default(tools)
