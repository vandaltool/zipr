import os
import sys

(sysname, nodename, release, version, machine)=os.uname()


env=Environment()

# default build options
env.Replace(CFLAGS=" -fPIC ")
env.Replace(CXXFLAGS=" -std=c++0x -fPIC ")
env.Replace(LINKFLAGS=" -fPIC ")

# parse arguments
env.Replace(SECURITY_TRANSFORMS_HOME=os.environ['SECURITY_TRANSFORMS_HOME'])
env.Replace(ZIPR_HOME=os.environ['ZIPR_HOME'])
env.Replace(ZIPR_INSTALL=os.environ['ZIPR_INSTALL'])
env.Replace(ZIPR_SDK=os.environ['ZIPR_SDK'])
env.Replace(profile=ARGUMENTS.get("profile",0))
env.Replace(debug=ARGUMENTS.get("debug",0))
env.Replace(do_cgc=ARGUMENTS.get("do_cgc",0))
env.Replace(do_64bit_build=ARGUMENTS.get("do_64bit_build",0))

if int(env['profile']) == 1:
        print "Setting profile and debug mode"
        env.Append(CFLAGS=" -pg")
        env.Append(CXXFLAGS=" -pg")
        env.Append(LINKFLAGS=" -pg")
        env['debug'] = 1
if int(env['debug']) == 1:
        print "Setting debug mode"
        env.Append(CFLAGS=" -g")
        env.Append(CXXFLAGS=" -g")
        env.Append(LINKFLAGS=" -g")
else:
        print "Setting release mode"
        env.Append(CFLAGS=" -O3")
        env.Append(CXXFLAGS=" -O3")
        env.Append(LINKFLAGS=" -O3")

env['build_appfw']=0
env['build_tools']=0

# add extra flag for solaris.
if sysname == "SunOS":
        env.Append(LINKFLAGS=" -L/opt/csw/lib -DSOLARIS  ")
        env.Append(CFLAGS=" -I/opt/csw/include -DSOLARIS ")
        env.Append(CXXFLAGS=" -I/opt/csw/include -DSOLARIS  ")
else:
	env.Append(CFLAGS=" -fPIE ")
	env.Append(CXXFLAGS=" -fPIE ")
	env.Append(LINKFLAGS=" -fPIE ")
	


Export('env')
SConscript("SConscript", variant_dir='build')

