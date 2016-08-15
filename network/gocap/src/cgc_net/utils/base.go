package utils;

import (
	"net"
	"fmt"
	"strings"
	"errors"
	"strconv"
	"github.com/google/gopacket"
	"github.com/google/gopacket/layers"
	"encoding/binary"
);

type Side int;

func (side Side) String() string {
	switch side {
		case Client:
			return "Client";
		case Server:
			return "Server";
	}
	return "Unknown";
}

type Packet gopacket.Packet;

type IdsPacket struct {
	Csid string;
	ConnectionID uint32;
	MessageID uint32;
	MessageLen uint16;
	Message []byte;
	ConnectionSide Side;
}

type PacketPair struct {
	RawPacket Packet;
	CgcPacket IdsPacket;
}

/*
 * Scan is called by the SQL db library to convert
 * from the value pulled from the DB into a type
 * that it does not know about. We want to be able
 * to do this for Side, so we implement sql.Scanner.
 */
func (s *Side) Scan(value interface{}) error {
	/*
	 * type assertion:
	 */
	if raw, ok := value.([]byte); ok {
		string_s := string(raw[:]);
		if string_s == "Client" {
			*s = Client;
		} else {
			*s = Server;
		}
		return nil;
	}
	return errors.New("Cannot convert to side enum\n");
}

const (
	Client Side = iota;
	Server;
);

func MatchAnyHost(match net.IP, candidates []net.IP) bool {
	does_match := false;
	for _, i := range candidates {
		does_match = does_match || (match.Equal(i))
	}
	return does_match;
}

func MatchAnyPort(match layers.UDPPort, candidates []layers.UDPPort) bool {
	does_match := false;
	for _, i := range candidates {
		does_match = does_match || (match == i)
	}
	return does_match;
}
func ParseHosts(host_list string) (hosts []net.IP, err error) {
	for _, host_string := range strings.Split(host_list, ",") {
		var host net.IP;
		if host = net.ParseIP(strings.Trim(host_string," ")); host == nil {
			err = errors.New(fmt.Sprintf("Invalid host: %s.", host_string));
			break;
		}
		hosts = append(hosts, host);
	}
	return;
}

func ParsePorts(port_list string) (ports []layers.UDPPort, err error) {
	for _, port_string := range strings.Split(port_list, ",") {
		var port int;
		var conv_error error;
		if port, conv_error = strconv.Atoi(strings.Trim(port_string," "));
		   conv_error != nil && port_string != "" {
			err = errors.New(fmt.Sprintf("Invalid port: %s.", port_string));
			break;
		}
		ports = append(ports, layers.UDPPort(port));
	}
	return;
}

func ParseCgcPacket(packet []byte) (cgc_packet IdsPacket, err error) {
	packet_length := len(packet);
	packet_offset := 0;

	if (packet_offset+4) > packet_length {
		err = errors.New("Could not parse past first field.");
		return;
	}
	csid := binary.LittleEndian.Uint32(packet[packet_offset:]);
	cgc_packet.Csid = fmt.Sprintf("%x", csid);
	packet_offset += 4;

	if (packet_offset+4) > packet_length {
		err = errors.New("Could not parse past csid field.");
		return;
	}
	cgc_packet.ConnectionID = binary.LittleEndian.Uint32(packet[packet_offset:]);
	packet_offset += 4;

	if (packet_offset+4) > packet_length {
		err = errors.New("Could not parse past connection id field.");
		return;
	}
	cgc_packet.MessageID = binary.LittleEndian.Uint32(packet[packet_offset:]);
	packet_offset += 4;

	if (packet_offset+2) > packet_length {
		err = errors.New("Could not parse past message id field.");
		return;
	}
	cgc_packet.MessageLen = binary.LittleEndian.Uint16(packet[packet_offset:]);
	packet_offset += 2;

	if (packet_offset+1) > packet_length {
		err = errors.New("Could not parse past message len field.");
		return;
	}
	side := uint8(packet[packet_offset]);
	if side == 0 {
		cgc_packet.ConnectionSide = Server;
	} else {
		cgc_packet.ConnectionSide = Client;
	}
	packet_offset += 1;

	if (packet_offset) > packet_length {
		err = errors.New("Could not parse past side field.");
		return;
	}

	cgc_packet.Message = make([]byte,
	                          len(packet[packet_offset:]),
														len(packet[packet_offset:]));
	copy(cgc_packet.Message, packet[packet_offset:]);

	return;
}


