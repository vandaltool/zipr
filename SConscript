import shutil
import os
import tarfile

Import('env')


#if [ ! -f manifest.txt.config -o ! -d "$PS_INSTALL" ]; then

if not os.path.isfile("manifest.txt.config"):
	print "Doing pedi setup"  
	pedisetup = Command( target = "./testoutput-setup",
			source = "./SConscript",
			action = os.environ['PEDI_HOME'] + 
					"/pedi --setup -m manifest.txt -l ida -l ida_key -l ps -l zipr -l stars -i " + 
					os.environ['PS_INSTALL']
		)
else:
	print "Pedi already setup"  
	pedisetup=list()



# build stars and libirdb
libirdb=      SConscript("irdb-libs/SConscript") # , variant_dir='build/irdb-libs')
Depends(libirdb,pedisetup)
libsmpsa=     SConscript("SMPStaticAnalyzer/SConscript") # 

# specify some explicit dependencies to make sure these build in order
Depends(libsmpsa,libirdb)

# now finish building irdb-libs once stars is setup
libirdbdeep=SConscript("irdb-libs/SConscript.deep", variant_dir='build/irdb-libs')
Depends(libirdbdeep,libsmpsa)

# list of zipr plugins and irdb xforms to build
transformDirs='''
	zipr_large_only_plugin
	irdb_transforms
	zipr_push64_reloc_plugin
	zipr_relax_plugin
	zipr
	zipr_trace_plugin
	zipr_unpin_plugin
	'''

# build the xforms and plugins
xforms=list()
for i in Split(transformDirs):
	Export('env')
	xform = SConscript(i+"/SConscript")
	print "After step "+str(i)+", env[LINKFLAGS]=:"+env['LINKFLAGS']
	Depends(xform, libirdbdeep)
	xforms = xforms + xform 


#finally, run pedi to do the final install

pedi = Command( target = "./testoutput-install",
		source = "./SConscript",
		action = os.environ['PEDI_HOME']+"/pedi -m manifest.txt " )

Depends(pedi,  xforms)
Default( pedi )

if env.GetOption('clean') and os.path.isfile("manifest.txt.config"):
	with open("manifest.txt.config") as myfile:
	    first_line=myfile.readlines()[0] #put here the interval you want

	first_line=first_line.rstrip()

	# if [[ $(head -1 manifest.txt.config) == $PS_INSTALL ]] ; then
	if str(first_line) == str(os.environ['PS_INSTALL']):
		print "Doing pedi clean"
		os.system( "pwd; $PEDI_HOME/pedi -c -m manifest.txt " )
		shutil.rmtree(os.environ['PS_INSTALL'])
	else:
		print "Eliding pedi clean as I'm not the root" 
		print "Root is '"+first_line+"'"
		print "I am    '"+str(os.environ['PS_INSTALL'])+"'"
