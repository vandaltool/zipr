# Building Zipr/Peasoup

The instructions that follow assume that:

* you have access to both the Zipr and ZAFL repo
* you have sudo privileges
* you are installing in your home directory
* you are using a recent version of Linux, e.g., Ubuntu 18.04


First install the Zipr static binary rewriting infrastructure
```
git clone --recurse-submodules  https://git.zephyr-software.com/allnp/zafl_umbrella.git # or git@git.zephyr-software.com:opensrc/peasoup_umbrella.git
cd peasoup_umbrella
. set_env_vars
./get-peasoup-packages.sh all
scons -j3
```

Carefully watch the `get-peasoup-packages` command for errors.  
It leverages' Ubuntu's `apt` or CentOS' `yum` commands to install packages.
All packages should be in standard repositiories, but your local configuration may
need to be adjusted if it doesn't work by default.

# Preparing Zipr for Use (Setting up local postgres tables)

Next we need to setup the proper tables in a local copy of the postgres database.
```
cd ~/peasoup_umbrella
./postgres_setup.sh
```

If all goes well with the postgres setup, you should be able to login into the database by typing: psql
The output of psql should look something like this:
```
psql (9.3.22)
SSL connection (cipher: DHE-RSA-AES256-GCM-SHA384, bits: 256)
Type "help" for help.

peasoup_XXX=> 
```

# Testing Zipr

Test  the binary rewriting infrastructure by rewriting /bin/ls
```
cd /tmp
$PSZ /bin/ls ls.zipr -c rida
```
Your terminal's output should look like this:
```
Using Zipr backend.
Detected ELF file.
Performing step gather_libraries [dependencies=mandatory] ...Done. Successful.
Performing step meds_static [dependencies=mandatory] ...Done. Successful.
Performing step pdb_register [dependencies=mandatory] ...Done. Successful.
Performing step fill_in_cfg [dependencies=mandatory] ...Done. Successful.
Performing step fill_in_indtargs [dependencies=mandatory] ...Done. Successful.
Performing step clone [dependencies=mandatory] ...Done. Successful.
Performing step fix_calls [dependencies=mandatory] ...Done. Successful.
Program not detected in signature database.
Performing step zipr [dependencies=clone,fill_in_indtargs,fill_in_cfg,pdb_register] ...Done. Successful.
```

Invoke the rewritten version of /bin/ls and make sure it runs normally:
```
./ls.zipr
```
