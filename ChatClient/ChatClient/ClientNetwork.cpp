#include "ClientNetwork.h"

CClientNetwork::CClientNetwork(void) {
	// create WSADATA object
	WSADATA wsaData;

	// socket
	m_ConnectSocket = INVALID_SOCKET;

	// holds address info for socket to connect to
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		exit(1);
	}

	// set address info
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;  //TCP connection!!!

	//resolve server address and port
	iResult = getaddrinfo(DEFAULT_IPADDRESS, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		exit(1);
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		m_ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (m_ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			exit(1);
		}

		// Connect to server.
		iResult = connect(m_ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(m_ConnectSocket);
			m_ConnectSocket = INVALID_SOCKET;
			printf("The server is down... did not connect");
		}
	}

	// no longer need address info for server
	freeaddrinfo(result);

	// if connection failed
	if (m_ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		exit(1);
	}

	/*
	// Set the mode of the socket to be nonblocking
	u_long iMode = 1;
	iResult = ioctlsocket(m_ConnectSocket, FIONBIO, &iMode);
	if (iResult == SOCKET_ERROR) {
		printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
		closesocket(m_ConnectSocket);
		WSACleanup();
		exit(1);
	}*/

	//disable nagle
	char value = 1;
	setsockopt(m_ConnectSocket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));

	// push SOCKET to NetworkDevice class
	m_pDevice = new CNetworkDevice(m_ConnectSocket);

}
// deconstruction
CClientNetwork::~CClientNetwork(void) {
}

// send package to server
bool  CClientNetwork::sendPackage(std::string & sPackage) {
	return m_pDevice->sendPackage(sPackage);
}

// receive package from server
bool CClientNetwork::receivePackage(std::string & sPackage, bool bWait) {
	return  m_pDevice->receivePackage(sPackage, bWait);
}
