#include "stdafx.h"
#include "SenderSocket.h"






SenderSocket::SenderSocket() {
	startTime = GetTickCount64();
}

int SenderSocket::Open(char * targethost, DWORD Magic_Port, int sender_window, LinkProperties * l) {
	
	//Create The packets
	SenderDataHeader sdh; //Data Header
	Flags f;			//Set flags for operation
	f.SYN = 1;
	f.ACK = 0;
	f.FIN = 0;
	f.reserved = 0;
	sdh.flags = f;
	sdh.seq = 0;
	SenderSynHeader s;
	s.sdh = sdh;
	s.lp = *l;

	char * buf = new char[1600]; //Completly arbitrary

	int max_atts = 3;
	int att = 0;


	while (att <= max_atts) {
		ULONGLONG endTime = GetTickCount64();
		ULONGLONG startConTime = GetTickCount64();
		int sentt = SendPacket((char *)&s, sizeof(s), targethost, buf);
		if (sentt == TIMEOUT) {
			
			long double time = (endTime - startTime);
			if (CONNECTION_MESSAGES) {
				cout << "[" << setw(5) << (time) / 1000 << "]  --> SYN 0 (attempt " << att + 1 << " of " << max_atts << ", RTO " << RTO
					<< ") to " << ip_globe << endl;
			}
		}
		else if(sentt == STATUS_OK) {
			//ULONGLONG endTime = GetTickCount64();
			long double time = (endTime - startTime);

			ReceiverHeader * R = (ReceiverHeader*)buf;
			if (R->flags.SYN == 1 && R->flags.SYN == 1) {
				if (CONNECTION_MESSAGES) {
					cout << "[" << setw(5) << (time) / 1000 << "]  --> SYN 0 (attempt " << att + 1 << " of " << max_atts << ", RTO " << RTO
						<< ") to " << ip_globe << endl;
				}
				RTO = (GetTickCount64() - startConTime) * 3;
				RTT = (GetTickCount64() - startConTime)*1.2;
				break;
			}

			//That wasnt the right packet, or it wasnt a responce at least. 
			
		}
		else {
			return sentt;
		}
		
		att++;
		
		if (att >= max_atts) {
			return TIMEOUT;
		}
		RTO *= 2;

	}

	//Lets Read the returned packet
	ReceiverHeader * R = (ReceiverHeader*)buf;
	ULONGLONG endTime = GetTickCount64();
	long double time = (endTime - startTime);
	if (CONNECTION_MESSAGES) {
		cout << "[" << setw(5) << (time) / 1000 << "]  <-- SYN-ACK window " << sender_window << "; setting inital RTO to " << RTO / 1000 << endl;
	}
	
	//Send back ACK that recieved the SYN-ACK
	//s.sdh.flags.ACK = 1;
	//s.sdh.flags.SYN = 0;

	//SendACK((char *)&s, sizeof(s), targethost, buf); //Need Error detection, but need better errors for that. 
	


	return STATUS_OK;
}


int SenderSocket::Close(char * targethost, DWORD Magic_Port, int sender_window, LinkProperties * l) {

	//Create The packets
	SenderDataHeader sdh; //Data Header
	Flags f;			//Set flags for operation
	f.SYN = 0;
	f.ACK = 0;
	f.FIN = 1;
	f.reserved = 0;
	sdh.flags = f;
	sdh.seq = 0;


	char * buf = new char[1600]; //Completly arbitrary

	int max_atts = 5;
	int att = 0;


	while (att <= max_atts) {
		ULONGLONG endTime = GetTickCount64();
		ULONGLONG startConTime = GetTickCount64();
		int sentt = SendPacket((char *)&sdh, sizeof(sdh), targethost, buf);
		if (sentt == TIMEOUT) {

			long double time = (endTime - startTime);
			if (CONNECTION_MESSAGES) {
				cout << "[" << setw(5) << (time) / 1000 << "]  --> FIN 0 (attempt " << att + 1 << " of " << max_atts << ", RTO " << RTO / 1000
					<< ")" << endl;
			}
		}
		else if(sentt == STATUS_OK) {
			//ULONGLONG endTime = GetTickCount64();
			long double time = (endTime - startTime);

			ReceiverHeader * R = (ReceiverHeader*)buf;
			if (R->flags.FIN == 1 && R->flags.ACK == 1) {
				
				if (CONNECTION_MESSAGES) {
					cout << "[" << setw(5) << (time) / 1000 << "]  --> FIN 0 (attempt " << att + 1 << " of " << max_atts << ", RTO " << RTO
						<< ") to " << ip_globe << endl;
				}
				//RTO = (GetTickCount64() - startConTime) * 3;
				DWORD checks = R->recvWnd;
				cout << "[" << fixed << setprecision(2) << setw(5) << time / 1000 << "]  <-- FIN-ACK " << window + 1 << " window " << hex << checks << endl;
				dChecksum = R->recvWnd;
				break;
			}
			//If it comes through here it means the packet was not the correct one. 
		}
		else {
			return sentt;
		}

		att++;

		if (att >= max_atts) {
			return TIMEOUT;
		}
		RTO *= 2;

	}

	//Lets Read the returned packet
	//ReceiverHeader * R = (ReceiverHeader*)buf;
	ULONGLONG endTime = GetTickCount64();
	long double time = (endTime - startTime);
	if (CONNECTION_MESSAGES) {
		cout << "[" << setw(5) << (time) / 1000 << "]  <-- FIN-ACK window " << sender_window << "; setting inital RTO to " << RTO / 1000 << endl;
	}

	//Send back ACK that recieved the SYN-ACK
	/*s.sdh.flags.ACK = 1; 
	s.sdh.flags.SYN = 0;

	SendACK((char *)&s, sizeof(s), targethost, buf); //Need Error detection, but need better errors for that. 
	*/


	return STATUS_OK;
}

int SenderSocket::SendPacket(char * packet, int size, string IPs, char * buf) {

	if (!sent) {
		WSADATA wsaData;
		//Initialize WinSock; once per program run
		WORD wVersionRequested = MAKEWORD(2, 2);
		if (WSAStartup(wVersionRequested, &wsaData) != 0) {
			//printf("WSAStartup error %d\n", WSAGetLastError());
			WSACleanup();
			return FAILED_SEND;
		}
		else { sent = true; }


		//Creating Socket 
		//SOCKET sock;
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			printf("socket error %d\n", WSAGetLastError());
			WSACleanup();
			return FAILED_SEND;
		}

		//struct sockaddr_in local;
		memset(&local, 0, sizeof(local));
		local.sin_family = AF_INET;
		local.sin_addr.s_addr = INADDR_ANY;
		local.sin_port = htons(0);
		if (bind(sock, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
			printf("socket error %d\n", WSAGetLastError());
			WSACleanup();
			return FAILED_SEND;
		}



		//struct hostent *rmt;
		//struct sockaddr_in remote;
		memset(&remote, 0, sizeof(remote));
		// first assume that the string is an IP address
		DWORD IP = inet_addr(IPs.c_str());
		if (IP == INADDR_NONE)
		{
			// if not a valid IP, then do a DNS lookup
			if ((rmt = gethostbyname(IPs.c_str())) == NULL)
			{
				//printf("Invalid string: neither FQDN, nor IP address\n");
				return INVALID_NAME;
			}
			else { // take the first IP address and copy into sin_addr
				memcpy((char *)&(remote.sin_addr), rmt->h_addr, rmt->h_length);

				//cout << ip_globe << endl;
			}
		}
		else
		{
			// if a valid IP, directly drop its binary version into sin_addr
			remote.sin_addr.S_un.S_addr = IP;

		}


		ip_globe = inet_ntoa(remote.sin_addr);
		//memset(&remote, 0, sizeof(remote));
		remote.sin_family = AF_INET;
		//remote.sin_addr.s_addr = inet_addr(IP.c_str()); // server’s IP Not sure if this is correct
		remote.sin_port = htons(MAGIC_PORT); // Changed port to correct one
	}
	//int MAX_ATTEMPTS = 3;

	//int attempt = -1; //used the code in the handout, but the examples started from 0
	//while (attempt++ < MAX_ATTEMPTS)
	//{
		ULONGLONG startTime = GetTickCount64();
		//cout << "Attempt " << attempt << " with " << dec << size << " bytes... ";
		if (sendto(sock, packet, size, 0, (struct sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR) {
			printf("sendto error %d\n", WSAGetLastError());
			WSACleanup();
			return FAILED_SEND;
		}

		timeval tout;
		tout.tv_sec = 0;
		tout.tv_usec = RTO * 1000;


		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(sock, &fd);
		int avaliable = select(0, &fd, NULL, NULL, &tout);
		struct sockaddr_in response;
		int * ressize;
		int g = sizeof(response);
		ressize = &g;
		if (avaliable > 0) {		//Data came back. 
			int recsize = recvfrom(sock, buf, MAX_PKT_SIZE, 0, (struct sockaddr *)&response, ressize); //dont care about the sender
			if (recsize > MAX_PKT_SIZE) {
				cout << "  ++ invalid reply, exceeds MAX UDP Packet Size" << endl;
				return FAILED_RECV;
			}
			if (recsize == SOCKET_ERROR) {
				printf("socket error %d\n", WSAGetLastError());
				return FAILED_RECV;
			}
			//Successful read
			ULONGLONG endTime = GetTickCount64();
			//cout << "response in " << endTime - startTime << " ms with " << recsize << " bytes" << endl;

			if (response.sin_addr.s_addr != remote.sin_addr.s_addr || response.sin_port != remote.sin_port) {
				cout << "  ++ invalid reply source" << endl;
				return FAILED_RECV;
			}
			//cout << buf << endl;
			connectpacketsize = recsize;
			return STATUS_OK;

		}
		else if (avaliable == SOCKET_ERROR) {
			printf("Recvfrom error %d\n", WSAGetLastError());
			return FAILED_RECV;
		}
		ULONGLONG endTime = GetTickCount64();
		//cout << " timeout in " << endTime - startTime << " ms" << endl;

	//}

	return TIMEOUT;
}

int SenderSocket::Send(char * packet, int size, int seq, StatsParameters * p) {




	SenderDataHeader sdh;
	Flags f;
	f.ACK = 0;
	f.FIN = 0;
	f.reserved = 0;
	f.SYN = 0;
	sdh.flags = f;
	sdh.seq = seq;

	//cout << "Pkt Size: " << sizeof(SenderDataHeader) + size << endl;

	char * formedPkt = new char[sizeof(SenderDataHeader) + size];
	memcpy(formedPkt, &sdh, sizeof(SenderDataHeader));
	memcpy(formedPkt + sizeof(SenderDataHeader), packet, size);
	//cout << hex << formedPkt << endl;

	int max_atts = 3;
	int att = 0;

	char * buf = new char[15]; 

	RTO = RTT + 4 * max(.04, 0.01);//Setting the RTO


	while (att <= max_atts) {
		ULONGLONG endTime = GetTickCount64();
		ULONGLONG startConTime = GetTickCount64();
		int sentt = SendData(formedPkt, sizeof(SenderDataHeader)+size, buf, seq);
		//cout << "Current Seq: " << seq << endl;
		if (sentt == TIMEOUT) {

			long double time = (endTime - startTime);
			if (CONNECTION_MESSAGES) {
				cout << "[" << setw(5) << (time) / 1000 << "]  --> SYN 0 (attempt " << att + 1 << " of " << max_atts << ", RTO " << RTO
					<< ") to " << ip_globe << endl;
			}
			WaitForSingleObject(p->mutex, INFINITE);
			p->timeouts += 1;
			ReleaseMutex(p->mutex);

		}
		else if (sentt == STATUS_OK) {
			//ULONGLONG endTime = GetTickCount64();
			long double time = (endTime - startTime);

			ReceiverHeader * R = (ReceiverHeader*)buf;
			//cout << "Next Seq: " << R->ackSeq << endl;
			if (R->ackSeq== seq + 1) {
				
				//cout << "Correct" << endl;
				
					//This needs to be changed to the new algorithm 

				WaitForSingleObject(p->mutex, INFINITE);
				p->base = seq;
				p->nextSequence = R->ackSeq;
				ReleaseMutex(p->mutex);
				
				window = seq;//Setting for the close; 
				break;
			}

			//That wasnt the right packet, or it wasnt a responce at least. 

		}
		else {
			return sentt;
		}

		att++;

		if (att >= max_atts) {
			return TIMEOUT;
		}
		//cout << RTO << endl;
		RTO *= 2;

	}


	return STATUS_OK;
}

int SenderSocket::SendData(char * packet, int size, char * buf, int seq) {

	//int MAX_ATTEMPTS = 3;

	//int attempt = -1; //used the code in the handout, but the examples started from 0
	//while (attempt++ < MAX_ATTEMPTS)
	//{
	ULONGLONG startTimesend = GetTickCount64();
	//cout << "Attempt " << attempt << " with " << dec << size << " bytes... ";

	//add the header onto the piece of data. 


	if (sendto(sock, packet, size, 0, (struct sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR) {
		printf("sendto error %d\n", WSAGetLastError());
		WSACleanup();
		return FAILED_SEND;
	}
	
	timeval tout;
	tout.tv_sec = 0;
	double t = RTO / 1000;
	int tv = t * 1000000;
	//cout <<"TV: "<< tv << endl;
	//cout << "RTO:" << t << endl;
	tout.tv_usec = tv; //*RTO


	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(sock, &fd);
	int avaliable = select(0, &fd, NULL, NULL, &tout);
	struct sockaddr_in response;
	int * ressize;
	int g = sizeof(response);
	ressize = &g;
	//char *buf = new char[15]; //response is 12, 15 incase Im missing something. 

	if (avaliable > 0) {		//Data came back. 
		int recsize = recvfrom(sock, buf, MAX_PKT_SIZE, 0, (struct sockaddr *)&response, ressize); //dont care about the sender
		if (recsize > MAX_PKT_SIZE) {
			cout << "  ++ invalid reply, exceeds MAX UDP Packet Size" << endl;
			return FAILED_RECV;
		}
		if (recsize == SOCKET_ERROR) {
			printf("socket error %d\n", WSAGetLastError());
			return FAILED_RECV;
		}
		//Successful read
		ULONGLONG endTime = GetTickCount64();
		//RTT = endTime - startTimesend;
		//cout << "response in " << endTime - startTime << " ms with " << recsize << " bytes" << endl;

		if (response.sin_addr.s_addr != remote.sin_addr.s_addr || response.sin_port != remote.sin_port) {
			cout << "  ++ invalid reply source" << endl;
			return FAILED_RECV;
		}
		//cout << buf << endl;
		connectpacketsize = recsize;
		return STATUS_OK;

	}
	else if (avaliable == SOCKET_ERROR) {
		printf("Recvfrom error %d\n", WSAGetLastError());
		return FAILED_RECV;
	}
	ULONGLONG endTime = GetTickCount64();
	//cout << " timeout in " << endTime - startTime << " ms" << endl;

	//}

	return TIMEOUT;


}