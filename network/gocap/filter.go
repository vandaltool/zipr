package main;

import (
	"fmt"
	"net"
	"github.com/google/gopacket"
	"github.com/google/gopacket/pcap"
	"github.com/google/gopacket/layers"
	cgc_utils "cgc_net/utils"
	"flag"
	"os"
	"os/signal"
	"github.com/kr/beanstalk"
	"time"
	"gopkg.in/yaml.v2"
);


type FilterDetectMsg struct {
	Id string;
	Csid string;
};
type FilterHeartbeatMsg struct {
	Id string;
};

type SeenCsid struct {
	Csid string;
	Count int;
};
type SeenCsids []SeenCsid;

type filter struct {
	SrcPorts, DstPorts		[]layers.UDPPort;
	SrcHosts, DstHosts		[]net.IP;
}
var debug bool;

func has_flag_page_addr(data []byte) bool {
	/*
	 * the pattern is three bytes long. So, if
	 * the data isn't three bytes, we know it's
	 * not there.
	 */
	if len(data) < 3 {
		return false;
	}
	for i := 0; i<len(data)-3; i++ {
		/*
		 * flag page between 0x4347C000 and 0x4347CF00
		 */
		/*
		 * first byte:
		 */
		if data[i] < 0xC0 || data[i] > 0xCF {
			continue;
		}
		/*
		 * second byte:
		 */
		if data[i+1] != 0x47 {
			continue;
		}
		/*
		 * third byte: (0x43)
		 */
		if data[i+2] != 0x43 {
			continue;
		}
		return true;
	}
	return false;
}

func (seen_csids SeenCsids) FindCsid(csid string) (*SeenCsid) {
	for i := 0; i<len(seen_csids); i++ {
		if seen_csids[i].Csid == csid {
			return &seen_csids[i];
		}
	}
	return nil;
}

func connect_to_q(existing_conn *beanstalk.Conn,
                  existing_tube *beanstalk.Tube,
                  addr string,
                  tube string) (new_conn *beanstalk.Conn,
                                new_tube *beanstalk.Tube,
                                err error) {
	if existing_conn != nil {
		if debug {
			fmt.Printf("Reusing existing connection/tube.\n");
		}
		return existing_conn, existing_tube, nil;
	}
	if new_conn, err = beanstalk.Dial("tcp", addr); err != nil {
		if debug {
			fmt.Printf("Could not connect.\n");
		}
		return nil, nil, err;
	}
	if debug {
		fmt.Printf("Connected successfully.\n");
	}
	return new_conn, &beanstalk.Tube{new_conn, tube}, nil;
}

func beanstalk_output(input <-chan string, output chan<- string, executive chan<- int, program_executive chan int, beanstalk_addr string) {
	var stopping bool;
	var discard bool;
	var counter int;
	var heartbeat_seconds int;
	var max_alerts int;

	var beanstalk_conn *beanstalk.Conn;
	var gamemaster_tube *beanstalk.Tube;
	var beanstalk_err error;

	beanstalk_conn = nil;
	gamemaster_tube = nil;
	beanstalk_err = nil;

	heartbeat_c := make(chan int);

	seen_csids := make(SeenCsids, 0, 10);

	stopping = false;
	discard = false;
	counter = 0;
	heartbeat_seconds = 30;
	max_alerts = 3;

	defer func() {
		if beanstalk_conn != nil {
			beanstalk_conn.Close();
		}
	}();

	/*
	 * Create channel that will pop every thirty
	 * seconds that will signal us to send a heart
	 * beat to the gamemaster.
	 */
	go func() {
		for {
			time.Sleep(time.Duration(heartbeat_seconds)*time.Second);
			heartbeat_c<-1;
		}
	}();

	for stopping == false {
		select {
			case _, ok := <- heartbeat_c:
				if ok && !discard {
					msg := FilterHeartbeatMsg{"filter_heartbeat"};
					yaml_msg, _ := yaml.Marshal(msg);

					if beanstalk_conn, gamemaster_tube, beanstalk_err =
					   connect_to_q(beanstalk_conn,
					                gamemaster_tube,
					                beanstalk_addr,
					                "gamemaster");
					   beanstalk_err != nil {
						fmt.Printf("Could not (re)connect: %v\n", beanstalk_err);
						beanstalk_conn = nil;
					}

					if beanstalk_conn != nil {
						if _, err := gamemaster_tube.Put(yaml_msg, 1, 0, 0); err != nil {
							fmt.Printf("Error putting heartbeat message.\n");
							beanstalk_conn = nil;
						} else if debug {
							fmt.Printf("Sending heartbeat!\n");
						}
					} else {
						fmt.Printf("Skipping heartbeat send because we cannot reach q.\n");
					}
				}
			case csid, ok := <-input:
				if !ok {
					stopping = true;
					break;
				}
				if !discard {
					/*
					 * Check to see if we have already sent enough
					 * detects for this csid.
					 */
					if seen_csid := seen_csids.FindCsid(csid);
					   seen_csid == nil || seen_csid.Count<max_alerts {
						if seen_csid == nil {
							seen_csids = append(seen_csids, SeenCsid{csid, 1});
						} else {
							seen_csid.Count++;
						}
						msg := &FilterDetectMsg{"filter_detect", csid};
						yaml_msg, _ := yaml.Marshal(msg);

						if beanstalk_conn, gamemaster_tube, beanstalk_err =
						   connect_to_q(beanstalk_conn,
						                gamemaster_tube,
						                beanstalk_addr,
						                "gamemaster");
						   beanstalk_err != nil {
							fmt.Printf("Could not (re)connect: %v\n", beanstalk_err);
							beanstalk_conn = nil;
						}

						if beanstalk_conn != nil {
							if _, err := gamemaster_tube.Put(yaml_msg, 1, 0, 0); err != nil {
								fmt.Printf("Error putting detect message.\n");
								beanstalk_conn = nil;
							} else if debug {
								fmt.Printf("Sending detect!\n");
							}

						} else {
							fmt.Printf("Skipping detect because we cannot reach q.\n");
						}
					}
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
		fmt.Printf("Beanstalk output counter: %d\n", counter);
	}
	executive <- 0;
}

func output_filter(input <-chan cgc_utils.PacketPair, output chan<- string, executive chan<- int, program_executive chan int) {
	var stopping bool;
	var discard bool;
	var counter int;

	stopping = false;
	discard = false;
	counter = 0;

	for stopping == false {
		select {
			case pair, ok := <-input:
				if !ok {
					stopping = true;
					break;
				}
				if !discard && has_flag_page_addr(pair.CgcPacket.Message) {
					/*
					 * Alert the gamemaster that there is a match!
					 */
					output<-pair.CgcPacket.Csid;
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
		fmt.Printf("Filter output counter: %d\n", counter);
	}
	executive<-0;
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
		fmt.Printf("Parser output counter: %v\n", counter);
	}
	executive<-number;
}

func producer(stage_one chan<- cgc_utils.Packet,
              executive chan<- int,
              program_executive chan int,
              iface *string,
              filename *string) {

	var live_handle *pcap.Handle;
	var err error;
	var stopping bool;

	stopping = false;

	if filename != nil {
		live_handle, err = pcap.OpenOffline(*filename, false);
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
					if debug {
						fmt.Printf("program_executive not ok; stopping == true\n");
					}
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
					if packet_counter>60000 {
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

/*
 * This is a named two stage pipeline architecture. 
 * _named_ because the channels between stages
 * are not anonymous, although they could easily be.
 */
func main() {
	//debug = true;
	parser_c := make(chan cgc_utils.Packet);
	output_c := make(chan cgc_utils.PacketPair, 500000);
	beanstalk_c := make(chan string, 500000);
	producer_executive := make(chan int);
	parsers_executive := make(chan int);
	output_executive := make(chan int);
	program_executive := make(chan int);
	beanstalk_executive := make(chan int);
	parsers := 1;
	output_filters := 1;
	signal_c := make(chan os.Signal, 1);

	signal.Notify(signal_c, os.Interrupt);

	go func() {
		_ = <-signal_c
		if debug {
			fmt.Printf("Got SIGINT\n");
		}
		close(program_executive);
	}();

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

	beanstalk_addr := flag.String("beanstalk", "127.0.0.1:11300", "IP:PORT of beanstalk server.");

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
	if *beanstalk_addr == "" {
		beanstalk_addr = nil;
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

	go beanstalk_output(beanstalk_c,
	                    nil,
	                    beanstalk_executive,
	                    program_executive,
	                    *beanstalk_addr);

	for i := 0; i<output_filters; i++ {
		/*
		 * Start an individual filter.
		 */
		go output_filter(output_c,beanstalk_c,output_executive,program_executive);
	}

	for i:=0;i<parsers;i++ {
		/*
		 * Start an individual parser.
		 */
		go parser(i,parser_c,output_c,parsers_executive,filter);
	}

	go producer(parser_c,
	            producer_executive,
	            program_executive,
	            online_interface,
	            offline_filename);

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

	for i:=0;i<output_filters;i++ {
		closed_worker = <- output_executive;
		if debug {
			fmt.Printf("Closed output %d.\n", closed_worker);
		}
	}

	/* 
	 * The last step is to close the beanstalk q.
	 */
	close(beanstalk_c);
	closed_worker = <- beanstalk_executive;
	if debug {
		fmt.Printf("Closed beanstalk %d\n", closed_worker);
	}

	close(parsers_executive);
	close(output_executive);
}
