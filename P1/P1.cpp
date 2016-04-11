// P1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
//https://msdn.microsoft.com/en-us/library/windows/desktop/ms686903(v=vs.85).aspx
#define BUFFER_SIZE 10
#define PRODUCER_SLEEP_TIME_MS 500
#define CONSUMER_SLEEP_TIME_MS 2000

LONG Buffer[BUFFER_SIZE];
LONG LastItemProduced;
ULONG QueueSize;
ULONG QueueStartOffset;

ULONG TotalItemsProduced;
ULONG TotalItemsConsumed;

CONDITION_VARIABLE BufferNotEmpty;
CONDITION_VARIABLE BufferNotFull;
CRITICAL_SECTION   BufferLock;

BOOL StopRequested;



DWORD WINAPI statsThread(LPVOID pParam) {
	StatsParameters *p = ((StatsParameters*)pParam);
	double lastPrintout = 0.0;
	while (true) {
		Sleep(2003);
		long double time = (GetTickCount64() - p->startTime);
		
		WaitForSingleObject(p->mutex, INFINITE);
		double ACKED = ((p->base) * (1464));
		ACKED = ACKED / 1000000;
		double goodput = (p->base - lastPrintout) * (8 * (MAX_PKT_SIZE - sizeof(SenderDataHeader)));
		goodput = goodput / 1000000;
		double rt = p->RTT;
		rt = rt / 1000;
		cout << "[" << fixed << setprecision(2) << setw(5) << time/1000 << "]  B "<< setw(5) << p->base<<" ( "<<
			fixed<<setprecision(1)<<setw(5)<< ACKED << " MB) " << " N" << 
			setw(5) << p->nextSequence << " T " << p->timeouts << " F " << p->fastTransmissions
			<< " W " << p->windowSize << " S " << fixed << setprecision(3) << setw(4)<< goodput << " Mbps RTT " << rt << endl;
		lastPrintout = p->base;
		ReleaseMutex(p->mutex);


	}



	return 0;
}

DWORD WINAPI sendThread(LPVOID pParam) {
	
	while (true)
	{
		// Produce a new item.

		Sleep(rand() % PRODUCER_SLEEP_TIME_MS);

		ULONG Item = InterlockedIncrement(&LastItemProduced);

		EnterCriticalSection(&BufferLock);

		while (QueueSize == BUFFER_SIZE && StopRequested == FALSE)
		{
			// Buffer is full - sleep so consumers can get items.
			SleepConditionVariableCS(&BufferNotFull, &BufferLock, INFINITE);
		}

		if (StopRequested == TRUE)
		{
			LeaveCriticalSection(&BufferLock);
			break;
		}

		// Insert the item at the end of the queue and increment size.

		Buffer[(QueueStartOffset + QueueSize) % BUFFER_SIZE] = Item;
		QueueSize++;
		TotalItemsProduced++;

		printf("Producer %u: item %2d, queue size %2u\r\n", Item, QueueSize);

		LeaveCriticalSection(&BufferLock);

		// If a consumer is waiting, wake it.

		WakeConditionVariable(&BufferNotEmpty);
	}

	printf("Producer %u exiting\r\n");
	return 0;

}

DWORD WINAPI ackThread(LPVOID pParam) {
	while (true)
	{
		EnterCriticalSection(&BufferLock);

		while (QueueSize == 0 && StopRequested == FALSE)
		{
			// Buffer is empty - sleep so producers can create items.
			SleepConditionVariableCS(&BufferNotEmpty, &BufferLock, INFINITE);
		}

		if (StopRequested == TRUE && QueueSize == 0)
		{
			LeaveCriticalSection(&BufferLock);
			break;
		}

		// Consume the first available item.

		LONG Item = Buffer[QueueStartOffset];

		QueueSize--;
		QueueStartOffset++;
		TotalItemsConsumed++;

		if (QueueStartOffset == BUFFER_SIZE)
		{
			QueueStartOffset = 0;
		}

		printf("Consumer %u: item %2d, queue size %2u\r\n", Item, QueueSize);

		LeaveCriticalSection(&BufferLock);

		// If a producer is waiting, wake it.

		WakeConditionVariable(&BufferNotFull);

		// Simulate processing of the item.

		Sleep(rand() % CONSUMER_SLEEP_TIME_MS);
	}

	printf("Consumer %u exiting\r\n");
	return 0;

}


int main(int argc, char *argv[])
{
	InitializeConditionVariable(&BufferNotEmpty);
	InitializeConditionVariable(&BufferNotFull);

	InitializeCriticalSection(&BufferLock);

	DWORD id;
	HANDLE hProducer1 = CreateThread(NULL, 0, sendThread, (PVOID)1, 0, &id);
	HANDLE hConsumer1 = CreateThread(NULL, 0, ackThread, (PVOID)1, 0, &id);
	//HANDLE hConsumer2 = CreateThread(NULL, 0, ConsumerThreadProc, (PVOID)2, 0, &id);

	puts("Press enter to stop...");
	getchar();

	EnterCriticalSection(&BufferLock);
	StopRequested = TRUE;
	LeaveCriticalSection(&BufferLock);

	WakeAllConditionVariable(&BufferNotFull);
	WakeAllConditionVariable(&BufferNotEmpty);

	WaitForSingleObject(hProducer1, INFINITE);
	WaitForSingleObject(hConsumer1, INFINITE);
	//WaitForSingleObject(hConsumer2, INFINITE);

	printf("TotalItemsProduced: %u, TotalItemsConsumed: %u\r\n",
		TotalItemsProduced, TotalItemsConsumed);



	if (argc != 8) {
		cout << "Options: " << endl;
		cout << " \t Host/IP" << endl;
		cout << " \t Buffer Size" << endl;
		cout << " \t Sender Window" << endl;
		cout << " \t Propogation delay" << endl;
		cout << " \t Loss in Forward Path" << endl;
		cout << " \t Loss on Return Path" << endl;
		cout << " \t Link Bottleneck" << endl;
		return 0;
	}

	char * targethost = argv[1];
	int power = atoi(argv[2]);
	int senderwindow = atoi(argv[3]);
	float propagation = atof(argv[4]);
	float lossF = atof(argv[5]);
	float lossB = atof(argv[6]);
	int bottleneck = atoi(argv[7]);

	//Inital Checks


	
	//Inital printout -- DONE
	cout << "Main:\t sender W = " << power; 
	printf(", RTT %g sec, loss ", propagation);
	printf("%g / %g", lossF, lossB);
	cout << ", link " << bottleneck << " Mbps" << endl;

	ULONGLONG startTime = GetTickCount64();
	UINT64 dwordBufSize = (UINT64)1 << power;
	DWORD *dwordBuf = new DWORD[dwordBufSize]; // user-requested buffer
	for (UINT64 i = 0; i < dwordBufSize; i++) // required initialization
		dwordBuf[i] = i;

	

	ULONGLONG endTime = GetTickCount64();
	cout << "Main:\t initializing DWORD array with 2^" << power << " elements... done in " << endTime - startTime << " ms" << endl;



	LinkProperties lp;
	lp.RTT = propagation;
	lp.speed = 1e6 * bottleneck; // convert to megabits
	lp.pLoss[0] = lossF;
	lp.pLoss[1] = lossB;
	lp.bufferSize = senderwindow + 5; //Could be source of error

	SenderSocket ss; // instance of your class
	int status = 0;
	if ((status = ss.Open(targethost, MAGIC_PORT, senderwindow, &lp)) != STATUS_OK) {
		cout << "Main:\t connect failed with status " << status << endl;
		return 0;
	}
	ULONGLONG endConnect = GetTickCount64();
	long double ctime = endConnect - ss.getStartTime();
	cout << "Main:\t connected to " << targethost << " in "<<ctime/1000<<" sec, pkt 1472 bytes"<< endl;
	ULONGLONG startTransfer = GetTickCount64();


	//Start Stats Thread 
	StatsParameters p;
	p.startTime = GetTickCount64();
	p.windowSize = 1; //Hard coded for now

	p.mutex = CreateMutex(NULL, 0, NULL);
	HANDLE *handles = new HANDLE[1];
	handles[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)statsThread, &p, 0, NULL);


	//Sleep(10000);

	char *charBuf = (char*)dwordBuf; // this buffer goes into socket
	UINT64 byteBufferSize = dwordBufSize << 2; // convert to bytes

	

	UINT64 off = 0; // current position in buffer
	propagation += .002;
	propagation *= 1000;
	int sequence_Num = 0;

	int sentbytes = 0;

	while (off < byteBufferSize)
	{
		
		// decide the size of next chunk
		int bytes = min(byteBufferSize - off, MAX_PKT_SIZE - sizeof(SenderDataHeader));
		// send chunk into socket
		//cout << "Bytes: " << bytes << endl;
		if ((status = ss.Send(charBuf + off, bytes,sequence_Num, &p)) != STATUS_OK) {
			cout << "Error Status: " << status << endl;
			// error handing: print status and quit
			off += bytes;
		}
		sequence_Num++;
		off += bytes;
		sentbytes += bytes;
		//update RTT
		
		
		p.RTT = ss.RTT; //Unclear about how estRTT is formed. 
	}
	

	

	//Close Stats Thread

	CloseHandle(handles[0]);

	ULONGLONG endTransfer = GetTickCount64();

	

	long double transfer_time= endTransfer - startTransfer;
	if ((status = ss.Close(targethost, MAGIC_PORT, senderwindow, &lp)) != STATUS_OK){
		cout << "Main:\t close failed with status " << status << endl;
		return 0;
	}

	Checksum cs;
	DWORD check = cs.CRC32((unsigned char *)dwordBuf, (dwordBufSize << 2));

	//Checksum cs;
	//DWORD check = cs.CRC32((unsigned char *)charBuf, byteBufferSize);
	double speed = ((sentbytes * 8)) / (transfer_time / 1000);
	speed = speed / 1000;
	cout << "Main:\t transfer finished in " << transfer_time / 1000 << " sec, " <<speed<< " Kbps, checksum " << hex << check<< endl;
	double ideal = (1 / (propagation / 1000)) * 1464 * 8;
	cout << "Main:\t estRTT " << fixed << setprecision(3)<< p.RTT / 1000 << ", ideal rate "<< setprecision(2) << ideal/1000<<" Kbps"<< endl;







    return 0;
}

