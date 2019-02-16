import os
import sys

env=Environment()


# default build options
env.Replace(CFLAGS="   -fPIC -fmax-errors=2 -Wall -Werror -fmax-errors=2 ")
env.Replace(CXXFLAGS=" -fPIC -fmax-errors=2 -Wall -Werror -fmax-errors=2 ")
env.Replace(LINKFLAGS="-fPIC -fmax-errors=2 -Wall -Werror -fmax-errors=2 -Wl,-unresolved-symbols=ignore-in-shared-libs ")
env.Replace(SHLINKFLAGS="-fPIC -fmax-errors=2 -Wall -Werror -fmax-errors=2 -shared ") # default is ignore all link errors

# parse arguments into env and set default values.
env.Replace(SECURITY_TRANSFORMS_HOME=os.environ['SECURITY_TRANSFORMS_HOME'])
env.Replace(IRDB_SDK=os.environ['IRDB_SDK'])
env.Replace(SMPSA_HOME=os.environ['SMPSA_HOME'])
env.Replace(debug=ARGUMENTS.get("debug",0))
env.Replace(PEDI_HOME=os.environ['PEDI_HOME'])


if int(env['debug']) == 1:
        print "Setting debug mode"
        env.Append(CFLAGS="      -g ")
        env.Append(CXXFLAGS="    -g ")
	env.Append(LINKFLAGS="   -g ")
	env.Append(SHLINKFLAGS=" -g ")
else:
        print "Setting release mode"
        env.Append(CFLAGS="      -O ")
        env.Append(CXXFLAGS="    -O ")
        env.Append(LINKFLAGS="   -O  ")
        env.Append(SHLINKFLAGS=" -O  ")

Export('env')
SConscript("SConscript")

