#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#include <windows.h>
#include <stdio.h>
#include <process.h>

fd_set g_fdClientSock;
int clientNum(0);

#pragma comment(lib, "ws2_32.lib")

#define PORT 6000

void InitSock() {
	WSADATA wsa{ 0 };
	if (WSAStartup(MAKEWORD(2, 2), &wsa))
		exit(-1);
	if (LOBYTE(wsa.wVersion) != 2 || HIBYTE(wsa.wVersion) != 2) {
		WSACleanup();
		exit(-1);
	}
}

void listenThreadProc(void * lParam) {
	fd_set fdRead;
	FD_ZERO(&fdRead);
	int nRet(0);
	char* recvBuffer = (char*)malloc(sizeof(char)* 1024);
	if (nullptr == recvBuffer)
		return;
	memset(recvBuffer, 0, 1024);
	while (true) {
		fdRead = g_fdClientSock;
		timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 10;
		nRet = select(0, &fdRead, NULL, NULL, &tv);
		if (nRet != SOCKET_ERROR) {
			fdRead = g_fdClientSock;
			if (nRet != SOCKET_ERROR){
				for (unsigned int i = 0; i < g_fdClientSock.fd_count; ++i) {
					if (FD_ISSET(g_fdClientSock.fd_array[i], &fdRead)) {
						memset(recvBuffer, 0, 1024);
						nRet = recv(g_fdClientSock.fd_array[i], recvBuffer, 1024, 0);
						if (nRet == SOCKET_ERROR || nRet == 0) {
							closesocket(g_fdClientSock.fd_array[i]);
							--clientNum;
							FD_CLR(g_fdClientSock.fd_array[i], &g_fdClientSock);
						}
						else {
							printf("Receive msg:%s\n", recvBuffer);
							send(g_fdClientSock.fd_array[i], recvBuffer, strlen(recvBuffer), 0);
						}
					}
				}
			}
		}
	}
	if (recvBuffer != nullptr) {
		free(recvBuffer);
		recvBuffer = nullptr;
	}
}


int main() {
	InitSock();
	SOCKET sockServer;
	sockServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in severAddr;
	severAddr.sin_family = AF_INET;
	severAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	severAddr.sin_port = htons(PORT);
	
	int ret = bind(sockServer, (sockaddr*)&severAddr, sizeof(sockaddr));
	ret = listen(sockServer, 4);
	sockaddr_in clientAddr;
	int nameLen = sizeof(clientAddr);
	_beginthread(listenThreadProc, 0, nullptr);
	while (clientNum < FD_SETSIZE) {
		SOCKET clientSock = accept(sockServer, (sockaddr*)&clientAddr, &nameLen);
		FD_SET(clientSock, &g_fdClientSock);
		++clientNum;
	}

	return 0;
};
