#!/bin/bash

pushd $PEASOUP_UMBRELLA_DIR

if [ ! -z "$PGDATABASE" ]; then
	echo "Dropping database $PGDATABASE"
	dropdb $PGDATABASE

	echo "Restart and create $PGDATABASE"
	./postgres_setup.sh
fi

popd
