import os
import sys


env=Environment()

# default build options
env.Replace(CFLAGS="-fPIC")
env.Replace(CCFLAGS="-fPIC")
env.Replace(LDFLAGS="-fPIC")

# parse arguments
env.Replace(SECURITY_TRANSFORMS_HOME=os.environ['SECURITY_TRANSFORMS_HOME'])
env.Replace(do_64bit_build=ARGUMENTS.get("do_64bit_build",None))
env.Replace(debug=ARGUMENTS.get("debug",0))


if int(env['debug']) == 1:
        print "Setting debug mode"
        env.Append(CFLAGS=" -g")
        env.Append(CCFLAGS=" -g")
else:
        print "Setting release mode"
        env.Append(CFLAGS=" -O3")
        env.Append(CCFLAGS=" -O3")

# set 32/64 bit build properly
print  "env[64bit]="+str(env['do_64bit_build'])
if env['do_64bit_build'] is None:
	print 'Defaulting to default compilation size.'
elif int(env['do_64bit_build']) == 1:
	print 'Using 64-bit compilation size.'
        env.Append(CFLAGS=" -m64")
        env.Append(CCFLAGS=" -m64")
        env.Append(LDFLAGS=" -m64")
else:
	print 'Using 32-bit compilation size.'
        env.Append(CFLAGS=" -m32")
        env.Append(CCFLAGS=" -m32")
        env.Append(LDFLAGS=" -m32")


Export('env')
SConscript("SConscript")

