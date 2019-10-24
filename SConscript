import shutil
import os
import tarfile

Import('env')

def createFolder(directory):
    try:
        if not os.path.exists(directory):
            os.makedirs(directory)
    except OSError:
        print ('Error: Creating directory. ' +  directory)



createFolder(os.environ['ZEST_RUNTIME']+'/lib32')
createFolder(os.environ['ZEST_RUNTIME']+'/lib64')
createFolder(os.environ['ZEST_RUNTIME']+'/sbin')
createFolder(os.environ['ZEST_RUNTIME']+'/bin')

if not os.path.isfile("manifest.txt.config"):
	os.system("$PEDI_HOME/pedi --setup -m manifest.txt -l ida -l ida_key -l ps -l zipr -l stars -i $PS_INSTALL")

# build stars and libirdb
libirdb=      SConscript("irdb-libs/SConscript") 
libsmpsa=     SConscript("SMPStaticAnalyzer/SConscript") 

# now finish building irdb-libs once stars is setup
libirdbdeep=SConscript("irdb-libs/SConscript.deep")
Depends(libirdbdeep,libsmpsa)

print "Zipr install is "+env['ZIPR_INSTALL']
Export('env')


# list of zipr plugins and irdb xforms to build
transformDirs='''
	builtin_xforms/add_lib  	
	builtin_xforms/move_globals  	
	builtin_xforms/resolve_callbacks
	zipr_push64_reloc_plugin
	zipr
	zipr_unpin_plugin
	'''

# build the xforms and plugins
xforms=list()
for i in Split(transformDirs):
	Export('env')
	xform = SConscript(i+"/SConscript")
	Depends(xform, libirdbdeep)
	xforms = xforms + xform 


#finally, run pedi to do the final install
pedi = Command( target = "./zipr-umb-testoutput-install",
		source = xforms,
		action = os.environ['PEDI_HOME']+"/pedi -m manifest.txt " )

ret=[]+xforms
if Dir('.').abspath == Dir('#.').abspath:
	ret=ret+pedi



if env.GetOption('clean') and os.path.isfile("manifest.txt.config"):
	with open("manifest.txt.config") as myfile:
	    first_line=myfile.readlines()[0].rstrip()

	# if [[ $(head -1 manifest.txt.config) == $PS_INSTALL ]] ; then
	if str(first_line) == str(os.environ['PS_INSTALL']):
		print "Doing pedi clean as I'm the pedi root"
		os.system( "pwd; $PEDI_HOME/pedi -c -m manifest.txt " )
		shutil.rmtree(os.environ['PS_INSTALL'])



Default(ret)
Return('ret')
