# populate DB w/ test entries
echo "Populate ZeST tables with test entries"
psql -U $Z_PGUSER -h $Z_PGHOST -p $Z_PGPORT -d $Z_PGDATABASE -f $PEASOUP_HOME/tools/db/zest/populate.sql
