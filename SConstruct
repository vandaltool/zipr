import os
import sys


env=Environment()

# default build options
env.Replace(CFLAGS="-fPIC -w ")
env.Replace(CXXFLAGS="-fPIC -w ")
env.Replace(LINKFLAGS="-fPIC -w ")

# parse arguments
env.Replace(SECURITY_TRANSFORMS_HOME=os.environ['SECURITY_TRANSFORMS_HOME'])
env.Replace(do_64bit_build=ARGUMENTS.get("do_64bit_build",None))
env.Replace(debug=ARGUMENTS.get("debug",0))
env.Replace(build_appfw=ARGUMENTS.get("build_appfw", None))

# by default, turn on build of appfw
if env['build_appfw'] is None:
	env['build_appfw'] = 1

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

# set 32/64 bit build properly
print  "env[64bit]="+str(env['do_64bit_build'])
if env['do_64bit_build'] is None:
	print 'Defaulting to default compilation size.'
elif int(env['do_64bit_build']) == 1:
	print 'Using 64-bit compilation size.'
        env.Append(CFLAGS=" -m64")
        env.Append(CXXFLAGS=" -m64")
        env.Append(LINKFLAGS=" -m64")
        env.Append(SHLINKFLAGS=" -m64")
else:
	print 'Using 32-bit compilation size.'
        env.Append(CFLAGS=" -m32")
        env.Append(CXXFLAGS=" -m32")
        env.Append(LINKFLAGS=" -m32")
        env.Append(SHLINKFLAGS=" -m32")

Export('env')
SConscript("SConscript", variant_dir='build')

