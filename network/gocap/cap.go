package main;

import (
	"fmt"
	"net"
	"github.com/google/gopacket"
	"github.com/google/gopacket/pcap"
	"github.com/google/gopacket/layers"
	"github.com/google/gopacket/pcapgo"
	cgc_utils "cgc_net/utils"
	"flag"
	"os"
	"os/signal"
	"time"
	"regexp"
	"database/sql"
	_ "github.com/go-sql-driver/mysql"
);


type filter struct {
	SrcPorts, DstPorts		[]layers.UDPPort;
	SrcHosts, DstHosts		[]net.IP;
}
var debug bool;

func log_cgc_packet(packet cgc_utils.IdsPacket, statement sql.Stmt) {
	if _, err := statement.Exec(packet.Csid, packet.ConnectionID, fmt.Sprintf("%v", packet.ConnectionSide), packet.MessageID, packet.Message); err != nil {
		fmt.Printf("Exec() error: %v\n", err);
	}
}

func fs_extract_output(input <-chan cgc_utils.PacketPair, output chan<- cgc_utils.Packet, executive chan<- int, program_executive chan int, csid_regex *regexp.Regexp, output_path string) {
	executive<-0;
	return;
}

func db_extract_output(input <-chan cgc_utils.PacketPair, output chan<- cgc_utils.Packet, executive chan<- int, program_executive chan int, csid_regex *regexp.Regexp, database *sql.DB) {
	var counter int;
	var statement *sql.Stmt;
	var statement_err error;
	var stopping = false;
	var discard = false;

	counter = 0;

	if statement, statement_err = database.Prepare("insert into pcap (cbid, conversation, side, message, contents) VALUES (?,?,?,?,?)"); statement_err != nil {
		fmt.Printf("Prepare() error: %v\n", statement_err);
		executive<-0;
		return;
	}

	for stopping == false {
		select {
			case pair, ok := <-input:
				if !ok {
					stopping = true;
					break;
				}
				if !discard && csid_regex.MatchString(pair.CgcPacket.Csid) {
					log_cgc_packet(pair.CgcPacket, *statement);
				}
				counter++;
			case _, ok := <-program_executive:
				/*
				 * We don't want to "stop" since that
				 * might back up the pipeline and cause
				 * hanging when we try to stop other
				 * go funcs, so we'll just signal to
				 * flush.
				 */
				if !ok {
					discard = true;
					break;
				}
		}
	}
	if debug {
		fmt.Printf("Extract output counter: %d\n", counter);
	}
	executive<-0;
}

func pcap_output(input <-chan cgc_utils.PacketPair, output chan<- cgc_utils.Packet, executive chan<- int, path string) {
	counter := 0;
	var file *os.File;
	var err error;
	do_sync := make(chan int, 1);
	stopping := false;

	filename := path + "/cap.pcap";

	if file, err = os.OpenFile(filename, os.O_WRONLY|os.O_TRUNC, os.ModePerm);
	   err != nil {
		open_error := err.(*os.PathError);
		if os.IsNotExist(err) {
			if debug {
				fmt.Printf("Have to make the directory.\n");
				fmt.Printf("open_error: %v\n", open_error);
			}
			if mkdir_err := os.MkdirAll(path, os.ModePerm|os.ModeDir);
			   mkdir_err != nil {
				fmt.Printf("Error making the '%s' directory.\n", path);
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
		fmt.Printf("Could not create/open %s/cap.pcap. Quitting.\n", path);
		return;
	}

	/*
 	 * Create a routine that will wake up
	 * this function ever 10 seconds to do
	 * a Sync() on the file to flush its 
	 * contents to disk.
	 */
	go func() {
		for {
			time.Sleep(10*time.Second);
			do_sync<-1;
		}
	}();

	pcap_writer:= pcapgo.NewWriter(file);
	pcap_writer.WriteFileHeader(1024,layers.LinkTypeEthernet);

	for stopping == false {
		select {
		case pair, ok := <- input:
			if !ok {
				stopping = true;
				break;
			}
			log_raw_packet(pair.RawPacket, pcap_writer);
			counter++;
		case <- do_sync:
			if (debug) {
				fmt.Printf("Syncing.\n");
			}
			file.Sync();
		}
	}
	if debug {
		fmt.Printf("pcap output counter: %d\n", counter);
	}
	file.Close();
	executive<-0;
}

func log_raw_packet(pkt cgc_utils.Packet, writer *pcapgo.Writer) {
	if write_error := writer.WritePacket(pkt.Metadata().CaptureInfo,pkt.Data());
	   write_error != nil {
		fmt.Printf("write_error: %v\n", write_error);
	}
}

func parser(number int, input <-chan cgc_utils.Packet, output chan<- cgc_utils.PacketPair, executive chan<- int, filter *filter) {
	if debug {
		fmt.Printf("Starting parser %d.\n", number);
	}

	stopping := false;
	counter := 0;

	for stopping == false {
		select {
			case i, ok := <- input:
				if !ok {
					stopping = true;
					break;
				}
				var ipv4_layer gopacket.Layer;
				var udp_layer gopacket.Layer;
				var udp *layers.UDP;
				var ip *layers.IPv4;
				var cgc_packet cgc_utils.IdsPacket;
				var err error;

				if ipv4_layer = i.Layer(layers.LayerTypeIPv4); ipv4_layer == nil {
					if debug {
						fmt.Printf("No %v layer.\n", layers.LayerTypeIPv4);
					}
				}
				if udp_layer = i.Layer(layers.LayerTypeUDP); udp_layer == nil {
					if debug {
						fmt.Printf("No %v layer.\n", layers.LayerTypeUDP);
					}
				}

				/*
				 * Unless there is an IP and UDP layer, we don't care.
				 */
				if ipv4_layer == nil || udp_layer == nil {
					continue;
				}

				/*
				 * Check filter conditions if there are any.
				 */
				if filter != nil {
					udp, _ = udp_layer.(*layers.UDP)
					ip, _ = ipv4_layer.(*layers.IPv4);
					if filter.DstPorts != nil && !cgc_utils.MatchAnyPort(udp.DstPort,filter.DstPorts){
						continue;
					}
					if filter.SrcPorts != nil && !cgc_utils.MatchAnyPort(udp.DstPort,filter.DstPorts){
						continue;
					}
					if filter.SrcHosts != nil && !cgc_utils.MatchAnyHost(ip.SrcIP,filter.SrcHosts){
						continue;
					}
					if filter.DstHosts != nil && !cgc_utils.MatchAnyHost(ip.DstIP,filter.DstHosts){
						continue;
					}
				}

				/*
				 * Try to parse the packet.
				 */
				if cgc_packet, err = cgc_utils.ParseCgcPacket(udp.Payload); err != nil {
					fmt.Printf("ParseCgcPacket error: %v\n", err);
					continue;
				}

				/*
				 * Output!
				 */
				if output != nil {
					output <- cgc_utils.PacketPair{i, cgc_packet};
					counter++;
				}
		}
	}
	/*
	 * Sending our number back to the executive says that we are done processing.
	 */
	if debug {
		fmt.Printf("parser counter: %v\n", counter);
	}
	executive<-number;
}

func producer(stage_one chan<- cgc_utils.Packet,
              executive chan<- int,
              program_executive chan int,
              iface *string,
              filename *string,
              offline_forever bool) {

	var live_handle *pcap.Handle;
	var err error;
	var stopping bool;

	stopping = false;

	if filename != nil {
		live_handle, err = pcap.OpenOffline(*filename, offline_forever);
	} else {
		live_handle, err = pcap.OpenLive(*iface, 1024, false, pcap.BlockForever);
	}

	if err != nil {
		fmt.Printf("Error occurred attempting to open the input handle.\n");
		executive<-0
		return;
	}

	defer func() {
		live_handle.Close();
	}();

	live_source := gopacket.NewPacketSource(live_handle, live_handle.LinkType());
	packet_counter := 0;

	if debug {
		fmt.Printf("WARNING: We are in debug mode -- " +
			"limiting output to 5000 packets.\n");
	}
	for stopping == false {
		select {
			case _, ok := <-program_executive:
				if !ok {
					stopping = true;
					break;
				}
			case p, ok := <- live_source.Packets():
				if !ok {
					stopping = true;
					break;
				}
				stage_one <- p;
				if debug {
					if packet_counter>6000 {
						stopping = true;
						break;
					}
				}
				packet_counter++;
		}
	}

	/*
	 * Sending our number to the executive says that we are done producing.
	 */
	if debug {
		fmt.Printf("Packet counter: %v\n", packet_counter);
	}
	executive<-0;
}

func connect_to_db(user string, password string, database string) (*sql.DB, error) {
	db, err := sql.Open("mysql",
	                    user + ":" + password +
	                    "@unix(/var/run/mysqld/mysqld.sock)/" +
	                    database );
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
	//parser_c := make(chan packet, 500000);
	parser_c := make(chan cgc_utils.Packet);
	output_c := make(chan cgc_utils.PacketPair, 500000);
	producer_executive := make(chan int);
	parsers_executive := make(chan int);
	output_executive := make(chan int);
	program_executive := make(chan int);
	parsers := 1;
	outputs := 1;
	signal_c := make(chan os.Signal, 1);

	signal.Notify(signal_c, os.Interrupt);

	go func() {
		_ = <-signal_c
		if debug {
			fmt.Printf("Got SIGINT\n");
		}
		close(program_executive);
	}();

	var extract_csid_regex *regexp.Regexp;
	var compile_regex_err error;

	/*
	 * There can be only one producer since 
	 * reading from the source cannot be multiplexed.
	 */

	filter := &filter{};

	src_hosts := flag.String("src_host", "", "Source host");
	dst_hosts := flag.String("dst_host", "", "Destination host");
	src_ports := flag.String("src_port", "", "Source port");
	dst_ports := flag.String("dst_port", "", "Destination port");

	/*
	 * Capturing from a file or an interface.
	 */
	offline_filename := flag.String("offline", "", "Name of file to use as source.");
	online_interface := flag.String("online", "", "Name of interface to use as source.");

	/*
	 * Extracting or not?
	 */
	extract := flag.Bool("e", false, "Extract output");

	/*
	 * What and how long to extract.
	 */
	extract_csid := flag.String("extract_csid", ".*", "CSID of messages to extract.");
	extract_forever := flag.Bool("extract_forever", false, "Wait forever when reading from an offline file.");

	/*
	 * Extracting into a database or file system.
	 */
	extract_to_fs := flag.Bool("f", false, "Extract to a FS.");
	extract_to_db := flag.Bool("d", false, "Extract to a DB.");

	/*
	 * Control location of capture output.
	 */
	capture_output := flag.String("capture_output", "", "Name capture output path.");

	/*
	 * Control FS extraction parameters.
	 */
	fs_extract_path := flag.String("fs_extract_path", "pcap", "Name path to place extracted packets.");

	/*
	 * Control connections to the DB for DB extraction.
	 */
	db_extract_db := flag.String("db_extract_db", "pcap", "Extract database name.");
	db_extract_user := flag.String("db_extract_user", "pcap", "Extract database username.");
	db_extract_password := flag.String("db_extract_password", "pcap", "Extract database password.");

	flag.Parse();

	/*
	 * convert defaults to nil!
	 */
	if *online_interface == "" {
		online_interface = nil;
	}
	if *offline_filename == "" {
		offline_filename = nil;
	}
	if *extract_csid == "" {
		extract_csid = nil;
	}
	if *capture_output == "" {
		capture_output = nil;
	}

	/*
	 * Check that exactly one of offline or online is configured.
	 */
	if online_interface == nil && offline_filename == nil {
		fmt.Printf("Must specify one of offline or online.\n");
		flag.PrintDefaults();
		os.Exit(-1);
	} else if online_interface != nil && offline_filename != nil {
		fmt.Printf("Can only specify offline OR online, not both.\n");
		flag.PrintDefaults();
		os.Exit(-1);
	}


	/*
	 * Check that exactly one of FS or DB extraction is configured
	 * when extracting.
	 */
	if *extract && (*extract_to_db == false && *extract_to_fs == false) {
		fmt.Printf("Must specify either output to FS or DB.\n");
		os.Exit(-1);
	}
	if *extract && (*extract_to_db == true && *extract_to_fs == true) {
		fmt.Printf("Can only specify output to FS OR DB, not both.\n");
		os.Exit(-1);
	}

	/*
	 * Warn about extracting live to a filesystem.
	 */
	if *extract && online_interface != nil {
		fmt.Printf("WARNING: Extracting pcap from a live capture \n" +
		           "         is not going to end well.\n");
	}

	/*
	 * Prepare a filter from the command line arguments.
	 */
	if *src_hosts != "" {
		if hosts, err := cgc_utils.ParseHosts(*src_hosts); err != nil {
			fmt.Printf("Error: %v\n", err);
			flag.PrintDefaults();
			os.Exit(-1);
		} else {
			filter.SrcHosts = hosts;
		}
	}

	if *dst_hosts != "" {
		if hosts, err := cgc_utils.ParseHosts(*dst_hosts); err != nil {
			fmt.Printf("Error: %v\n", err);
			flag.PrintDefaults();
			os.Exit(-1);
		} else {
			filter.DstHosts = hosts;
		}
	} else {
		/*
		 * By default we filter for 192.168.1.30
		 */
		filter.DstHosts = []net.IP{net.ParseIP("192.168.1.30"),};
	}

	if *dst_ports != "" {
		if ports, err := cgc_utils.ParsePorts(*dst_ports); err != nil {
			fmt.Printf("Error: %v\n", err);
			flag.PrintDefaults();
			os.Exit(-1);
		} else {
			filter.DstPorts = ports;
		}
	} else {
		filter.DstPorts = []layers.UDPPort{layers.UDPPort(1999)};
	}

	if *src_ports != "" {
		if ports, err := cgc_utils.ParsePorts(*src_ports); err != nil {
			fmt.Printf("Error: %v\n", err);
			flag.PrintDefaults();
			os.Exit(-1);
		} else {
			filter.SrcPorts = ports;
		}
	}

	if debug {
		fmt.Printf("DstPorts: %v\n", filter.DstPorts);
		fmt.Printf("SrcPorts: %v\n", filter.SrcPorts);
		fmt.Printf("SrcHosts: %v\n", filter.SrcHosts);
		fmt.Printf("DstHosts: %v\n", filter.DstHosts);
	}

	if (extract_csid != nil && *extract_csid != "") {
		/*
		 * We are going to support the extract_csid being a regular expression. 
		 * The first thing that we are going to do, then, is attempt
		 * to compile and make sure that it is valid.
		 */
		if extract_csid_regex, compile_regex_err=regexp.Compile(*extract_csid);
			compile_regex_err != nil {
			fmt.Printf("Invalid extract_csid regular expression: %v\n",
				compile_regex_err);
			os.Exit(-1);
		}
	}

	for i:=0;i<parsers;i++ {
		/*
		 * Start an individual parser.
		 */
		go parser(i,parser_c,output_c,parsers_executive,filter);
	}

	if *extract {
		if (*extract_to_db) {
			var db *sql.DB;
			var db_err error;

			if db, db_err = connect_to_db(*db_extract_user,
			                              *db_extract_password,
			                              *db_extract_db); db_err != nil{
				fmt.Printf("Error connecting to extraction database: %v\n", db_err);
				os.Exit(-1);
			}

			/*
			 * When we are extracting into the database, please increase
			 * the number of output threads.
			 */
			outputs = 25;
			for i:=0; i<outputs; i++ {
				go db_extract_output(output_c,
				                     nil,
				                     output_executive,
				                     program_executive,
				                     extract_csid_regex,
				                     db);
			}
		} else {
			/*
			 * extract to file.
			 */
			go fs_extract_output(output_c,
			                     nil,
			                     output_executive,
			                     program_executive,
			                     extract_csid_regex,
			                     *fs_extract_path);
		}
	} else {
		/*
		 * Just capture the output into a single file.
		 */
		go pcap_output(output_c,
		               nil,
		               output_executive,
		               *capture_output);
	}

	go producer(parser_c,
	            producer_executive,
	            program_executive,
	            online_interface,
	            offline_filename,
	            *extract_forever);

	/*
	 * Now, wait for the producer to complete. When
	 * that happens, we know that we can start to drain
	 * each stage.
	 */

	closed_worker := <- producer_executive;
	if debug {
		fmt.Printf("Closed producer %d.\n", closed_worker);
	}

	/*
	 * Everything that needs to be, should be in 
	 * the queue/channel to go to stage 1 (parser_c) 
	 * by now.
	 */
	close(parser_c);

	for i:=0;i<parsers;i++ {
		closed_worker := <- parsers_executive;
		if debug {
			fmt.Printf("Closed parser %d.\n", closed_worker);
		}
	}
	/*
	 * Everything that needs to be, should be in 
	 * the queue/channel to go to stage 2 (output_c) 
	 * by now.
	 */
	close(output_c);

	for i:=0;i<outputs;i++ {
		closed_worker = <- output_executive;
		if debug {
			fmt.Printf("Closed output %d.\n", closed_worker);
		}
	}
	close(parsers_executive);
	close(output_executive);
}
