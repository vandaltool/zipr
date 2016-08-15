package main;

import (
	"fmt"
	"flag"
	"os"
	"database/sql"
	_ "github.com/go-sql-driver/mysql"
);

func connect_to_db(user string, password string, database string, host string) (*sql.DB, error) {
	db, err := sql.Open("mysql",
	                    user + ":" + password +
	                    "@tcp(" + host + ":3306)/" +
	                    database + "?parseTime=true");
	if err != nil {
		return nil, err;
	}
	err = db.Ping();
	return db, err;
}

/*
 * This is a named two stage pipeline architecture. 
 * _named_ because the channels between stages
 * are not anonymous, although they could easily be.
 */
func main() {
	/*
	 * Control connections to the DB for DB extraction.
	 */
	db_host := flag.String("db_host", "127.0.0.1", "Database host.");
	db_db := flag.String("db_db", "pcap", "Database name.");
	db_user := flag.String("db_user", "pcap", "Database username.");
	db_password := flag.String("db_password", "pcap", "Database password.");
	db_table := flag.String("db_table", "pcap", "Database table name.");

	csid := flag.String("csid", "", "Csid to clean.");

	flag.Parse();

	var db *sql.DB;
	var db_err error;
	var delete_statement *sql.Stmt;
	var delete_statement_err error;

	if db, db_err = connect_to_db(*db_user,
	                              *db_password,
	                              *db_db,
																*db_host); db_err != nil{
		fmt.Printf("Error connecting to the database: %v\n", db_err);
		os.Exit(-1);
	}

	if *csid == "" {
		csid = nil;
	}

	defer db.Close();

	if csid != nil {
		/*
		 * We are going to delete the data only for a
		 * particular csid.
		 */
		if delete_statement,delete_statement_err = 
		   db.Prepare(fmt.Sprintf("delete from %v where cbid=?", *db_table));
		   delete_statement_err != nil {
			fmt.Printf("Error preparing the delete statement: %v\n", delete_statement_err);
			os.Exit(-1);
		}
		if _, err := delete_statement.Exec(*csid); err != nil {
			fmt.Printf("Error executing the delete statement: %v\n", err);
			os.Exit(-1);
		}
	} else {
		/*
		 * We are going to delete all the data from the db.
		 */
		if delete_statement,delete_statement_err = 
		   db.Prepare(fmt.Sprintf("delete from %v", *db_table));
		   delete_statement_err != nil {
			fmt.Printf("Error preparing the delete statement: %v\n", delete_statement_err);
			os.Exit(-1);
		}
		if _, err := delete_statement.Exec(); err != nil {
			fmt.Printf("Error executing the delete statement: %v\n", err);
			os.Exit(-1);
		}
	}
}
