#include <stdio.h>
#include <WinSock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 6000

SOCKET   arrSocket[WSA_MAXIMUM_WAIT_EVENTS] = { 0 };
WSAEVENT arrEvent[WSA_MAXIMUM_WAIT_EVENTS] = { 0 };
unsigned g_total(0);
unsigned g_index(0);
int      g_clientNum(0);

bool initSock() {
	WSADATA wsa = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &wsa))
		return false;
	if (LOBYTE(wsa.wVersion) != 2 || HIBYTE(wsa.wVersion) != 2) {
		WSACleanup();
		return false;
	}
	return true;
}

void clearSock(SOCKET& sock) {
	closesocket(sock);
	WSACleanup();
}

inline bool eventFunc(WSANETWORKEVENTS& netWorkEvent, SOCKET& sockClient, char* buf, long clientEventType) {
	if (netWorkEvent.lNetworkEvents & FD_ACCEPT) {
		if (netWorkEvent.iErrorCode[FD_ACCEPT_BIT] != 0)
			return false;
		sockClient = accept(arrSocket[g_index - WSA_WAIT_EVENT_0], NULL, NULL);
		if (sockClient == INVALID_SOCKET)
			return false;
		WSAEVENT newEvent = WSACreateEvent();
		WSAEventSelect(sockClient, newEvent, clientEventType);
		arrSocket[g_total] = sockClient;
		arrEvent[++g_total] = newEvent;
		++g_clientNum;
	}
	if (netWorkEvent.lNetworkEvents & FD_READ) {
		if (netWorkEvent.iErrorCode[FD_READ_BIT] != 0) {
			return false;
		}
		recv(arrSocket[g_index - WSA_WAIT_EVENT_0], buf, sizeof(buf), 0);
		printf("Recv: %s\n", buf);
	}

	if (netWorkEvent.lNetworkEvents & FD_WRITE) {
		if (netWorkEvent.iErrorCode[FD_WRITE_BIT] != 0) {
			return false;
		}
	}

	if (netWorkEvent.lNetworkEvents & FD_CLOSE) {
		if (netWorkEvent.iErrorCode[FD_CLOSE_BIT] != 0) {
			return false;
		}
		closesocket(arrSocket[g_index - WSA_WAIT_EVENT_0]);
	}
	return true;
}


int main(int argc, char* argv[]) {
	while (!initSock());
	SOCKET sockServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockServer == INVALID_SOCKET) {
		clearSock(sockServer);
		return -1;
	}
	sockaddr_in serverAddr = { 0 };
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(PORT);


	if (bind(sockServer, (sockaddr*)&serverAddr, sizeof(sockaddr))) {
		clearSock(sockServer);
		return -1;
	}

	WSAEVENT listenEvent = WSACreateEvent();
	long eventType = FD_ACCEPT | FD_CLOSE;
	WSAEventSelect(sockServer, listenEvent, eventType);
	printf("******************************************************************************\n");
	printf("**********welcome to use the high-big-up server built by lancelot************");
	printf("********************************************************************************\n");
	printf("\nwaiting...\n");
	if (listen(sockServer, SOMAXCONN) == SOCKET_ERROR) {
		clearSock(sockServer);
		printf("\n                               bye-bye \n");
		return -1;
	}
	arrSocket[g_total] = sockServer;
	arrEvent[g_total++] = listenEvent;

	char buf[1024] = { 0 };
	SOCKET sockClient = INVALID_SOCKET;
	WSANETWORKEVENTS netWorkEvent = { 0 };
	long clientEventType = FD_CLOSE | FD_READ | FD_WRITE;
	while (true) {
		g_index = WSAWaitForMultipleEvents(g_total, arrEvent, false, 100, false);
		if (g_index == WSA_WAIT_TIMEOUT)
			continue;
		WSAEnumNetworkEvents(arrSocket[g_index - WSA_WAIT_EVENT_0], arrEvent[g_index - WSA_WAIT_EVENT_0], &netWorkEvent);
		if(!eventFunc(netWorkEvent, sockClient, buf, clientEventType))
			continue;
		printf("get a message!\n");
	}

	if (sockServer != INVALID_SOCKET)
		closesocket(sockServer);
	WSACleanup();
	return 0;
}