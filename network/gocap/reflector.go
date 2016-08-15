package main;

import (
	"fmt"
	"net"
	"github.com/google/gopacket"
	"github.com/google/gopacket/pcap"
	"github.com/google/gopacket/layers"
	"flag"
	"cgc_net/utils"
	"os"
);

type packet gopacket.Packet;

type filter struct {
	SrcPorts, DstPorts		[]layers.UDPPort;
	SrcHosts, DstHosts		[]net.IP;
}

var debug bool;

func output(number int, input <-chan []byte, output chan<- packet, executive chan<- int, iface string) {
	counter := 0;
	var live_handle *pcap.Handle;
	var err error;

	if live_handle, err = pcap.OpenLive(iface, 65535, false, pcap.BlockForever);
	   err != nil {
		panic(fmt.Sprintf("Could not open the output interface: %v\n", err));
		executive<-number;
		return;
	}

	defer func() {
		live_handle.Close();
	}()

	for cgc_packet := range input {
		live_handle.WritePacketData(cgc_packet);
		counter++;
	}
	if debug {
		fmt.Printf("counter: %d\n", counter);
	}
	executive<-number;
}

func rewriter(number int, input <-chan packet, output chan<- []byte, executive chan<- int, filter *filter) {
	if debug {
		fmt.Printf("Starting rewriter %d.\n", number);
	}
	for i := range input {
		var ethernet_layer gopacket.Layer;
		var ipv4_layer gopacket.Layer;
		var udp_layer gopacket.Layer;
		var udp *layers.UDP;
		var ip *layers.IPv4;
		var ethernet *layers.Ethernet;

		if ethernet_layer = i.Layer(layers.LayerTypeEthernet);ethernet_layer==nil {
			if debug {
				fmt.Printf("No %v layer.\n", layers.LayerTypeIPv4);
			}
		}
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
		 * Unless there is an ethernet, IP and UDP layer, we don't care.
		 */
		if ethernet_layer == nil || ipv4_layer == nil || udp_layer == nil {
			continue;
		}

		ethernet, _ = ethernet_layer.(*layers.Ethernet)
		udp, _ = udp_layer.(*layers.UDP)
		ip, _ = ipv4_layer.(*layers.IPv4);

		/*
		 * Check filter conditions if there are any.
		 */
		if filter != nil {
			if filter.DstPorts != nil && !utils.MatchAnyPort(udp.DstPort,filter.DstPorts){
				continue;
			}
			if filter.SrcPorts != nil && !utils.MatchAnyPort(udp.DstPort,filter.DstPorts){
				continue;
			}
			if filter.SrcHosts != nil && !utils.MatchAnyHost(ip.SrcIP,filter.SrcHosts){
				continue;
			}
			if filter.DstHosts != nil && !utils.MatchAnyHost(ip.DstIP,filter.DstHosts){
				continue;
			}
		}
		/*
		 * Rewrite the packet.
		 */
		ethernet.DstMAC = layers.EthernetBroadcast;
		ip.DstIP = net.IPv4(0xff, 0xff, 0xff, 0xff);
		buffer := gopacket.NewSerializeBuffer();
		options := gopacket.SerializeOptions{};
		gopacket.SerializeLayers(buffer, options,
			ethernet,
			ip,
			udp,
			gopacket.Payload(udp.Payload));

		/*
		 * Output!
		 */
		if output != nil {
			output <- buffer.Bytes();
		}
	}

	/*
	 * Sending our number back to the executive says that we are done processing.
	 */
	executive<-number;
}

func producer(number int, stage_one chan<- packet, executive chan<- int, iface *string, filename *string) {
	var live_handle *pcap.Handle;
	var err error;

	if filename != nil {
		live_handle, err = pcap.OpenOffline(*filename);
	} else {
		live_handle, err = pcap.OpenLive(*iface, 65535, false, pcap.BlockForever);
	}

	if err != nil {
		panic(fmt.Sprintf("Error occurred attempting to open the input handle.\n"));
		return;
	}

	defer func() {
		live_handle.Close();
	}()

	if debug {
		fmt.Printf("WARNING: We are in debug mode -- " +
			"limiting output to 5000 packets.\n");
	}

	live_source := gopacket.NewPacketSource(live_handle, live_handle.LinkType());
	packet_counter := 0;
	for p := range live_source.Packets() {
		stage_one <- p;
		if debug {
			if packet_counter>5000 {
				break;
			}
		}
		packet_counter++;
	}

	/*
	 * Sending our number to the executive says that we are done producing.
	 */
	executive<-number;
	return;
}
/*
 * This is a named two stage pipeline architecture. 
 * _named_ because the channels between stages
 * are not anonymous, although they could easily be.
 */
func main() {
	rewriter_c := make(chan packet);
	output_c := make(chan []byte);
	producers_executive := make(chan int);
	rewriters_executive := make(chan int);
	outputs_executive := make(chan int);
	rewriters := 10;
	outputs := 10;
	producers := 1;

	debug = true;

	filter := &filter{};

	src_hosts := flag.String("src_host", "", "Source host");
	dst_hosts := flag.String("dst_host", "", "Destination host");
	src_ports := flag.String("src_port", "", "Source port");
	dst_ports := flag.String("dst_port", "", "Destination port");

	offline_filename := flag.String("offline", "", "Name of file to use as source.");
	online_interface := flag.String("online", "", "Name of interface to use as source.");
	output_interface := flag.String("output", "", "Name of interface to use for output.");

	flag.Parse();

	if *offline_filename == ""  {
		offline_filename = nil;
	}
	if *online_interface == "" {
		online_interface = nil;
	}

	if *output_interface == "" {
		fmt.Printf("Must specify and output interface.\n");
		flag.PrintDefaults();
		os.Exit(-1);
	}

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
		if hosts, err := utils.ParseHosts(*src_hosts); err != nil {
			fmt.Printf("Error: %v\n", err);
			flag.PrintDefaults();
			os.Exit(-1);
		} else {
			filter.SrcHosts = hosts;
		}
	}

	if *dst_hosts != "" {
		if hosts, err := utils.ParseHosts(*dst_hosts); err != nil {
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
		if ports, err := utils.ParsePorts(*dst_ports); err != nil {
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
		if ports, err := utils.ParsePorts(*src_ports); err != nil {
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

	for i:=0;i<rewriters;i++ {
		/*
		 * Start an individual worker.
		 */
		go rewriter(i,rewriter_c,output_c,rewriters_executive,filter);
	}

	for i:=0;i<outputs;i++ {
		/*
		 * Start stage two workers.
		 */
		go output(i, output_c, nil, outputs_executive, *output_interface);
	}

	for i:=0; i<producers; i++ {
		go producer(i, rewriter_c, producers_executive, online_interface, offline_filename);
	}

	/*
	 * Now, wait for the producers to complete. When
	 * that happens, we know that we can start to drain
	 * each stage.
	 */

	for i:=0; i<producers; i++ {
		closed_worker := <- producers_executive;
		if debug {
			fmt.Printf("Closed producer %d.\n", closed_worker);
		}
	}
	/*
	 * Everything that needs to be, should be in 
	 * the queue/channel to go to stage 1 (rewriter_c) 
	 * by now.
	 */
	close(rewriter_c);

	for i:=0;i<rewriters;i++ {
		closed_worker := <- rewriters_executive;
		if debug {
			fmt.Printf("Closed rewriter %d.\n", closed_worker);
		}
	}
	/*
	 * Everything that needs to be, should be in 
	 * the queue/channel to go to stage 2 (output_c) 
	 * by now.
	 */
	close(output_c);

	for i:=0;i<outputs;i++ {
		closed_worker := <- outputs_executive;
		if debug {
			fmt.Printf("Closed output %d.\n", closed_worker);
		}
	}

	close(rewriters_executive);
	close(outputs_executive);
}
