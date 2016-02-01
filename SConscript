import shutil
import os
import tarfile

Import('env')

(sysname, nodename, release, version, machine)=os.uname()


if env.GetOption('clean'):
    if os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/ELFIO"):
        print 'Removing third_party/ELFIO'
    	shutil.rmtree(os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/ELFIO")
    if os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/SQLITE3"):
        print 'Removing third_party/SQLITE3'
    	shutil.rmtree(os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/SQLITE3")
    if os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/elfio"):
        print 'Removing include/elfio'
    	shutil.rmtree(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/elfio")
    if os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/targ-config.h"):
        print 'Removing include/targ-config.h'
    	os.remove(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/targ-config.h")
else:
    ELFIO_DIR=os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/ELFIO/"
    if not os.path.exists(ELFIO_DIR):
        os.makedirs(ELFIO_DIR)     # make directory 
        tgz=tarfile.open(os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/elfio-2.2.tar.gz", "r:gz")
        print 'Extracting elfio tarball'
        tgz.list(verbose=False)
        tgz.extractall(ELFIO_DIR)
    	shutil.copytree(ELFIO_DIR+"elfio-2.2/elfio", os.environ['SECURITY_TRANSFORMS_HOME']+"/include/elfio")
        shutil.copy(os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'],"third_party","elfio.hpp"), 
		    os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'],"include","elfio","elfio.hpp"))
    else:
        assert os.path.isdir(ELFIO_DIR)

    # SQLITE3
    SQLITE3_DIR=os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'], 'third_party/SQLITE3')
    if not os.path.exists(SQLITE3_DIR):
        os.makedirs(SQLITE3_DIR)     # make directory 
        tgz=tarfile.open(os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'], 'third_party/sqlite-autoconf-3071300.tar.gz'), "r:gz")
        print 'Extracting needed files from sqlite3 tarball'
        tgz.list(verbose=False)
        tgz.extract('sqlite-autoconf-3071300/sqlite3.h', SQLITE3_DIR)
        tgz.extract('sqlite-autoconf-3071300/sqlite3.c', SQLITE3_DIR)
		# copy sqlite3.h
        source_dir = os.path.join(SQLITE3_DIR, 'sqlite-autoconf-3071300') 
        target_dir = os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'], 'appfw', 'src')
        shutil.copy(os.path.join(source_dir, 'sqlite3.h'), os.path.abspath(os.path.join(target_dir, 'sqlite3.h')))
    else:
        assert os.path.isdir(SQLITE3_DIR)

    # check/install targ-config.h
    if not os.path.isfile(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/targ-config.h"):
	#print "uname=", sysname, " xx ", nodename, " xx ", release, " xx ", version, " xx ", machine
	shutil.copy( os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'],"include",machine,"config.h"), 
		     os.path.join(os.environ['SECURITY_TRANSFORMS_HOME'],"include","targ-config.h"))



env['BASE_IRDB_LIBS']="IRDB-core", "pqxx", "pq", "BeaEngine_s_d", "EXEIO"

if sysname != "SunOS":
	libPEBLISS=SConscript("pebliss/trunk/pe_lib/SConscript", variant_dir='scons_build/libPEBLISS')
	# setup libraries needed for linking
	env['BASE_IRDB_LIBS']="IRDB-core", "pqxx", "pq", "BeaEngine_s_d", "EXEIO", "pebliss"

# pebliss requires iconv, which needs to be explicit on cygwin.
if "CYGWIN" in sysname:
	# add tuple of 1 item!
	env['BASE_IRDB_LIBS']=env['BASE_IRDB_LIBS']+("iconv",)

Export('env')


libEXEIO=SConscript("libEXEIO/SConscript", variant_dir='scons_build/libEXEIO')
libbea=SConscript("beaengine/SConscript", variant_dir='scons_build/beaengine')
libMEDSannotation=SConscript("libMEDSannotation/SConscript", variant_dir='scons_build/libMEDSannotation')
libxform=SConscript("xform/SConscript", variant_dir='scons_build/libxform')
libtransform=SConscript("libtransform/SConscript", variant_dir='scons_build/libtransform')
libIRDB=SConscript("libIRDB/SConscript", variant_dir='scons_build/libIRDB')
libStructDiv=SConscript("libStructDiv/SConscript", variant_dir='scons_build/libStructDiv')

if 'build_tools' not in env or env['build_tools'] is None or int(env['build_tools']) == 1:
	SConscript("tools/SConscript", variant_dir='scons_build/tools')

# appfw
if 'build_appfw' in env:
    if int(env['build_appfw']) == 1:		 
        SConscript("appfw/src/SConscript.64", variant_dir='scons_build/appfw.64')
        SConscript("appfw/src/SConscript.32", variant_dir='scons_build/appfw.32')

