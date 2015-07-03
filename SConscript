import shutil
import os
import tarfile

Import('env')

if env.GetOption('clean'):
    if os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/ELFIO"):
	print 'Removing third_party/ELFIO'
    	shutil.rmtree(os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/ELFIO")
    if os.path.exists(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/elfio"):
	print 'Removing include/elfio'
    	shutil.rmtree(os.environ['SECURITY_TRANSFORMS_HOME']+"/include/elfio")
else:
    ELFIO_DIR=os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/ELFIO/"
    if not os.path.exists(ELFIO_DIR):
        os.makedirs(ELFIO_DIR)     # make directory 
        tgz=tarfile.open(os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/elfio-2.2.tar.gz", "r:gz")
	print 'Extracting elfio tarball'
	tgz.list(verbose=False)
        tgz.extractall(ELFIO_DIR)
    	shutil.copytree(ELFIO_DIR+"elfio-2.2/elfio", os.environ['SECURITY_TRANSFORMS_HOME']+"/include/elfio")
	shutil.copy(os.environ['SECURITY_TRANSFORMS_HOME']+"/third_party/elfio.hpp", os.environ['SECURITY_TRANSFORMS_HOME']+"/include/elfio/elfio.hpp")
    else:
        assert os.path.isdir(ELFIO_DIR)


#print 'env='
#print env.Dump()
libbea=SConscript("beaengine/SConscript", variant_dir='scons_build/beaengine')
libMEDSannotation=SConscript("libMEDSannotation/SConscript", variant_dir='scons_build/libMEDSannotation')
libxform=SConscript("xform/SConscript", variant_dir='scons_build/libxform')
libtransform=SConscript("libtransform/SConscript", variant_dir='scons_build/libtransform')
libIRDB=SConscript("libIRDB/SConscript", variant_dir='scons_build/libIRDB')
SConscript("tools/SConscript", variant_dir='scons_build/tools')

build_appfw=ARGUMENTS.get("build_appfw", None)
if build_appfw is None or int(build_appfw)==1:
	SConscript("appfw/SConscript", variant_dir='scons_build/appfw')
#if ARGUMENTS.get("build_appfw", None) is None or int(env['build_appfw'])==1:
