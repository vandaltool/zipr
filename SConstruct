import os
import sys



(sysname, nodename, release, version, machine)=os.uname()

env=Environment()


# default build options
env.Replace(CFLAGS="-fPIC -w ")
env.Replace(CXXFLAGS="-fPIC -w ")
env.Replace(LINKFLAGS="-fPIC -w ")

# parse arguments
env.Replace(SECURITY_TRANSFORMS_HOME=os.environ['SECURITY_TRANSFORMS_HOME'])
env.Replace(SMPSA_HOME=os.environ['SMPSA_HOME'])
env.Replace(do_64bit_build=ARGUMENTS.get("do_64bit_build",None))
env.Replace(debug=ARGUMENTS.get("debug",0))
env.Replace(build_appfw=ARGUMENTS.get("build_appfw", 1))
env.Replace(build_tools=ARGUMENTS.get("build_tools", 1))
env.Replace(build_stars=ARGUMENTS.get("build_stars", 1))
env.Replace(build_cgc=ARGUMENTS.get("build_cgc", 0))


if int(env['debug']) == 1:
        print "Setting debug mode"
        env.Append(CFLAGS=" -g ")
        env.Append(CXXFLAGS=" -g ")
        env.Append(LINKFLAGS=" -g ")
else:
        print "Setting release mode"
        env.Append(CFLAGS=" -O3 ")
        env.Append(CXXFLAGS=" -O3 ")
        env.Append(LINKFLAGS=" -O3 ")

if 'build_cgc' in env and int(env['build_cgc']) == 1:
        print "Setting debug mode"
        env.Append(CFLAGS=" -DCGC ")
        env.Append(CXXFLAGS=" -DCGC ")
        env.Append(LINKFLAGS=" -DCGC ")
	print 'Turn off appfw as we are building CGC'
	env['build_appfw'] = 0
elif env['build_appfw'] is None: # by default, turn on build of appfw, unless cgc is on
	env['build_appfw'] = 1


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
        env.Append(CFLAGS=" -I/opt/csw/include -DSOLARIS ")
        env.Append(CXXFLAGS=" -I/opt/csw/include -DSOLARIS  ")


Export('env')
SConscript("SConscript", variant_dir='build')

