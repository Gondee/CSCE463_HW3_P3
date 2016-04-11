#pragma once
#include "stdafx.h"


#define MAGIC_PORT 22345 // receiver listens on this port
#define MAX_PKT_SIZE (1500-28)
#define STATUS_OK 0 // no error
#define ALREADY_CONNECTED 1 // second call to ss.Open() without closing connection
#define NOT_CONNECTED 2 // call to ss.Send()/Close() without ss.Open()
#define INVALID_NAME 3 // ss.Open() with targetHost that has no DNS entry
#define FAILED_SEND 4 // sendto() failed in kernel
#define TIMEOUT 5 // timeout after all retx attempts are exhausted
#define FAILED_RECV 6 // recvfrom() failed in kernel

#define MAGIC_PROTOCOL 0x8311AA
#define MAGIC_PORT 22345 



#pragma pack(push,1)
class Flags {
public:
	DWORD reserved : 5; // must be zero
	DWORD SYN : 1;
	DWORD ACK : 1;
	DWORD FIN : 1;
	DWORD magic : 24;
	Flags() { memset(this, 0, sizeof(*this)); magic = MAGIC_PROTOCOL; }
};

class LinkProperties {
public:
	// transfer parameters
	float RTT; // propagation RTT (in sec)
	float speed; // bottleneck bandwidth (in bits/sec)
	float pLoss[2]; // probability of loss in each direction
	DWORD bufferSize; // buffer size of emulated routers (in packets)
	LinkProperties() { memset(this, 0, sizeof(*this)); }
};

class SenderDataHeader {
public:
	Flags flags;
	DWORD seq; // must begin from 0
};

class ReceiverHeader {
public:
	Flags flags;
	DWORD recvWnd; // receiver window for flow control (in pkts)
	DWORD ackSeq; // ack value = next expected sequence
};

class SenderSynHeader {
public:
	SenderDataHeader sdh;
	LinkProperties lp;
};
#pragma pack(pop) // restores old packing 


class StatsParameters {
public:
	HANDLE mutex;
	int base = 0;
	int dataACKd = 0;
	int nextSequence = 0;
	int timeouts = 0;
	int fastTransmissions = 0;
	int windowSize = 0;
	int goodput = 0;
	double RTT = 0;
	int checksum = 0;
	ULONGLONG startTime;

};


class SenderSocket {
	bool CONNECTION_MESSAGES = false;

	ULONGLONG startTime;
	int SendPacket(char * packet, int size, std::string IP, char * buf);
	//bool SendACK(char * packet, int size, std::string IP, char * buf);
	long double RTO = 1000;
	std::string ip_globe = "";
	bool sent = false;
	int window = 0;
	DWORD dChecksum = 0;
	
	SOCKET sock;
	struct sockaddr_in local;
	struct hostent *rmt;
	struct sockaddr_in remote;
	//WSADATA wsaData;
	

public:

	long double RTT = 0;

	int connectpacketsize = 1;
	std::string getIP() { return ip_globe; }
	ULONGLONG getStartTime() { return startTime; }

	SenderSocket();
	int Open(char * targethost,DWORD Magic_Port,int sender_window,LinkProperties * l);
	int Close(char * targethost, DWORD Magic_Port, int sender_window, LinkProperties * l);
	int Send(char * packet, int size, int seq, StatsParameters * p);
	int SendData(char * packet, int size, char * buf, int seq);


};