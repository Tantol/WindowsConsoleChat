#pragma once
// Networking libraries
#include <winsock2.h>
#include <Windows.h>
#include "NetworkDevice.h"
#include <ws2tcpip.h>
#include <stdio.h>

// port to connect sockets through 
#define DEFAULT_PORT "6881"
// ip to connect sockets through
#define DEFAULT_IPADDRESS "127.0.0.1"
// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

class CClientNetwork {

public:

	//// variable
		/**/

public:

	//// function

	// const. / deconst.
	CClientNetwork(void);
	~CClientNetwork(void);

	// send package to server
	bool sendPackage(std::string & sPackage);

	// receive package from server 
	bool receivePackage(std::string & sPackage, bool bWait);

protected:

	//// variable

	// client socket to connect with server
	SOCKET m_ConnectSocket;

	// devices for client
	CNetworkDevice * m_pDevice;

protected:

	//// function
		/**/
};
