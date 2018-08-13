import shutil
import os
import tarfile

Import('env')

(sysname, nodename, release, version, machine)=os.uname()


if env.GetOption('clean'):
    if os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/ELFIO"):
        print 'Removing third_party/ELFIO'
    	shutil.rmtree(os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/ELFIO")
    if os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/elfio"):
        print 'Removing include/elfio'
    	shutil.rmtree(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/elfio")
    if os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/targ-config.h"):
        print 'Removing include/targ-config.h'
    	os.remove(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/targ-config.h")
    os.chdir(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone")
    os.system("make clean")
    if os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone/zipr_unpack"):
    	shutil.rmtree(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone/zipr_unpack")
    os.chdir(os.environ['SECURITY_TRANSFORMS_HOME'])


else:
    ELFIO_DIR=os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/ELFIO/"
    if not os.path.exists(ELFIO_DIR):
        os.makedirs(ELFIO_DIR)     # make directory 
        tgz=tarfile.open(os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/elfio-2.2.tar.gz", "r:gz")
        print 'Extracting elfio tarball'
        #tgz.list(verbose=False)
        tgz.extractall(ELFIO_DIR)
    	shutil.copytree(ELFIO_DIR+"elfio-2.2/elfio", os.environ['SECURITY_TRANSFORMS_HOME']+"/include/elfio")
        shutil.copy(os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'],"third_party","elfio.hpp"), 
		    os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'],"include","elfio","elfio.hpp"))
    else:
        assert os.path.isdir(ELFIO_DIR)

    # check/install targ-config.h
    if not os.path.isfile(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/targ-config.h"):
	#print "uname=", sysname, " xx ", nodename, " xx ", release, " xx ", version, " xx ", machine
	shutil.copy( os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'],"include",machine,"config.h"), 
		     os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'],"include","targ-config.h"))

    os.chdir(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone")
    print "Rebuilding libcapstone."
    os.system("./make.sh ")
    os.chdir(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone")
    if not os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone/zipr_unpack"):
	    os.mkdir(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone/zipr_unpack")
	    os.chdir(os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone/zipr_unpack")
            print "Unpacking libcapstone.a for libIRDB-core."
	    os.system("ar x "+os.environ['SECURITY_TRANSFORMS_HOME']+"/libcapstone/libcapstone.a")
    os.chdir(os.environ['SECURITY_TRANSFORMS_HOME'])


env['BASE_IRDB_LIBS']="IRDB-core", "pqxx", "pq", "EXEIO"

pedi = Command( target = "./testoutput",
		source = "./SConscript",
                action = os.environ['PEDI_HOME']+"/pedi -m manifest.txt " )

if sysname != "SunOS":
	libPEBLISS=SConscript("pebliss/trunk/pe_lib/SConscript", variant_dir='scons_build/libPEBLISS')
	# setup libraries needed for linking
	env['BASE_IRDB_LIBS']="IRDB-core", "pqxx", "pq", "EXEIO", "pebliss"
	Depends(pedi,libPEBLISS)

# pebliss requires iconv, which needs to be explicit on cygwin.
if "CYGWIN" in sysname:
	# add tuple of 1 item!
	env['BASE_IRDB_LIBS']=env['BASE_IRDB_LIBS']+("iconv",)

Export('env')

libehp=env.SConscript("libehp/SConscript", variant_dir='scons_build/libehp')
libehp=env.Install("$SECURITY_TRANSFORMS_HOME/lib", libehp);

libtransform=SConscript("libtransform/SConscript", variant_dir='scons_build/libtransform')
libEXEIO=SConscript("libEXEIO/SConscript", variant_dir='scons_build/libEXEIO')
libbea=SConscript("beaengine/SConscript", variant_dir='scons_build/beaengine')
libMEDSannotation=SConscript("libMEDSannotation/SConscript", variant_dir='scons_build/libMEDSannotation')
libxform=SConscript("xform/SConscript", variant_dir='scons_build/libxform')
libIRDB=SConscript("libIRDB/SConscript", variant_dir='scons_build/libIRDB')
libStructDiv=SConscript("libStructDiv/SConscript", variant_dir='scons_build/libStructDiv')
libElfDep=SConscript("libElfDep/SConscript", variant_dir='scons_build/libElfDep')


Depends(pedi, (libehp,libtransform,libEXEIO,libbea,libMEDSannotation,libxform,libIRDB,libStructDiv,libElfDep))

tools=None
if 'build_tools' not in env or env['build_tools'] is None or int(env['build_tools']) == 1:
	tools=SConscript("tools/SConscript", variant_dir='scons_build/tools')
	Depends(pedi,tools)

Default( pedi )

