package main;

import (
	"fmt"
	cgc_utils "cgc_net/utils"
	"flag"
	"os"
	"os/signal"
	"database/sql"
	_ "github.com/go-sql-driver/mysql"
	"time"
);

var debug bool;

func extract_cgc_message(cbid string,
                         conversation int,
                         side cgc_utils.Side,
                         message int,
                         contents []byte,
                         path string) {
	var file *os.File;
	var err error;

	packet_path := path + "/" +
	               cbid + "/" +
	               fmt.Sprintf("%d", conversation) + "/" +
	               fmt.Sprintf("%v", side) + "/";
	filename := packet_path + fmt.Sprintf("%d", message) + ".input";

	if file, err = os.OpenFile(filename, os.O_WRONLY|os.O_TRUNC, os.ModePerm);
	   err != nil {
		open_error := err.(*os.PathError);
		if os.IsNotExist(err) {
			if debug {
				fmt.Printf("Have to make the directory.\n");
			}
			if mkdir_err := os.MkdirAll(packet_path, os.ModePerm|os.ModeDir);
			   mkdir_err != nil {
				fmt.Printf("Error making the packet_path directory.\n");
			}
		} else {
			fmt.Printf("open_error: %v\n", open_error);
		}
		/*
		 * Now, try to open the file again.
		 */
		if file, err = os.OpenFile(filename,
		                           os.O_WRONLY|os.O_CREATE|os.O_TRUNC,
		                           os.ModePerm);
		   err != nil {
			fmt.Printf("err: %v\n", err);
		}
	}

	if file == nil {
		fmt.Printf("file is nil. We are going to quit.\n");
		return;
	}

	if write_len, write_error := file.Write(contents);
	   write_len != len(contents) || write_error != nil {
		fmt.Printf("write_error: %v\n", write_error);
	}
	file.Close();
}

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

func extract(extract_exec <-chan int,
             program_exec chan<- int,
             db *sql.DB,
             csid string,
             path string,
             forever bool) {

	var statement *sql.Stmt;
	var rows *sql.Rows;
	var statement_err error;

	var cbid string;
	var conversation, message int;
	var side cgc_utils.Side;
	var contents []byte;
	var timestamp time.Time;

	var stopping = false;

	var select_statement = `select 
	                        cbid,conversation,side,message,contents,message_time
	                        from pcap where cbid=? and message_time>?`;

	if statement, statement_err = db.Prepare(select_statement);
	   statement_err != nil {
		fmt.Printf("Prepare() error: %v\n", statement_err);
		program_exec<-1;
		return;
	}

	defer statement.Close();

	for {
		var results_counter = 0;
		rows, statement_err = statement.Query(csid, timestamp);
		if statement_err != nil {
			fmt.Printf("Error running Query(): %v\n", statement_err);
			program_exec<-1;
			return;
		}

		defer rows.Close();

		for rows.Next() && !stopping {
			rows.Scan(&cbid, &conversation, &side, &message, &contents, &timestamp);
			extract_cgc_message(cbid, conversation, side, message, contents, path);
			results_counter++;

			select {
				case <-extract_exec:
					stopping = true;
					break;
				default:
			}
		}

		if debug {
			fmt.Printf("results_counter: %v\n", results_counter);
		}

		select {
			case <-extract_exec:
				stopping = true;
				break;
			default:
		}

		if !forever || stopping {
			break;
		}

		if debug {
			fmt.Printf("Sleeping for 15 seconds before requerying.\n");
		}
		time.Sleep(time.Second*15);
	}

	program_exec<-1;
}

func main() {
	signal_c := make(chan os.Signal, 1);
	program_executive := make(chan int, 1);
	extract_executive := make(chan int, 1);

	signal.Notify(signal_c, os.Interrupt);

	go func() {
		_ = <-signal_c
		fmt.Printf("Got SIGINT\n");
		extract_executive<-1;
	}();


	/*
	 * What and how long to extract.
	 */
	extract_csid := flag.String("extract_csid", "", "CSID of messages to extract.");
	extract_forever := flag.Bool("extract_forever", false, "Wait forever when reading from an offline file.");

	/*
	 * Control FS extraction parameters.
	 */
	fs_extract_path := flag.String("fs_extract_path", "extracted", "Name path to place extracted packets.");

	/*
	 * Control connections to the DB for DB extraction.
	 */
	db_extract_db := flag.String("db_extract_db", "pcap", "Extract database name.");
	db_extract_user := flag.String("db_extract_user", "pcap", "Extract database username.");
	db_extract_password := flag.String("db_extract_password", "pcap", "Extract database password.");
	db_extract_host := flag.String("db_extract_host", "localhost", "Extract database host.");

	flag.Parse();

	if *extract_csid == "" {
		fmt.Printf("Must specify a CSID for extraction. No wildcards allowed.\n");
		flag.PrintDefaults();
		os.Exit(-1);
	}

	if debug && *extract_forever {
		fmt.Printf("Extracting forever.\n");
	}
	if debug {
		fmt.Printf("Extracting to %v\n", *fs_extract_path);
	}

	db, err := connect_to_db(*db_extract_user,
	                         *db_extract_password,
	                         *db_extract_db,
	                         *db_extract_host);
	if err != nil {
		fmt.Printf("Could not connect to db: %v\n", err);
		os.Exit(-1);
	}

	go extract(extract_executive,
	           program_executive,
	           db,
	           *extract_csid,
	           *fs_extract_path,
	           *extract_forever);

	<-program_executive;
	db.Close();
}
