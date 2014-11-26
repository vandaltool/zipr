#!/bin/bash

# Installer name is of the form: program.install.sh
PROG=$(basename $0 .install.sh)

# Default installation directory
INSTALL_DIR=/tmp

echo "Installing protected version of $PROG"
echo "Installation directory: $INSTALL_DIR"
echo "Fully qualified path: $INSTALL_DIR/$PROG"

echo
echo "Type any key to continue"
read anykey

chmod +x analysis_dir/a.stratafied

# wipe out any old installs
if [ -d "$INSTALL_DIR/${PROG}_analysis" ]; then
		rm -fr "$INSTALL_DIR/${PROG}_analysis"
fi

#cp -r analysis_dir $INSTALL_DIR/${PROG}_analysis
mv analysis_dir $INSTALL_DIR/${PROG}_analysis
echo "$INSTALL_DIR/${PROG}_analysis/ps_run.sh $INSTALL_DIR/${PROG}_analysis     \"\$0\" \"\$@\"" > $INSTALL_DIR/$PROG
chmod +x $INSTALL_DIR/$PROG

echo "Installation complete"
echo ""
echo "Type any key to continue"
read anykey
