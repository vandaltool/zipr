# Zipr Clone with Symbols

# Zipr via Docker 

Run our docker image by [installing Docker](https://docs.docker.com/get-docker/), then running:

```
docker run -it git.zephyr-software.com:4567/opensrc/zipr/zipr-bin
```

The image will prompt you how to proceed.  To learn how to invoke and use Zipr transforms, see the [IRDB Cookbook Examples repository](https://git.zephyr-software.com/opensrc/irdb-cookbook-examples) section on using [transforms in Docker](https://git.zephyr-software.com/opensrc/irdb-cookbook-examples#docker-recommended).

# Building Zipr/Peasoup

The instructions that follow assume that:

* you have access to both the Zipr repo
* you have sudo privileges
* you are installing in your home directory
* you are using a recent version of Linux, e.g., Ubuntu 18.04


First install the Zipr static binary rewriting infrastructure
```
git clone --recurse-submodules  https://git.zephyr-software.com/opensrc/zipr.git # or git@git.zephyr-software.com:opensrc/zipr.git
cd zipr
. set_env_vars
./get-peasoup-packages.sh all
scons -j3
```

Carefully watch the `get-peasoup-packages` command for errors.  
It leverages' Ubuntu's `apt` or CentOS' `yum` commands to install packages.
All packages should be in standard repositiories, but your local configuration may
need to be adjusted if it doesn't work by default.

# Preparing Zipr for Use (Setting up local postgres tables)

Next we need to setup the proper tables in a local copy of the postgres database.  In the `zipr` directory, run:
```
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

Test the binary rewriting infrastructure by rewriting /bin/ls
```
cd /tmp
$PSZ /bin/ls ls.zipr -c rida
```
Your terminal's output should look like this:
```
Using Zipr backend.
Detected ELF shared object.
Performing step rida [dependencies=mandatory] ...Done.  Successful.
Performing step pdb_register [dependencies=mandatory] ...Done.  Successful.
Performing step fill_in_cfg [dependencies=unknown] ...Done.  Successful.
Performing step fill_in_indtargs [dependencies=unknown] ...Done.  Successful.
Performing step fix_calls [dependencies=unknown] ...Done.  Successful.
Performing step zipr [dependencies=none] ...Done.  Successful.
```

Invoke the rewritten version of /bin/ls and make sure it runs normally:
```
./ls.zipr
```

# Zipr with IDAPro 

In some configurations, Zipr can leverage IDA Pro's information to get better rewriting.  IDA is most useful when Rida cannot analyze the program (Rida currently only handles ELF x86 binaries, not PE binaries for Windows or other architectures).  To setup Zipr to use IDA, install (or clone) IDA and the corresponding IDA SDK, then set these environment variables:

```
export IDAROOT=/path/to/idapro
export IDASDK=/path/to/idapro-sdk
```

Next, rebuild Zipr:

```
$ cd /path/to/zipr 
$ scons
```

The `$PSZ` script uses IDA pro by default if it is setup properly.  You'll see the `meds-static` step replace the `rida` step.



