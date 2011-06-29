#!/bin/bash

psql << 'EOF'
CREATE LANGUAGE plpgsql;
CREATE OR REPLACE FUNCTION drop_tables(username IN VARCHAR) RETURNS void AS $$
DECLARE
    statements CURSOR FOR
        SELECT tablename FROM pg_tables
        WHERE tableowner = username;
BEGIN
    FOR stmt IN statements LOOP
        EXECUTE 'DROP TABLE IF EXISTS ' || quote_ident(stmt.tablename) || ' CASCADE;';
    END LOOP;
END;
$$
LANGUAGE plpgsql;
EOF

psql -c "SELECT drop_tables('$PGUSER');"
