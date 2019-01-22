import os
import sys



(sysname, nodename, release, version, machine)=os.uname()

env=Environment()


# default build options
env.Replace(CFLAGS="-fPIC  -fmax-errors=2 -Wall -Werror -fmax-errors=2")
env.Replace(CXXFLAGS="-fPIC  -fmax-errors=2 -Wall -Werror -fmax-errors=2 ")
env.Replace(LINKFLAGS="-fPIC -fmax-errors=2 -Wall -Werror -fmax-errors=2 ")

# parse arguments
env.Replace(SECURITY_TRANSFORMS_HOME=os.environ['SECURITY_TRANSFORMS_HOME'])
if 'PEDI_HOME' in os.environ:
	env.Replace(PEDI_HOME=os.environ['PEDI_HOME'])
#env.Replace(SMPSA_HOME=os.environ['SMPSA_HOME'])
env.Replace(do_64bit_build=ARGUMENTS.get("do_64bit_build",None))
env.Replace(debug=ARGUMENTS.get("debug",0))
env.Replace(build_appfw=ARGUMENTS.get("build_appfw", 0))
env.Replace(build_tools=ARGUMENTS.get("build_tools", 1))
env.Replace(build_stars=ARGUMENTS.get("build_stars", 1))
env.Replace(build_cgc=ARGUMENTS.get("build_cgc", 0))

env.Append(LINKFLAGS=" -Wl,-unresolved-symbols=ignore-in-shared-libs ")

if int(env['debug']) == 1:
        print "Setting debug mode"
        env.Append(CFLAGS=" -g ")
        env.Append(CXXFLAGS=" -g ")
	env.Append(LINKFLAGS=" -g ")
	env.Append(SHLINKFLAGS=" -g ")
else:
        print "Setting release mode"
        env.Append(CFLAGS=" -O3 ")
        env.Append(CXXFLAGS=" -O3 ")
        env.Append(LINKFLAGS=" -O3  ")
        env.Append(SHLINKFLAGS=" -O3  ")

# set 32/64 bit build properly
print  "env[64bit]="+str(env['do_64bit_build'])
if env['do_64bit_build'] is None:
	print 'Defaulting to default compilation size.'
elif int(env['do_64bit_build']) == 1:
	print 'Using 64-bit compilation size.'
        env.Append(CFLAGS=" -m64 ")
        env.Append(CXXFLAGS=" -m64 ")
        env.Append(LINKFLAGS=" -m64 ")
        env.Append(SHLINKFLAGS=" -m64 ")
else:
	print 'Using 32-bit compilation size.'
        env.Append(CFLAGS=" -m32 ")
        env.Append(CXXFLAGS=" -m32 ")
        env.Append(LINKFLAGS=" -m32 ")
        env.Append(SHLINKFLAGS=" -m32 ")



# add extra flag for solaris.
if sysname == "SunOS":
        env.Append(LINKFLAGS=" -L/opt/csw/lib -DSOLARIS  ")
        env.Append(SHLINKFLAGS=" -L/opt/csw/lib -DSOLARIS  ")
        env.Append(CFLAGS=" -I/opt/csw/include -DSOLARIS ")
        env.Append(CXXFLAGS=" -I/opt/csw/include -DSOLARIS  ")


Export('env')
SConscript("SConscript", variant_dir='build')

