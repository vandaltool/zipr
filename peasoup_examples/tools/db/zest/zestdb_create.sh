# Setup the Zest database tables
echo "create ZeST tables"
psql -U $Z_PGUSER -h $Z_PGHOST -p $Z_PGPORT -d $Z_PGDATABASE -f $PEASOUP_HOME/tools/db/zest/zest.create.tbl
