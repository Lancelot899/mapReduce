#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <WinSock2.h>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define MESSAGESIZE 512
#define PORT        6000

bool initSock() {
	WSADATA wsa = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &wsa))
		return false;
	if (LOBYTE(wsa.wVersion) != 2 || HIBYTE(wsa.wVersion) != 2)
		return false;
	return true;
}


typedef struct __overlap {
	WSAOVERLAPPED _overlap;
	WSABUF        _buf;
	char          _message[MESSAGESIZE];
	DWORD         _NumberOfBytesRecvd;
	DWORD         _flag;
}OVERLAP_IO_DATA, *pOVERLAP_IO_DATA;

int g_itotalConn(0);
SOCKET             g_ClientSockArr[MAXIMUM_WAIT_OBJECTS];
WSAEVENT           g_ClientEventArr[MAXIMUM_WAIT_OBJECTS];
pOVERLAP_IO_DATA   g_pPerIODataArr[MAXIMUM_WAIT_OBJECTS];

void Cleanup(int index);

void workProc(void*lParam) {
	int ret(0), index(0);
	DWORD cbTransferred;
	while (true) {
		ret = WSAWaitForMultipleEvents(g_itotalConn, g_ClientEventArr, false, 100, false);
		if (ret == WSA_WAIT_FAILED || ret == WSA_WAIT_TIMEOUT)
			continue;
		index = ret - WSA_WAIT_EVENT_0;
		WSAGetOverlappedResult(
			g_ClientSockArr[index],
			&g_pPerIODataArr[index]->_overlap,
			&cbTransferred,
			true,
			&g_pPerIODataArr[index]->_flag);
		if (cbTransferred == 0) 
			Cleanup(index);
		else {
			g_pPerIODataArr[index]->_message[cbTransferred] = 0;
			send(g_ClientSockArr[index], g_pPerIODataArr[index]->_message, cbTransferred, 0);
			WSARecv(g_ClientSockArr[index], &g_pPerIODataArr[index]->_buf, 1, &g_pPerIODataArr[index]->_NumberOfBytesRecvd,
				&g_pPerIODataArr[index]->_flag, &g_pPerIODataArr[index]->_overlap, NULL);
		}
	}
}


void Cleanup(int index) {
	closesocket(g_ClientSockArr[index]);
	WSACloseEvent(g_ClientEventArr[index]);
	HeapFree(GetProcessHeap(), 0, g_pPerIODataArr[index]);
	if (index < g_itotalConn - 1) {
		g_ClientSockArr[index] = g_ClientSockArr[g_itotalConn - 1];
		g_ClientEventArr[index] = g_ClientEventArr[g_itotalConn - 1];
		g_pPerIODataArr[index] = g_pPerIODataArr[g_itotalConn - 1];
	}
}


int main() {
	SOCKET          serverSock, clientSock;
	sockaddr_in     serverAddr{ 0 }, clientAddr{ 0 };
	unsigned long   threadID;
	int             addrSize = sizeof(sockaddr_in);

	//! initial socket
	initSock();
	if (INVALID_SOCKET == (serverSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP))) {
		closesocket(serverSock);
		WSACleanup();
		return -1;
	}
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = PORT;
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	bind(serverSock, (sockaddr*)&serverAddr, addrSize);
	threadID = _beginthread(workProc, NULL, NULL);
	listen(serverSock, SOMAXCONN);


	while (true) {
		clientSock = accept(serverSock, (sockaddr*)&clientAddr, &addrSize);
		printf("client: %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
		g_pPerIODataArr[g_itotalConn] = (pOVERLAP_IO_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OVERLAP_IO_DATA));
		g_ClientSockArr[g_itotalConn] = clientSock;
		g_pPerIODataArr[g_itotalConn]->_buf.len = MESSAGESIZE;
		g_pPerIODataArr[g_itotalConn]->_buf.buf = g_pPerIODataArr[g_itotalConn]->_message;
		g_ClientEventArr[g_itotalConn] = g_pPerIODataArr[g_itotalConn]->_overlap.hEvent = WSACreateEvent();
		WSARecv(g_ClientSockArr[g_itotalConn],
			&g_pPerIODataArr[g_itotalConn]->_buf,
			1,
			&g_pPerIODataArr[g_itotalConn]->_NumberOfBytesRecvd,
			&g_pPerIODataArr[g_itotalConn]->_flag,
			&g_pPerIODataArr[g_itotalConn]->_overlap,
			NULL);
		++g_itotalConn;
	}


	closesocket(serverSock);
	WSACleanup();
	return 0;
}