import shutil
import os
import tarfile



env=Environment()
Export('env')


if env.GetOption('clean'):
    if os.path.exists("third_party/ELFIO"):
    	shutil.rmtree("third_party/ELFIO")
    if os.path.exists("include/elfio"):
    	shutil.rmtree("include/elfio")
else:
    ELFIO_DIR="third_party/ELFIO/"
    if not os.path.exists(ELFIO_DIR):
        os.makedirs(ELFIO_DIR)     # make directory 
        tgz=tarfile.open("third_party/elfio-2.2.tar.gz", "r:gz")
	print 'Extracting elfio tarball'
	tgz.list(verbose=False)
        tgz.extractall(ELFIO_DIR)
    	shutil.copytree(ELFIO_DIR+"elfio-2.2/elfio", "include/elfio")
	shutil.copy("third_party/elfio.hpp", "include/elfio/elfio.hpp")
    else:
        assert os.path.isdir(ELFIO_DIR)


libbea=SConscript("beaengine/SConscript", variant_dir='scons_build/beaengine')
libMEDSannotation=SConscript("libMEDSannotation/SConscript", variant_dir='scons_build/libMEDSannotation')
libxform=SConscript("xform/SConscript", variant_dir='scons_build/libxform')
libtransform=SConscript("libtransform/SConscript", variant_dir='scons_build/libtransform')
libIRDB=SConscript("libIRDB/SConscript", variant_dir='scons_build/libIRDB')
SConscript("tools/SConscript", variant_dir='scons_build/tools')
