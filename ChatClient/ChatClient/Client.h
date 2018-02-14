#pragma once
#include <winsock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include <string>
#include <mutex>
#include <iostream>
#include <stdio.h>
#include <conio.h>
#include <vector>

#include "ClientNetwork.h"

// define protocol
#include "Protocol.h"

// class with protected vector (by mutex)
class CVecotr {

public:
	//const. / deconst.
	CVecotr(void) {
	};
	~CVecotr(void) {
	};

	// add string to vector
	void add(std::string sChars) {
		m_mMtxVector.lock();
		m_vKeyboard.push_back(sChars);
		m_mMtxVector.unlock();
	}

	// remove last string from vector
	void remove() {
		m_mMtxVector.lock();
		m_vKeyboard.pop_back();
		m_mMtxVector.unlock();
	}

	// check if vector is empty
	bool isEmpty() {
		m_mMtxVector.lock();
		bool bEmpty = m_vKeyboard.empty();
		m_mMtxVector.unlock();
		return bEmpty;
	}

	// get front string from vector
	std::string getFront() {
		m_mMtxVector.lock();
		std::string sVar = m_vKeyboard.front();
		m_mMtxVector.unlock();
		return sVar;
	}

protected:

	// vector with chars from keyboard
	std::vector<std::string> m_vKeyboard;

	// mutex -> secure vector
	mutable std::mutex m_mMtxVector;

};

class CClient {

public:

	//// variable
		/**/

public:

	//// function

	// Const. / Dec.
	CClient(void);
	CClient(std::string sNickName, std::string sPassword);
	~CClient(void);

	// keep one's ear to the ground 
	void recvQuestion();

	// if client typed somthing -> send message to server // else -> do nothing
	void sendQuestion();

	// take chars from user keyboard (after -> Enter)
	void loopKeyboard();




protected:

	//// variable

	// client network
	CClientNetwork* m_pNetwork;

	// client unique nickname
	std::string m_sNickname;

	// client status
	int m_iLogin;
	int m_iGroup;
	std::string m_sGroupName;

	// vector with chars from keyboard
	CVecotr m_vKeyboard;

	// graf
	bool m_bSendMessage;
	int m_iType;
	std::string m_sTypeName;

	// TODO WYKMINIC INNY SPOSOB
	std::string m_sNicknameInvUser;

	// hide / show consol input
	HANDLE m_hHideShowInput;
	DWORD m_mHideShowInput;

protected:

	//// function

	// log and logout devices
	bool login(std::string sNickname, std::string sPassword);
	bool logout();
	bool registerUser(std::string sLogin, std::string sPassword);


	// group devices
	bool joinGroup(std::string sGroupName);
	bool abandonGroup();
	bool groupMembersList();
	bool removeFromGroup(std::string sNickname);
	bool changeLeader(std::string sNickname);
	bool createGroup(std::string sGroupName);
	bool voteKick(std::string sNickname);
	bool inviteUser(std::string sNickname);
	bool requestInviteUserToGroup(std::string sNickname);
	bool changeGroupName(std::string sNewGroupName);


	// admin devices
	bool loginUsersList();
	bool allGroupsList();
	bool deleteGroup(std::string sGroupName);
	bool removeFromGroupByAdmin(std::string sGroupName, std::string sNickname);
	bool unregisterUser(std::string sLogin);


	// send message devices
	bool sendMessageToAll(std::string sMessage); // to all
	bool sendMessageToGroup(std::string sMessage); // to group
	bool sendMessageToUser(std::string sMessage, std::string sNickName); // to other client


	// send templates
	bool sendMessage(const std::string & sMessage, int parameter, std::string sID);
	bool sendPackage(const std::string & sMessage, int iParameter, std::string sID, int iType);


	// receive templates
	bool receivePackageBlocking(std::string & sPackage);
	bool receivePackageNonBlocking(std::string & sPackage);


	// main function
	// take package -> do somthing with it
	void recvBrain(std::string sPackage);
	// take char / chars from vector -> do somthing with it
	void sendBrain(std::string sVar);

	// print menu on screan
	void printMenu();
	void groupMenu();

};