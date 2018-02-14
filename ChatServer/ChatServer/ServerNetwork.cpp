#include "ServerNetwork.h"

CServerNetwork::CServerNetwork(void) {
	// create WSADATA object
	WSADATA wsaData;

	// our sockets for the server
	m_sListenSocket = INVALID_SOCKET;

	// address info for the server to listen to
	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		exit(1);
	}

	// set address information
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;    // TCP connection!!!
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		exit(1);
	}

	// Create a SOCKET for connecting to server
	m_sListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (m_sListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		exit(1);
	}

	/*
	// Set the mode of the socket to be nonblocking
	u_long iMode = 1;
	iResult = ioctlsocket(m_sListenSocket, FIONBIO, &iMode);
	if (iResult == SOCKET_ERROR) {
		printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
		closesocket(m_sListenSocket);
		WSACleanup();
		exit(1);
	}*/

	// Setup the TCP listening socket
	iResult = bind(m_sListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(m_sListenSocket);
		WSACleanup();
		exit(1);
	}

	// no longer need address information
	freeaddrinfo(result);

	// start listening for new clients attempting to connect
	iResult = listen(m_sListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(m_sListenSocket);
		WSACleanup();
		exit(1);
	}
}
CServerNetwork::~CServerNetwork(void) {
}


// login / logout tools
bool CServerNetwork::registerUser(std::string & sError, CVectorRegister & mRegisterMap,
	std::string sLogin, std::string sPassword) {
	return mRegisterMap.registerUser(sError, sLogin, sPassword);
}
// device login user
bool CServerNetwork::login(std::string & sError, std::string & sLoginAs, CVectorRegister & mRegisterMap,
	CUserMap & mUserMap, CAdminMap & mAdminMap, SOCKET sClientSock, std::string sLogin, std::string sPassword) {
	
	bool bRegister = mRegisterMap.check(sError, sLoginAs, sLogin, sPassword);
	if (!bRegister) {
		return false;
	}

	bool bAdd = mUserMap.addClient(sError, sClientSock, sLogin);
	if (bAdd && sLoginAs == "admin") {
		bAdd = mAdminMap.addClient(sError, sClientSock, sLogin);
	}
	return bAdd;
}
// device login admin
bool CServerNetwork::loginAdmin(std::string & sError, CAdminMap & mAdminMap, SOCKET sClientSock, std::string sNickname) {
	return mAdminMap.addClient(sError, sClientSock, sNickname);
}
// device logout client
bool CServerNetwork::logout(std::string & sError, CUserMap & mUserMap, CAdminMap & mAdminMap, SOCKET sClientSock) {
	mAdminMap.removeClient(sError, sClientSock);
	return mUserMap.removeClient(sError, sClientSock);
}
void CServerNetwork::disconnected(CVectorGroups & mGroupMap, CUserMap & mUserMap,
	CAdminMap & mAdminMap, SOCKET sClientSock) {
	char cLogInfo[512];
	std::string sLogInfo;

	std::string sCurrentTime;
	updateTime(sCurrentTime);

	std::string sError;
	std::string sNickname;
	mUserMap.findClientNnByCs(sError, sNickname, sClientSock);

	sprintf_s(cLogInfo, "%s %s -> Type: [Disconnected]\n", sCurrentTime.c_str(),
		sNickname.c_str());
	sLogInfo += cLogInfo;
	
	std::string sInfo = "";
	bool bRemovedAll = mGroupMap.removeFromAllGroups(sError, sInfo, sClientSock); // remove client from groups
	if (bRemovedAll) {
		if (sInfo != "") {
			sprintf_s(cLogInfo, "%s SYSTEM -> %s\n", sCurrentTime.c_str(), sInfo.c_str());
			sLogInfo += cLogInfo;
		}
	}

	mUserMap.removeClient(sError, sClientSock); // remove client from login map (if he was there)
	mAdminMap.removeClient(sError, sClientSock); // remove client from admin map (if he was there)

	printf("%s", sLogInfo.c_str());
	writeTextToLogFile(sLogInfo);
}


// send tools
bool CServerNetwork::sendToClient(std::string & sError, CUserMap & mUserMap, std::string sMessage,
	std::string sSendTo, std::string sWhoSend) {
	
	bool bLoginSend = mUserMap.isLogin(sError, sWhoSend);
	if (!bLoginSend) {
		return false;
	}

	SOCKET sSendToSock;
	bool bLoginRecv = mUserMap.findClientCsByNn(sError, sSendToSock, sSendTo);
	if (!bLoginRecv) {
		return false;
	}

	CNetworkDevice ndSendTo(sSendToSock);
	std::string sPackage = ndSendTo.compresPackage(STATE_SEND_MESSAGE, sMessage, STATE_TO_USER, sWhoSend);
	ndSendTo.sendPackage(sPackage);
	return true;
}
bool CServerNetwork::sendToAll(std::string & sError, CUserMap & mUserMap, std::string sMessage, std::string sWhoSend) {
	std::string sPackage = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE, sMessage, STATE_TO_ALL, sWhoSend);
	return mUserMap.sendMessageToAll(sPackage); // this function return always true (on this moment)
}
bool CServerNetwork::sendToGroup(std::string & sError, CVectorGroups & mGroupMap, std::string sMessage, std::string sWhoSend, std::string sGroupName) {
	sMessage = sWhoSend + ": " + sMessage;
	std::string sPackage = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE, sMessage, STATE_TO_GROUP, sGroupName);
	return mGroupMap.sendMessageToAll(sError, sPackage, sGroupName);
}


// group tools
bool CServerNetwork::createGroup(std::string & sError, CVectorGroups & mGroupMap, std::string sGroupName, SOCKET sClientSock,
	std::string sNickname) {
	return mGroupMap.createGroup(sError, sGroupName, sClientSock, sNickname);
}
bool CServerNetwork::joinGroup(std::string & sError, CVectorGroups & mGroupMap, std::string sGroupName, SOCKET sClientSock,
	std::string sNickname) {
	return mGroupMap.addToGroup(sError, sGroupName, sClientSock, sNickname);
}
bool CServerNetwork::abandonGroup(std::string & sError, std::string & sInfo,
	CVectorGroups & mGroupMap, std::string sGroupName, SOCKET sClientSocket) {
	return mGroupMap.removeFromGroup(sError, sInfo, sGroupName, sClientSocket);
}
bool CServerNetwork::abandonAllGroups(std::string & sError, std::string & sInfo, 
	CVectorGroups & mGroupMap, SOCKET sClientSocket) {
	return mGroupMap.removeFromAllGroups(sError, sInfo, sClientSocket);
}
bool CServerNetwork::changeLeader(std::string & sError, CVectorGroups & mGroupMap, std::string sGroupName,
	std::string sGLName) {
	bool bChange = mGroupMap.changeLeader(sError, sGLName, sGroupName);
	return bChange;
}
bool CServerNetwork::kickFromGroup(std::string & sError, std::string & sInfo, 
	CVectorGroups & Group, std::string sGroupName, SOCKET sRemoveClient) {

	bool bRemoved = Group.removeFromGroup(sError, sInfo, sGroupName, sRemoveClient);
	if (!bRemoved) {
		return false;
	}
	std::string sPackageRemove = CNetworkDevice::compresPackage(STATE_KICK_USER_FROM_GROUP,
		"U have been removed\n", STATE_KICKED, sGroupName);
	CNetworkDevice::sendPackage(sPackageRemove, sRemoveClient);

	return true;
}
bool CServerNetwork::sendGroupMembersList(std::string & sError, CVectorGroups & mGroupMap, SOCKET sClientSock,
	std::string sGroupName) {
	bool bSend = mGroupMap.sendMembersList(sError, sGroupName, sClientSock);
	return bSend;
}
bool CServerNetwork::voteKick(std::string & sError, std::string & sInfo, CVectorGroups & mGroupMap,
	std::string sNickname, std::string sGroupName) {
	bool bKick = mGroupMap.voteKick(sError, sInfo, sGroupName, sNickname);
	return bKick;
}
bool CServerNetwork::inviteToGroup(std::string & sError, CVectorGroups & mGroupMap,
	std::string sGroupName, SOCKET sClientSock, std::string sWhoInv) {
	bool bIsIn = mGroupMap.canInivteUser(sError, sGroupName, sClientSock, sWhoInv);
	if (!bIsIn) {
		return false;
	}
	std::string sPackageRemove = CNetworkDevice::compresPackage(STATE_INVITE_USER_TO_GROUP,
		"U have been invited", STATE_INVITE, sGroupName);
	CNetworkDevice::sendPackage(sPackageRemove, sClientSock);
	return true;
}
bool CServerNetwork::requestInviteUser(std::string & sError, CVectorGroups & mGroupMap,
	std::string sGroupName, std::string sWhoInv, std::string sInvited) {
	return mGroupMap.requestInviteUser(sError, sGroupName, sWhoInv, sInvited);
}
bool CServerNetwork::changeGroupName(std::string & sError, CVectorGroups & mGroupMap,
	std::string sWhoChange, std::string sOldGroupName, std::string sNewGroupName) {
	return mGroupMap.changeGroupName(sError, sWhoChange, sOldGroupName, sNewGroupName);
}


// admin tools
bool CServerNetwork::loginUsersList(std::string & sError, CUserMap & mUserMap, SOCKET sAdminSock) {
	return mUserMap.sendLoginUsersList(sError, sAdminSock);
}
bool CServerNetwork::allGroupsList(std::string & sError, CVectorGroups & mGroupMap, SOCKET sAdminSock) {
	return mGroupMap.groupList(sError, sAdminSock);
}
bool CServerNetwork::deleteGroupByAdmin(std::string & sError, CVectorGroups & mGroupMap, std::string sGroupName) {
	return mGroupMap.deleteGroupByAdmin(sError, sGroupName);
}
bool CServerNetwork::kickFromGroupByAdmin(std::string & sError, std::string & sInfo, CVectorGroups & mGroupMap,
	std::string sGroupName, SOCKET sRemoveClientSock) {
	bool bRemoved = mGroupMap.removeFromGroup(sError, sInfo, sGroupName, sRemoveClientSock);

	if (!bRemoved) {
		return false;
	}

	std::string sPackageRemove = CNetworkDevice::compresPackage(STATE_KICK_USER_FROM_GROUP_BY_ADMIN,
		"U have been removed\n", STATE_KICKED_BY_ADMIN, sGroupName);
	CNetworkDevice::sendPackage(sPackageRemove, sRemoveClientSock);
	return true;
}
bool CServerNetwork::unregisterUser(std::string & sError, CVectorRegister & mRegisterMap, std::string sLogin) {
	return mRegisterMap.unregisterUser(sError, sLogin);
}


// log tools
void CServerNetwork::writeTextToLogFile(const std::string & text) {
	std::ofstream log_file(
		"log_file.txt", std::ios_base::out | std::ios_base::app);
	log_file << text;
}
void CServerNetwork::updateTime(std::string & sCurrentTime) {
	time_t czas;
	struct tm ptr;
	time(&czas);
	char charTime[100];
	localtime_s(&ptr, &czas);
	asctime_s(charTime, &ptr);
	charTime[strlen(charTime) - 1] = '\0';
	sCurrentTime = charTime;
}


// class with pointer (need for map lists)
class CClientResources {
public:

	//// variable
	// pointer on network settings
	CServerNetwork * m_pNetwork;

	// client socket
	SOCKET m_ClientSocket;

	// flag (protecing overwriting above variables)
	bool m_bCopied;

public:

	//// function
	/**/

protected:

	//// variable
	/**/

protected:

	//// function
	/**/

};


// main function to put on ice server
void CServerNetwork::start() {
	
	m_mRegisterMap.start();

	CClientResources ServerInterface;
	ServerInterface.m_pNetwork = this;
	do {
		// if client waiting, accept the connection and save the socket
		ServerInterface.m_ClientSocket = accept(m_sListenSocket, NULL, NULL);
		if (ServerInterface.m_ClientSocket != INVALID_SOCKET) {
			//disable nagle on the client's socket
			char value = 1;
			setsockopt(ServerInterface.m_ClientSocket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
			ServerInterface.m_bCopied = false;
			HANDLE  hThread = (HANDLE)_beginthread((_beginthread_proc_type)recvLoop, 0, (void*)&ServerInterface);
			//WaitForSingleObject(hThread, INFINITE);
			/*
			HANDLE  hThread = CreateThread(
				NULL,                   // default security attributes
				0,                      // use default stack size  
				(LPTHREAD_START_ROUTINE)recvLoop,			// thread function name
				(void*)&ServerInterface,       // argument to thread function
				0,                      // use default creation flags
				NULL);					// returns the thread identifier
				*/
			
			while (!ServerInterface.m_bCopied) {
				Sleep(10);
			}
		}		
	} while (""); // infinit loop
}
// main function for new client (new thread)
DWORD CServerNetwork::recvLoop(void * pData) {

	// take pointer on resources
	CClientResources* pServerInterface = (CClientResources*)pData;
	CClientResources ServerInterface = *pServerInterface;

	// set flag on true
	pServerInterface->m_bCopied = true;

	// init socket
	SOCKET sClientSocket = ServerInterface.m_ClientSocket;

	// init all maps
	CVectorRegister & mRegisterMap = ServerInterface.m_pNetwork->m_mRegisterMap;
	CUserMap & mUserMap = ServerInterface.m_pNetwork->m_mUserMap;
	CVectorGroups & mGroupMap = ServerInterface.m_pNetwork->m_mGroupMap;
	CAdminMap & mAdminMap = ServerInterface.m_pNetwork->m_mAdminMap;

	// recv variable
	std::string sPackage;
	CNetworkDevice ndDevice(sClientSocket);

	// main recv loop
	bool recive = true;
	while (recive){
		recive = ndDevice.receivePackage(sPackage, true); // recving massages from client
		if (!recive) {
			break;
		}
		ServerInterface.m_pNetwork->brain(mRegisterMap, mGroupMap, mUserMap, mAdminMap, sClientSocket, sPackage);
	}
	
	// clean garbage after client
	ServerInterface.m_pNetwork->disconnected(mGroupMap, mUserMap, mAdminMap, sClientSocket);
	_endthread();
	return 0;
}
// server brain (take package -> decompress -> do somthing)
void CServerNetwork::brain(CVectorRegister & mRegisterMap, CVectorGroups & mGroupMap, CUserMap & mUserMap,
	CAdminMap & mAdminMap, SOCKET ClientSocket, std::string sPackage) { // will be harvester !
	
	// recv and send variable
	SOCKET sClientSocket = ClientSocket;
	std::string sNickname = "";
	CNetworkDevice ndDevice(sClientSocket);

	// decompress package -> type, par, message, id
	int iType = -1;
	int iPar = -1;
	std::string sMessage = "";
	std::string sID = "";
	CNetworkDevice::decompresPackage(iType, sMessage, iPar, sID, sPackage);

	// variable for operation feedback
	std::string sError = "";
	std::string sInfo = "";

	// variable for log info
	char cLogInfo [512]; // attention! static variable
	std::string sLogInfo = "";
	
	// PROTOCOL
	// ->Protocol.h
	// END PROTOCOL

	// brain main part
	if (iType < PROTOCOL_LENGTH && iType >= STATE_ERROR) {

		// check if operation succeed
		bool bOperation = false;

		// admdin / user
		bool bAdmin = false;
		bAdmin = mAdminMap.findClientNnByCs(sError, sNickname, sClientSocket);
		bool bUser = false;
		bUser = mUserMap.findClientNnByCs(sError, sNickname, sClientSocket);

		// string with information to client
		std::string sPackageSend = "";

		// time for log info
		std::string sCurrentTime ="";
		updateTime(sCurrentTime);

		// add to log ...
		sprintf_s(cLogInfo, "%s ", sCurrentTime.c_str());
		sLogInfo += cLogInfo;

		// authorization
		switch (iType) {

		 case STATE_REGISTER:
			 sprintf_s(cLogInfo, "%s -> Type: [%s", sMessage.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 bOperation = registerUser(sError, mRegisterMap, sMessage, sID);

			 switch (bOperation) {
			  case STATE_OPERATION_SUCCEED:
				  sprintf_s(cLogInfo, "] -> succeed\n");
				  sPackageSend = CNetworkDevice::compresPackage(STATE_REGISTER, "", STATE_SUCCSSED, "");
				  break;

			  case STATE_OPERATION_FAILED:
				  sprintf_s(cLogInfo, "] -> failed (%s)\n", sError.c_str());
				  sPackageSend = CNetworkDevice::compresPackage(STATE_REGISTER,
					  sError, STATE_FAILED, "");
				  break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_LOGIN:
			 sprintf_s(cLogInfo, "%s -> Type: [%s", sMessage.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

   			 bOperation = login(sError, sInfo, mRegisterMap, mUserMap, mAdminMap, sClientSocket, sMessage, sID);
					
			 switch (bOperation) {
			  case STATE_OPERATION_SUCCEED:
				  sprintf_s(cLogInfo, "] -> succeed\n");
				  sPackageSend = CNetworkDevice::compresPackage(STATE_LOGIN, sMessage, STATE_SUCCSSED, sInfo);
				  break;

			  case STATE_OPERATION_FAILED:
				  sprintf_s(cLogInfo, "] -> failed (%s)\n", sError.c_str());
				  sPackageSend = CNetworkDevice::compresPackage(STATE_LOGIN, sError, STATE_FAILED, "");
				  break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_LOGOUT:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				  sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bAdmin || bUser) {
				 bOperation = logout(sError, mUserMap, mAdminMap, sClientSocket);
				 if (bOperation) {
					 bOperation = abandonAllGroups(sError, sInfo, mGroupMap, sClientSocket);
				 }
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] -> succeed\n");
				 if (sInfo != "") {
					 sLogInfo += cLogInfo;
					 sprintf_s(cLogInfo, "%s SYSTEM -> %s\n", sCurrentTime.c_str(), sInfo.c_str());
				 }
				 sPackageSend = CNetworkDevice::compresPackage(STATE_LOGOUT, "Logout succeed", STATE_SUCCSSED, "");
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "] -> failed (%s)\n", sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_LOGOUT, sError, STATE_FAILED, "");
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_JOIN_GROUP:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				  sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bUser) {
				 bOperation = joinGroup(sError, mGroupMap, sID, sClientSocket, sNickname);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sID.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_JOIN_GROUP, "Join group succeed", STATE_SUCCSSED, sID);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> failed (%s)\n",
					 m_sProIdType[iType].c_str(), sID.c_str(), sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_JOIN_GROUP, sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_ABANDON_GROUP:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				  sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bUser) {
				 bOperation = abandonGroup(sError, sInfo, mGroupMap, sID, sClientSocket);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sID.c_str());
				 if (sInfo != "") {
					 sLogInfo += cLogInfo;
					 sprintf_s(cLogInfo, "%s SYSTEM -> %s\n", sCurrentTime.c_str(), sInfo.c_str());
				 }
				 sPackageSend = CNetworkDevice::compresPackage(STATE_ABANDON_GROUP,
					 "Abandon group succeed", STATE_SUCCSSED, sID);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> failed (%s)\n",
					 m_sProIdType[iType].c_str(), sID.c_str(), sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_ABANDON_GROUP,
					 sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_SEND_MESSAGE:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				  sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 switch (iPar) {
			  case STATE_ERROR:
				  sprintf_s(cLogInfo, "->%s] (%s:[%s]) Message: [%s] -> failed (wrong parameter)\n",
					  m_sProPar[iPar].c_str(), m_sProIdPar[iPar].c_str(), sID.c_str(), sMessage.c_str());
				  sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
					  "Send message failed please contact with administration", STATE_FAILED, "");
				  break;

			  case STATE_TO_ALL:

				  if (bUser) {
					  bOperation = sendToAll(sError, mUserMap, sMessage, sNickname);
				  }
				  switch (bOperation) {
				  case STATE_OPERATION_SUCCEED:
					  sprintf_s(cLogInfo, "->%s] Message: [%s] -> succeed\n",
						  m_sProPar[iPar].c_str(), sMessage.c_str());
					  sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
						  "Send message succeed", STATE_DEFAULT_PAR, "");
					  break;

				  case STATE_OPERATION_FAILED:
					  sprintf_s(cLogInfo, "->%s] Message: [%s] -> failed (%s)\n",
						  m_sProPar[iPar].c_str(), sMessage.c_str(), sError.c_str());
					  sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
						  sError, STATE_DEFAULT_PAR, "");
					  break;
				  }
				  ndDevice.sendPackage(sPackageSend);
				  break;

			  case STATE_TO_GROUP:
				  if (bUser) {
					  bOperation = sendToGroup(sError, mGroupMap, sMessage, sNickname, sID);
				  }
				  switch (bOperation) {
				  case STATE_OPERATION_SUCCEED:
					  sprintf_s(cLogInfo, "->%s] (%s:[%s]) Message: [%s] -> succeed\n",
						  m_sProPar[iPar].c_str(), m_sProIdPar[iPar].c_str(), sID.c_str(), sMessage.c_str());
					  sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
						  "Send message succeed", STATE_DEFAULT_PAR, "");
					  break;

				  case STATE_OPERATION_FAILED:
					  sprintf_s(cLogInfo, "->%s] (%s:[%s]) Message: [%s] -> failed (%s)\n",
						  m_sProPar[iPar].c_str(), m_sProIdPar[iPar].c_str(), sID.c_str(), sMessage.c_str(), sError.c_str());
					  sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
						  sError, STATE_DEFAULT_PAR, "");
					  break;
				  }
				  ndDevice.sendPackage(sPackageSend);
				  break;

			  case STATE_TO_USER:
				  if (bUser) {
					  bOperation = sendToClient(sError, mUserMap, sMessage, sID,
						  sNickname);
				  }
				  switch (bOperation) {
				  case STATE_OPERATION_SUCCEED:
					  sprintf_s(cLogInfo, "->%s] (%s:[%s]) Message: [%s] -> succeed\n",
						  m_sProPar[iPar].c_str(), m_sProIdPar[iPar].c_str(), sID.c_str(), sMessage.c_str());
					  sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
						  "Send message succeed", STATE_DEFAULT_PAR, "");
					  break;

				  case STATE_OPERATION_FAILED:
					  sprintf_s(cLogInfo, "->%s] (%s:[%s]) Message: [%s] -> failed (%s)\n",
						  m_sProPar[iPar].c_str(), m_sProIdPar[iPar].c_str(), sID.c_str(), sMessage.c_str(), sError.c_str());
					  sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
						  sError, STATE_DEFAULT_PAR, "");
					  break;
				  }
				  ndDevice.sendPackage(sPackageSend);
				  break;
			 }
			 break;

		 case STATE_CREATE_GROUP:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				 sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bUser) {
				 bOperation = createGroup(sError, mGroupMap, sID, sClientSocket, sNickname);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sID.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_CREATE_GROUP,
					 "Create group succeed", STATE_SUCCSSED, sID);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> failed (%s)\n",
					 m_sProIdType[iType].c_str(), sID.c_str(), sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_CREATE_GROUP, 
					 sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_CHANGE_LEADER:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				  sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bUser) {
				 bOperation = changeLeader(sError, mGroupMap, sID, sMessage);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sMessage.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_CHANGE_LEADER,
					 "Change leader succeed", STATE_SUCCSSED, sID);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> failed (%s)\n",
					 m_sProIdType[iType].c_str(), sMessage.c_str(), sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_CHANGE_LEADER,
					 sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_KICK_USER_FROM_GROUP:
		 {
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				  sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 SOCKET sRemoveClient;
			 mUserMap.findClientCsByNn(sError, sRemoveClient, sMessage);

			 if (bUser) {
				 bOperation = kickFromGroup(sError, sInfo, mGroupMap, sID, sRemoveClient);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sMessage.c_str());
				 if (sInfo != "") {
					 sLogInfo += cLogInfo;
					 sprintf_s(cLogInfo, "%s SYSTEM -> %s", sCurrentTime.c_str(), sInfo.c_str());
				 }
				 sPackageSend = CNetworkDevice::compresPackage(STATE_KICK_USER_FROM_GROUP,
					 "Remove user from group succeed", STATE_SUCCSSED, sMessage);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> failed\n", m_sProIdType[iType].c_str(), sMessage.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_KICK_USER_FROM_GROUP,
					 sError, STATE_FAILED, sMessage);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;
		 }
		 case STATE_GROUP_MEMBERS_LIST:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				  sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bUser) {
				 bOperation = sendGroupMembersList(sError, mGroupMap, sClientSocket, sID);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sID.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_GROUP_MEMBERS_LIST,
					 "Group list received succeed", STATE_SUCCSSED, sID);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> failed (%s)\n",
					 m_sProIdType[iType].c_str(), sID.c_str(), sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_GROUP_MEMBERS_LIST,
					 sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_VOTE_KICK:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				  sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bUser) {
				 bOperation = voteKick(sError, sInfo, mGroupMap, sMessage, sID);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sID.c_str());
				 if (sInfo != "") {
					 sLogInfo += cLogInfo;
					 sprintf_s(cLogInfo, "%s SYSTEM -> %s", sCurrentTime.c_str(), sInfo.c_str());
				 }
				 sPackageSend = CNetworkDevice::compresPackage(STATE_VOTE_KICK,
					 "Send vote kick succeed", STATE_SUCCSSED, sID);
					 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> failed (%s)\n",
					 m_sProIdType[iType].c_str(), sID.c_str(), sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_VOTE_KICK,
					 sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_INVITE_USER_TO_GROUP:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				  sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 SOCKET sClientInvSock;
			 if (bUser) {
				 bOperation = mUserMap.findClientCsByNn(sError, sClientInvSock, sMessage);
				 if (bOperation) {
					 bOperation = inviteToGroup(sError, mGroupMap, sID, sClientInvSock, sNickname);
				 }
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sMessage.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_INVITE_USER_TO_GROUP,
					 "Invite send", STATE_SUCCSSED, sID);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> failed (%s)\n",
					 m_sProIdType[iType].c_str(), sMessage.c_str(), sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_INVITE_USER_TO_GROUP,
					 sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;
			 
		 case STATE_REQUEST_INVITE_USER_TO_GROUP:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				  sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bUser) {
				 bOperation = requestInviteUser(sError, mGroupMap, sID, sNickname, sMessage);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sID.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_REQUEST_INVITE_USER_TO_GROUP,
					 "Request send", STATE_SUCCSSED, sID);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> failed (%s)\n",
					 m_sProIdType[iType].c_str(), sID.c_str(), sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_REQUEST_INVITE_USER_TO_GROUP,
					 sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_CHANGE_GROUP_NAME:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				  sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bUser) {
				 bOperation = changeGroupName(sError, mGroupMap, sNickname, sID, sMessage);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n",
					 m_sProIdType[iType].c_str(), sMessage.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_CHANGE_GROUP_NAME,
					 "Group name was changed", STATE_SUCCSSED, sMessage);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> failed (%s)\n",
					 m_sProIdType[iType].c_str(), sID.c_str(), sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_CHANGE_GROUP_NAME,
					 sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_USERS_LIST:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				 sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bAdmin) {
				 bOperation = loginUsersList(sError, mUserMap, sClientSocket);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] -> succeed\n");
				 sPackageSend = CNetworkDevice::compresPackage(STATE_USERS_LIST,
					 sMessage, STATE_SUCCSSED, sID);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "]  -> failed (%s)\n", sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_USERS_LIST,
					 sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_ALL_GROUPS_LIST:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				 sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bAdmin) {
				 bOperation = allGroupsList(sError, mGroupMap, sClientSocket);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] -> succeed\n");
				 sPackageSend = CNetworkDevice::compresPackage(STATE_ALL_GROUPS_LIST,
					 sMessage, STATE_SUCCSSED, sID);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "]  -> failed (%s)\n", sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_ALL_GROUPS_LIST,
					 sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_DELETE_GROUP:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				 sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bAdmin) {
				 bOperation = deleteGroupByAdmin(sError, mGroupMap, sID);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] -> (%s:[%s]) succeed\n",
					 m_sProIdType[iType].c_str(), sID.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_DELETE_GROUP,
					 sMessage, STATE_SUCCSSED, sID);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "]  (%s:[%s]) -> failed (%s)\n",
					 m_sProIdType[iType].c_str(), sID.c_str(), sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_DELETE_GROUP,
					 sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;

		 case STATE_KICK_USER_FROM_GROUP_BY_ADMIN:
		 {
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				 sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bAdmin) {
				 SOCKET sRemoveClientSock = SOCKET_ERROR;
				 bOperation = mUserMap.findClientCsByNn(sError, sRemoveClientSock, sMessage);
				 if (bOperation) {
					 bOperation = kickFromGroupByAdmin(sError, sInfo, mGroupMap, sID, sRemoveClientSock);
				 }
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sMessage.c_str());
				 if (sInfo != "") {
					 sLogInfo += cLogInfo;
					 sprintf_s(cLogInfo, "%s SYSTEM -> %s\n", sCurrentTime.c_str(), sInfo.c_str());
				 }
				 sPackageSend = CNetworkDevice::compresPackage(STATE_KICK_USER_FROM_GROUP,
					 "Remove user from group succeed", STATE_SUCCSSED, sMessage);
				 break;

			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "] (%s:[%s]) -> failed\n", m_sProIdType[iType].c_str(), sMessage.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_KICK_USER_FROM_GROUP,
					 sError, STATE_FAILED, sMessage);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;
		 }

		 case STATE_UNREGISTER_USER:
			 sprintf_s(cLogInfo, "%s -> Type: [%s",
				 sNickname.c_str(), m_sProType[iType].c_str());
			 sLogInfo += cLogInfo;

			 if (bAdmin) {
				 bOperation = unregisterUser(sError, mRegisterMap, sID);
			 }
			 switch (bOperation) {
			 case STATE_OPERATION_SUCCEED:
			 {
				 sprintf_s(cLogInfo, "] -> (%s:[%s]) succeed\n",
					 m_sProIdType[iType].c_str(), sID.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_UNREGISTER_USER,
					 sMessage, STATE_SUCCSSED, sID);

				 // logout user if he was login
				 SOCKET sUnregisterSock = SOCKET_ERROR;
				 bool bLoginUser = mUserMap.findClientCsByNn(sError, sUnregisterSock, sID);
				 if (bLoginUser) {
					 std::string sUnregisterPackageSend = CNetworkDevice::compresPackage(STATE_UNREGISTER_USER,
						 "", STATE_UNREGISTER_BY_ADMIN, sID);
					 CNetworkDevice::sendPackage(sUnregisterPackageSend, sUnregisterSock);
				 }
				 break;
			 }
			 case STATE_OPERATION_FAILED:
				 sprintf_s(cLogInfo, "]  (%s:[%s]) -> failed (%s)\n",
					 m_sProIdType[iType].c_str(), sID.c_str(), sError.c_str());
				 sPackageSend = CNetworkDevice::compresPackage(STATE_UNREGISTER_USER,
					 sError, STATE_FAILED, sID);
				 break;
			 }
			 ndDevice.sendPackage(sPackageSend);
			 break;
		}

		// TEMPLATE !!!!
		/*
		 case *state*:
			bOperation = mUserMap.findClientNnByCs(sError, sNickname, sClientSocket);
			sprintf(*log info print on screan, to txt file*);
			sLogInfo += cLogInfo;
			
			if (bOperation){
				bOperation = *operation*;
			}
			switch (bOperation){
			 case STATE_OPERATION_SUCCEED:
				*do somthing*
				break;
			 
			 case STATE_OPERATION_FAILED:
				*do somthing*;
				break;
			}
			ndDevice.sendPackage(*package*);
			break;
		*/

		// OLD BRAIN
		/*
		if (iType == STATE_AUTHORIZATION) {
			sprintf_s(cLogInfo, "%s -> Type: [%s", sID.c_str(), m_sProType[iType].c_str());
			sLogInfo += cLogInfo;

			bOperation = mAuthorizationMap.addClientToMap(sClientSocket, sID.c_str());

			if (bOperation) {
				sprintf_s(cLogInfo, "] -> succeed\n");
				sPackageSend = CNetworkDevice::compresPackage(STATE_AUTHORIZATION, "", STATE_SUCCSSED, "");
			} else {
				sprintf_s(cLogInfo, "] -> failed\n");
				sPackageSend = CNetworkDevice::compresPackage(STATE_AUTHORIZATION,
					"u can not be authorized second time", STATE_FAILED, "");
			}

			ndDevice.sendPackage(sPackageSend);

		// login
		} else if (iType == STATE_LOGIN) {
			sprintf_s(cLogInfo, "%s -> Type: [%s", sID.c_str(), m_sProType[iType].c_str());
			sLogInfo += cLogInfo;

			bOperation = login(mUserMap, sClientSocket, mAuthorizationMap.findClientNnByCs(sClientSocket));

			if (bOperation) {
				sprintf_s(cLogInfo, "] -> succeed\n");
				sPackageSend = CNetworkDevice::compresPackage(STATE_LOGIN, "Login succeed", STATE_SUCCSSED, "");
			}
			else {
				sprintf_s(cLogInfo, "] -> failed\n");
				sPackageSend = CNetworkDevice::compresPackage(STATE_LOGIN, "Login failed", STATE_FAILED, "");
			}

			ndDevice.sendPackage(sPackageSend);

		// logout
		} else if (iType == STATE_LOGOUT) {
			sprintf_s(cLogInfo, "%s -> Type: [%s", sID.c_str(), m_sProType[iType].c_str());
			sLogInfo += cLogInfo;

			bOperation = logout(mUserMap, sClientSocket);

			if (bOperation) {
				sprintf_s(cLogInfo, "] -> succeed\n");
				sPackageSend = CNetworkDevice::compresPackage(STATE_LOGOUT, "Logout succeed", STATE_SUCCSSED, "");
			}
			else {
				sprintf_s(cLogInfo, "] -> failed\n");
				sPackageSend = CNetworkDevice::compresPackage(STATE_LOGOUT, "Logout failed", STATE_FAILED, "");
			}

			ndDevice.sendPackage(sPackageSend);
		
		// joinGroup
		} else if (iType == STATE_JOIN_GROUP) {
			sprintf_s(cLogInfo, "%s -> Type: [%s",
				mAuthorizationMap.findClientNnByCs(sClientSocket).c_str(),
				m_sProType[iType].c_str());
			sLogInfo += cLogInfo;

			bOperation = addClientToGroup(Group, sID, sClientSocket, mAuthorizationMap.findClientNnByCs(sClientSocket));

			if (bOperation) {
				sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sID.c_str());
				sPackageSend = CNetworkDevice::compresPackage(STATE_JOIN_GROUP, "Join group succeed", STATE_SUCCSSED, sID);
			}
			else {
				sprintf_s(cLogInfo, "] (%s:[%s]) -> failed\n", m_sProIdType[iType].c_str(), sID.c_str());
				sPackageSend = CNetworkDevice::compresPackage(STATE_JOIN_GROUP, "Join group failed", STATE_FAILED, sID);
			}

			ndDevice.sendPackage(sPackageSend);

		// abandonGroup
		} else if (iType == STATE_ABANDON_GROUP) {
			sprintf_s(cLogInfo, "%s -> Type: [%s",
				mAuthorizationMap.findClientNnByCs(sClientSocket).c_str(),
				m_sProType[iType].c_str());
			sLogInfo += cLogInfo;

			bOperation = abandonGroup(Group, sID, sClientSocket);

			if (bOperation) {
				sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sID.c_str());
				sPackageSend = CNetworkDevice::compresPackage(STATE_ABANDON_GROUP,
					"Abandon group succeed", STATE_SUCCSSED, sID);
			}
			else {
				sprintf_s(cLogInfo, "] (%s:[%s]) -> failed\n", m_sProIdType[iType].c_str(), sID.c_str());
				sPackageSend = CNetworkDevice::compresPackage(STATE_ABANDON_GROUP,
					"Abandon group failed", STATE_FAILED, sID);
			}

			ndDevice.sendPackage(sPackageSend);

		// send message to ...
		} else if (iType == STATE_SEND_MESSAGE) {

			sprintf_s(cLogInfo, "%s -> Type: [%s",
				mAuthorizationMap.findClientNnByCs(sClientSocket).c_str(),
				m_sProType[iType].c_str());
			sLogInfo += cLogInfo;

			// ... ERROR (par was equal 0)
			if (iPar == STATE_ERROR) {
				sprintf_s(cLogInfo, "->%s] (%s:[%s]) Message: [%s] -> failed (wrong parameter)\n",
					m_sProPar[iPar].c_str(), m_sProIdPar[iPar].c_str(), sID.c_str(), sMessage.c_str());
				sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
					"Send message failed (wrong parameter)", STATE_FAILED, "");
			
			// ... all
			} else if (iPar == STATE_TO_ALL) {	
				bOperation = sendToAll(sMessage, mAuthorizationMap, mAuthorizationMap.findClientNnByCs(sClientSocket));

				if (bOperation) {
					sprintf_s(cLogInfo, "->%s] Message: [%s] -> succeed\n",
						m_sProPar[iPar].c_str(), sMessage.c_str());
					sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
						"Send message succeed", STATE_DEFAULT_PAR, "");
				}
				else {
					sprintf_s(cLogInfo, "->%s] Message: [%s] -> failed\n",
						m_sProPar[iPar].c_str(), sMessage.c_str());
					sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
						"Send message failed", STATE_DEFAULT_PAR, "");
				}

				ndDevice.sendPackage(sPackageSend);

			// ... group
			} else if (iPar == STATE_TO_GROUP) {
				bOperation = sendToGroup(sMessage, Group, mAuthorizationMap.findClientNnByCs(sClientSocket), sID);

				if (bOperation) {
					sprintf_s(cLogInfo, "->%s] (%s:[%s]) Message: [%s] -> succeed\n",
						m_sProPar[iPar].c_str(), m_sProIdPar[iPar].c_str(), sID.c_str(), sMessage.c_str());
					sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
						"Send message succeed", STATE_DEFAULT_PAR, "");
				}
				else {
					sprintf_s(cLogInfo, "->%s] (%s:[%s]) Message: [%s] -> failed\n",
						m_sProPar[iPar].c_str(), m_sProIdPar[iPar].c_str(), sID.c_str(), sMessage.c_str());
					sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
						"Send message failed", STATE_DEFAULT_PAR, "");
				}

				ndDevice.sendPackage(sPackageSend);

			// ... client
			} else if (iPar == STATE_TO_USER) {
				SOCKET sSendToSocket = mAuthorizationMap.findClientCsByNn(sID);

				if (sSendToSocket != SOCKET_ERROR) {
					sprintf_s(cLogInfo, "->%s] (%s:[%s]) Message: [%s] -> succeed\n",
						m_sProPar[iPar].c_str(), m_sProIdPar[iPar].c_str(), sID.c_str(), sMessage.c_str());
					sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
						"Send message succeed", STATE_DEFAULT_PAR, "");

					ndDevice.sendPackage(sPackageSend);
					sendToClient(sMessage, sSendToSocket, mAuthorizationMap.findClientNnByCs(sClientSocket));

				} else {
					sprintf_s(cLogInfo, "->%s] (%s:[%s]) Message: [%s] -> failed\n",
						m_sProPar[iPar].c_str(), m_sProIdPar[iPar].c_str(), sID.c_str(), sMessage.c_str());
					sPackageSend = CNetworkDevice::compresPackage(STATE_SEND_MESSAGE,
						"Send message failed", STATE_DEFAULT_PAR, "");

					ndDevice.sendPackage(sPackageSend);

				}
			}

		// createGroup
		} else if (iType == STATE_CREATE_GROUP) {

			sprintf_s(cLogInfo, "%s -> Type: [%s", mAuthorizationMap.findClientNnByCs(sClientSocket).c_str(),
				m_sProType[iType].c_str());
			sLogInfo += cLogInfo;

			bOperation = createGroup(Group, sID, sClientSocket, mAuthorizationMap.findClientNnByCs(sClientSocket));

			if (bOperation) {
				sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sID.c_str());
				sPackageSend = CNetworkDevice::compresPackage(STATE_CREATE_GROUP,
					"Create group succeed", STATE_SUCCSSED, sID);
			}
			else {
				sprintf_s(cLogInfo, "] (%s:[%s]) -> failed\n", m_sProIdType[iType].c_str(), sID.c_str());
				sPackageSend = CNetworkDevice::compresPackage(STATE_CREATE_GROUP, "Create group failed",
					STATE_FAILED, sID);
			}

			ndDevice.sendPackage(sPackageSend);

		// changeLeader (only for leader)
		} else if (iType == STATE_CHANGE_LEADER) {

			sprintf_s(cLogInfo, "%s -> Type: [%s", mAuthorizationMap.findClientNnByCs(sClientSocket).c_str(),
				m_sProType[iType].c_str());
			sLogInfo += cLogInfo;

			bOperation = changeLeader(Group, sID, sMessage, mAuthorizationMap.findClientCsByNn(sMessage));

			if (bOperation) {
				sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sMessage.c_str());
				sPackageSend = CNetworkDevice::compresPackage(STATE_CHANGE_LEADER,
					"Change leader succeed", STATE_SUCCSSED, sID);
			}
			else {
				sprintf_s(cLogInfo, "] (%s:[%s]) -> failed\n", m_sProIdType[iType].c_str(), sMessage.c_str());
				sPackageSend = CNetworkDevice::compresPackage(STATE_CHANGE_LEADER,
					"Change leader failed", STATE_FAILED, sID);
			}

			ndDevice.sendPackage(sPackageSend);

		// kickClientFromGroup (only for leader)
		} else if (iType == STATE_KICK_USER_FROM_GROUP) {

			sprintf_s(cLogInfo, "%s -> Type: [%s", mAuthorizationMap.findClientNnByCs(sClientSocket).c_str(),
				m_sProType[iType].c_str());
			sLogInfo += cLogInfo;

			SOCKET sRemoveClient = mAuthorizationMap.findClientCsByNn(sMessage);

			bOperation = kickFromGroup(Group, sID, sRemoveClient);

			if (bOperation) {
				sprintf_s(cLogInfo, "] (%s:[%s]) -> succeed\n", m_sProIdType[iType].c_str(), sMessage.c_str());
				sPackageSend = CNetworkDevice::compresPackage(STATE_KICK_USER_FROM_GROUP,
					"Remove user from group succeed", STATE_SUCCSSED, sMessage);
				std::string sPackageRemove = CNetworkDevice::compresPackage(STATE_KICK_USER_FROM_GROUP,
					"U have been removed\n", STATE_KICKED, sMessage);
				CNetworkDevice::sendPackage(sPackageRemove, sRemoveClient);
			}
			else {
				sprintf_s(cLogInfo, "] (%s:[%s]) -> failed\n", m_sProIdType[iType].c_str(), sMessage.c_str());
				sPackageSend = CNetworkDevice::compresPackage(STATE_KICK_USER_FROM_GROUP,
					"Remove user from group failed", STATE_FAILED, sMessage);
			}

			ndDevice.sendPackage(sPackageSend);
		}*/

		sLogInfo += cLogInfo;

	// ERROR type out of range
	} else {
		bool bOperation = mUserMap.findClientNnByCs(sError, sNickname, sClientSocket);

		if (bOperation) {
			sprintf_s(cLogInfo, "%s -> Type out of range [%d]\n", sNickname.c_str(), iType);
			std::string sPackageSend = CNetworkDevice::compresPackage(-1, "Error type out of range [%d]\n", STATE_ERROR, "");
		} else {
			sprintf_s(cLogInfo, "? -> Type out of range [%d]\n", iType);
		}
		std::string sPackageSend = CNetworkDevice::compresPackage(-1, "Error type out of range [%d]\n", STATE_ERROR, "");
		ndDevice.sendPackage(sPackageSend);
		sLogInfo += cLogInfo;
	}

	// print to consol
	printf("%s",sLogInfo.c_str());
	
	// print to txt file
	writeTextToLogFile(sLogInfo);
}
