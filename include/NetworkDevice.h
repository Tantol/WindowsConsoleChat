#pragma once
#include <winsock2.h>
#include <Windows.h>
#include <string>
#include <ostream>

// size of our buffer
#define BUFF_SIZE 512


class CNetworkDevice {

public:

	//// variable
		/**/

public:

	//// function
	// const. / deconst.
	CNetworkDevice(SOCKET sConnectSocket) {
		m_sConnectSocket = sConnectSocket;
		m_sRemainder = "";
	};
	~CNetworkDevice() {
		//deconst.
	};

	// function to send package (Client -> Server // Server -> Client)
	bool sendPackage(std::string & sPackage) {
		hashPackage(sPackage);
		int iSend = send(m_sConnectSocket, sPackage.c_str(), sPackage.size(), 0);

		if (iSend != SOCKET_ERROR){
			return true;
		} else {
			return false;
		}
	};

	// function to send package (Client -> Server // Server -> Client)
	static bool sendPackage(std::string & sPackage, SOCKET sClientSock) {
		hashPackage(sPackage);
		int iSend = send(sClientSock, sPackage.c_str(), sPackage.size(), 0);

		if (iSend != SOCKET_ERROR) {
			return true;
		}
		else {
			return false;
		}
	};

	// function to receive package (Client -> Server // Server -> Client)
	bool receivePackage(std::string & sPackage, bool bWait) {
		char buffor[BUFF_SIZE];
		std::string sTmpBuff;
		std::string Zero(1, 0);
		int iEndLoop = 0;
		timeval timeout;

		FD_ZERO(&m_ReadSet);

		FD_SET(m_sConnectSocket, &m_ReadSet);
		int iTotal = -1;

		sPackage = m_sRemainder;

		// check m_sRemainder
		do {
			// unhash
			unhashPackage(sPackage);

			iEndLoop = 0;
			for (size_t iIter = 0; iIter < sPackage.length(); iIter++) {
				if (sPackage[iIter] == 0) {
					iEndLoop++;
					if (iEndLoop == 4) {
						m_sRemainder = sPackage.substr(iIter+1);
						hashPackage(m_sRemainder);
						sPackage = sPackage.substr(0, iIter);
						return true;
					}
				}
			}
		// check m_sRemainder end

			// if bWait = true -> receive will be non-blocking
			if (!bWait) {
				iTotal = select(m_sConnectSocket + 1, & m_ReadSet, NULL, NULL, &timeout);

				if (iTotal != 1) {
					return false;
				}
			}

			int iRet = recv(m_sConnectSocket, buffor, BUFF_SIZE, 0);

			if (iRet == 0) {
				//printf("%i\n", WSAGetLastError());
				return false;
			}
			else if (iRet < 0) {
				//printf("%i\n", WSAGetLastError());
				return false;
			}

			sTmpBuff.assign(buffor, iRet);
			sPackage += sTmpBuff;

		} while (1);
		
		return true;
	};

	// static function to compress package
	static std::string compresPackage(int iType, const std::string & sMessage, int iPar, std::string sID) {

		std::string Zero(1, 0);
		std::string sPackage = std::to_string(iType) + Zero +
			sMessage + Zero +
			std::to_string(iPar) + Zero +
			sID + Zero;
		return sPackage;
	};

	// static function to decompress package
	static void decompresPackage(int & iType, std::string & sMessage, int & iPar, std::string & sID,
								 const std::string & cPackage) {
		

		//security for package length
		if (cPackage.length() <= 0) {
			return;
		}

		// decompress type
		iType = atoi(cPackage.c_str());

		// decompress message
		int iTypeLen = strlen(cPackage.c_str());
		const char* pMessage = cPackage.c_str() + iTypeLen + 1;
		sMessage.assign(pMessage);

		// decompress par
		int iMessageLen = sMessage.length();
		const char* pPar = pMessage + iMessageLen + 1;
		iPar = atoi(pPar);

		// decompress id
		int iParLen = strlen(pPar);
		const char* pID = pPar + iParLen + 1;
		sID.assign(pID);
	};

	static void hashPackage(std::string & sPackage) {

		for (size_t i = 0; i < sPackage.length(); i++) {
			sPackage[i] ^= 0x55;
		}
	
	};

	static void unhashPackage(std::string & sPackage) {

		for (size_t i = 0; i < sPackage.length(); i++) {
			sPackage[i] ^= 0x55;
		}
	
	};
	/* TODO <<<
	static std::string hashPassword() {};
	static std::string unhashPassword() {};
	*/
protected:

	//// variable

	// socket
	SOCKET m_sConnectSocket;

	// remainder for receive function
	std::string m_sRemainder;

	// fd_sets for select function
	FD_SET m_WriteSet;
	FD_SET m_ReadSet;

protected:

	//// function
		/**/

};