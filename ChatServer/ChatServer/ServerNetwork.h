#pragma once
#include <string>
#include <winsock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include <map>
#include <fstream>
#include <time.h>
#include <stdio.h>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <iostream>
#include <sstream>
#include <process.h>

#include "NetworkDevice.h"

// define PROTOCOL
#include "Protocol.h"

#pragma comment (lib, "Ws2_32.lib")

// server port
#define DEFAULT_PORT "6881"


// class group
class CGroup {

public:

	//// variable
	/**/

public:

	//// function

	// const. / denost. 
	CGroup(std::string sGroupName, std::string sGLNickname, SOCKET sGLSocket) {
		m_sGroupName = sGroupName;
		m_sGLNickname = sGLNickname;
		m_sGLSocket = sGLSocket;
	};
	~CGroup(void) {
	};

	// add client to group
	bool addToGroup(std::string & sError, SOCKET sClientSock, std::string sNickname) {
		bool bIsInGroup = checkIfClientIsInGroupBy(sClientSock);

		if (bIsInGroup) {
			sError = sNickname + " is already member of " + m_sGroupName + " group";
			return false;
		}

		m_mGroupMap.insert(std::pair<SOCKET, std::string>(sClientSock, sNickname));
		return true;
	};

	// change group leader
	bool changeLeader(std::string & sError, std::string sGLNickname) {
		if (m_sGLNickname == sGLNickname) {
			sError = sGLNickname + " is already the leader in " + m_sGroupName + " group";
			return false;
		}
		SOCKET sGLSocket;
		bool bInGroup = checkIfClientIsInGroupByAndRecv(sGLSocket, sGLNickname);

		if (!bInGroup) {
			sError = sGLNickname + " is not a member of " + m_sGroupName + " group";
			return false;
		}


		m_sGLNickname = sGLNickname;
		m_sGLSocket = sGLSocket;
		//sInfo = m_sGLNickname + " in now the leader of " + m_sGroupName + " group\n";
		std::string sPackageChange = CNetworkDevice::compresPackage(STATE_CHANGE_LEADER,
			"U have been promoted\n", STATE_PROMOTED, m_sGroupName);
		CNetworkDevice::sendPackage(sPackageChange, sGLSocket);
		return true;
	}
	
	// remove client from group (by SOCKET)
	bool removeFromGroup(std::string & sError, std::string & sInfo, SOCKET sClientSock) {
		if (m_mGroupMap.find(sClientSock) == m_mGroupMap.end()) {
			sError = "This client is not a member of " + m_sGroupName + " group";
			return false;
		} else {
			m_mGroupMap.erase(sClientSock); // remove client from map
			if (m_sGLSocket == sClientSock) {
				mapType::iterator iLeader = m_mGroupMap.begin();
				if (iLeader != m_mGroupMap.end()) {
					m_sGLNickname = iLeader->second; // change
					m_sGLSocket = iLeader->first;    // leader
					std::string sPromoted = CNetworkDevice::compresPackage(STATE_CHANGE_LEADER,
						"U have been promoted\n", STATE_PROMOTED, m_sGroupName);
					CNetworkDevice::sendPackage(sPromoted, m_sGLSocket);
					sInfo = m_sGLNickname +" is now the leader of " + m_sGroupName + " group";
				}
			}
			return true;
		}
	};

	// remove client from group (by NICKNAME)
	bool removeFromGroup(std::string & sError, std::string & sInfo, std::string sNickname) {
		mapType::iterator iter;
		for (iter = m_mGroupMap.begin(); iter != m_mGroupMap.end(); iter++) {
			if (iter->second == sNickname) {
				m_mGroupMap.erase(iter->first);
				if (m_sGLNickname == sNickname) {
					mapType::iterator iLeader = m_mGroupMap.begin();
					m_sGLNickname = iLeader->second;
					m_sGLSocket = iLeader->first;
					std::string sPromoted = CNetworkDevice::compresPackage(STATE_CHANGE_LEADER,
						"U have been promoted\n", STATE_PROMOTED, m_sGroupName);
					CNetworkDevice::sendPackage(sPromoted, m_sGLSocket);
					sInfo = m_sGLNickname + " is now the leader of " + m_sGroupName + " group";
				}
				return true;
			}
		}
		sError = sNickname + " is not a member of " + m_sGroupName + " group";
		return false;
	};

	// get group name
	std::string getName() {
		std::string sGroupName = m_sGroupName;
		return sGroupName;
	};

	// send message to all client from group
	bool sendMessageToAll(std::string sPackage) {
		mapType::iterator iter;
		for (iter = m_mGroupMap.begin(); iter != m_mGroupMap.end(); iter++) {
			CNetworkDevice::sendPackage(sPackage, iter->first);
		}
		return true;
	};

	// check if group is empty
	bool isEmpty() {
		bool bEmpty = m_mGroupMap.empty();	
		return bEmpty;
	};

	// list with all members
	bool sendMembersList(std::string & sError, SOCKET sClientSock) {
		mapType::iterator listIter = m_mGroupMap.begin();
		if (listIter == m_mGroupMap.end()) {
			sError = m_sGroupName + " group is empty";
			return false;
		}
		std::string sMembersList;
		for (listIter; listIter != m_mGroupMap.end(); listIter++) {
			
			if (listIter->second == m_sGLNickname) {
				sMembersList += listIter->second + " [LEADER]\n";
			} else {
				sMembersList += listIter->second + "\n";
			}
		}

		sMembersList = CNetworkDevice::compresPackage(STATE_GROUP_MEMBERS_LIST,
			sMembersList, STATE_GROUP_LIST, m_sGroupName);
		CNetworkDevice::sendPackage(sMembersList, sClientSock);

		return true;
	};

	// vote kick (if ...)
	bool voteKick(std::string & sError, std::string & sInfo, std::string sNickname) {
		SOCKET sClientSock;
		bool bIsInGroup = checkIfClientIsInGroupByAndRecv(sClientSock, sNickname);
		if (!bIsInGroup) {
			sError = sNickname + " is not a member of " + m_sGroupName + " group";
			return false;
		}
		if (sNickname == m_sGLNickname) {
			sError = sNickname + " is the leader of " + m_sGroupName + " group. U can not kick group leader";
			return false;
		}

		if (m_mVoteKick.find(sNickname) == m_mVoteKick.end()) {
			m_mVoteKick.insert(std::pair<std::string, int>(sNickname, 1));
			sInfo = sNickname + " have " + std::to_string(m_mVoteKick.find(sNickname)->second) +
				"/" + std::to_string((int)m_mGroupMap.size() - 2) + " votes too kick\n";
			return true;
		}

		m_mVoteKick.find(sNickname)->second = m_mVoteKick.find(sNickname)->second + 1;

		if (m_mVoteKick.find(sNickname)->second <= (int)m_mGroupMap.size() - 2) { // dopracowac
			m_mGroupMap.erase(sClientSock);
			m_mVoteKick.erase(sNickname);

			std::string sPackageRemove = CNetworkDevice::compresPackage(STATE_KICK_USER_FROM_GROUP,
				"U have been removed\n", STATE_KICKED, m_sGroupName);
			CNetworkDevice::sendPackage(sPackageRemove, sClientSock);
			sInfo = sNickname + " was kicked from " + m_sGroupName + " group\n";

			return true;
		} else {

			sInfo = sNickname + " have " + std::to_string(m_mVoteKick.find(sNickname)->second) +
				"/" + std::to_string((int)m_mGroupMap.size() - 2) + " votes too kick\n";
			return true;
		}

	};

	bool canInviteUser(std::string & sError,SOCKET sClientSock, std::string sWhoInv) {
		bool bIsInGroup = checkIfClientIsInGroupBy(sClientSock);
		if (bIsInGroup) {
			sError = "This user is already a memeber of " + m_sGroupName + " group";
			return false;
		}

		if (sWhoInv != m_sGLNickname) {
			sError = sWhoInv + " u are not the leader, only leader can inv people.";
			return false;
		}

		return true;
	};

	bool canInviteUser(std::string & sError, std::string sNickname, std::string sWhoInv) {
		bool bIsInGroup = checkIfClientIsInGroupBy(sNickname);
		if (bIsInGroup) {
			sError = sNickname + " is already a memeber of " + m_sGroupName + " group";
			return false;
		}

		if (sWhoInv != m_sGLNickname) {
			sError = sWhoInv + " u are not the leader, only leader can inv people.";
			return false;
		}

		return true;
	};

	bool requestInviteUser(std::string & sError, std::string sWhoInv, std::string sInvited) {
		bool bIsInGroup = checkIfClientIsInGroupBy(sWhoInv);
		if (!bIsInGroup) {
			sError = "You are not a memeber of " + m_sGroupName + " group";
			return false;
		}

		bIsInGroup = checkIfClientIsInGroupBy(sInvited);
		if (bIsInGroup) {
			sError = "This client is already a memeber of " + m_sGroupName + " group";
			return false;
		}
		std::string sPackageMessage = sWhoInv + " propose to invite " + sInvited + " to " + m_sGroupName + " group";
		std::string sPackageRemove = CNetworkDevice::compresPackage(STATE_REQUEST_INVITE_USER_TO_GROUP,
			sPackageMessage, STATE_REQUEST, sInvited);
		CNetworkDevice::sendPackage(sPackageRemove, m_sGLSocket);

		return true;
	};

	bool changeName(std::string & sError, std::string sNewGroupName, std::string sWhoChange) {
		if (sWhoChange != m_sGLNickname) {
			sError = "This operation is available ONLY for leader";
			return false;
		}
		m_sGroupName = sNewGroupName;
		
		std::string sPackageGroupName;
		CNetworkDevice::compresPackage(STATE_CHANGE_GROUP_NAME, "Group name was changed", STATE_SUCCSSED, m_sGroupName);
		mapType::iterator iter = m_mGroupMap.begin();
		iter++; // skip leader
		for (iter; iter != m_mGroupMap.end(); iter++) {
			CNetworkDevice::sendPackage(sPackageGroupName, iter->first);
		}
		return true;
	};

	bool deleteGroupByAdmin() {
		std::string sPackageDelete = "your group was deleted by admin\n(if u do not agree with this decision, pls contact with us)";
		sPackageDelete = CNetworkDevice::compresPackage(STATE_DELETE_GROUP, sPackageDelete, STATE_DELETE_GROUP_DELETED, "");
		mapType::iterator iter;
		for (iter = m_mGroupMap.begin(); iter != m_mGroupMap.end(); iter++) {
			CNetworkDevice::sendPackage(sPackageDelete, iter->first);
		}

		return true;
	};

	std::string getLeader() {
		return m_sGLNickname;
	};

protected:

	//// variable

	// typedef
	typedef std::map<SOCKET, std::string> mapType;
	typedef std::map<std::string, int> mapVoteKick;

	// group name
	std::string m_sGroupName;

	// group-map
	mapType m_mGroupMap;

	// voteKick-map
	mapVoteKick m_mVoteKick;
	
	// group leader
	std::string m_sGLNickname; // nickname
	SOCKET m_sGLSocket; // socket

	// mutex TODO ???
	//mutable std::mutex m_mMtxGroup;

protected:
	
	//// function

	// check if client is in group by SOCKET and receive NICKNAME
	bool checkIfClientIsInGroupByAndRecv(std::string & sNickname, SOCKET sClientSock) {
		if (m_mGroupMap.find(sClientSock) != m_mGroupMap.end()) {
			sNickname = m_mGroupMap.find(sClientSock)->second;
			return true;
		}
		else {
			return false;
		}

	};

	// check if client is in group by NICKNAME and receive SOCKET
	bool checkIfClientIsInGroupByAndRecv(SOCKET & sClientSock, std::string sNickname) {
		mapType::iterator iter;
		for (iter = m_mGroupMap.begin(); iter != m_mGroupMap.end(); iter++) {
			if (iter->second == sNickname) {
				sClientSock = iter->first;
				return true;
			}
		}
		return false;
	};

	// check if client is in group by SOCKET
	bool checkIfClientIsInGroupBy(SOCKET sClientSock) {
		if (m_mGroupMap.find(sClientSock) != m_mGroupMap.end()) {
			return true;
		}
		else {
			return false;
		}

	};

	// check if client is in group by NICKNAME
	bool checkIfClientIsInGroupBy(std::string sNickname) {
		mapType::iterator iter;
		for (iter = m_mGroupMap.begin(); iter != m_mGroupMap.end(); iter++) {
			if (iter->second == sNickname) {
				return true;
			}
		}
		return false;
	};
};


// class with groups
class CVectorGroups {

public:

	//// variable
		/**/

public:

	//// function
	
	// const. / deconst.
	CVectorGroups(void) {
	};
	~CVectorGroups(void) {
	};

	// create new group -> set group name -> set leader info -> add client to group
	bool createGroup(std::string & sError, std::string sGroupName, SOCKET sClientSock, std::string sNickname) {

		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (bGroupExists) {
			sError = sGroupName + " group already exists";
			return false;
		}

		m_mMtxVector.lock();
		m_vGroup.push_back(CGroup(sGroupName, sNickname, sClientSock));
		m_mMtxVector.unlock();

		bool bAdded = addToGroup(sError, sGroupName, sClientSock, sNickname);
		return bAdded;
	};

	// remove group from group-list
	bool deleteGroup(std::string & sError, std::string sGroupName) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (!bGroupExists) {
			sError = sGroupName + "does not exist";
			return false;
		}

		m_mMtxVector.lock();
		m_vGroup.erase(m_vGroup.begin()+iIndex);
		m_mMtxVector.unlock();
		return true;
	};

	// add client to group (if group with that name exist)
	bool addToGroup(std::string & sError, std::string sGroupName, SOCKET sClientSock, std::string sNickname) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (!bGroupExists) {
			sError = sGroupName + " group does not exist";
			return false;
		}
		
		m_mMtxVector.lock();
		bool bAdded = m_vGroup[iIndex].addToGroup(sError, sClientSock, sNickname);
		m_mMtxVector.unlock();

		return bAdded;
	};

	// remove client from group -> by SOCKET (if group with that name exist)
	bool removeFromGroup(std::string & sError, std::string & sInfo, std::string sGroupName, SOCKET sClientSock) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (!bGroupExists) {
			sError = sGroupName + " group does not exist";
			return false;
		}

		m_mMtxVector.lock();
		bool bRemoved = m_vGroup[iIndex].removeFromGroup(sError, sInfo, sClientSock);
		bool bEmpty = m_vGroup[iIndex].isEmpty();

		if (bEmpty) {
			sInfo += m_vGroup[iIndex].getName() + " group was destroyed";
			m_vGroup.erase(m_vGroup.begin() + iIndex);
		}

		m_mMtxVector.unlock();

		return bRemoved;
	};

	// remove client from group -> by Nickname (if group with that name exist)
	bool removeFromGroup(std::string & sError, std::string & sInfo, std::string sGroupName, std::string sNickname) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (!bGroupExists) {
			sError = sGroupName + " group does not exist";
			return false;
		}

		m_mMtxVector.lock();

		bool bRemoved = m_vGroup[iIndex].removeFromGroup(sError, sInfo, sNickname);
		bool bEmpty = m_vGroup[iIndex].isEmpty();
		if (bEmpty) {
			m_vGroup.erase(m_vGroup.begin() + iIndex);
			sInfo += m_vGroup[iIndex].getName() + " group was destroyed";
		}
	
		m_mMtxVector.unlock();

		return bRemoved;
	};

	// remove client from all groups -> by SOCKET
	bool removeFromAllGroups(std::string & sError, std::string & sInfo, SOCKET sClientSock) {
		m_mMtxVector.lock();
		for (std::size_t i = 0; i <m_vGroup.size(); i++) {
			bool bRemoved = m_vGroup[i].removeFromGroup(sError, sInfo, sClientSock);
			if (bRemoved) {
				bool bEmpty = m_vGroup[i].isEmpty();
				if (bEmpty) {
					sInfo += m_vGroup[i].getName() + " group was destroyed";
					m_vGroup.erase(m_vGroup.begin() + i);
				}
			}
		}
		m_mMtxVector.unlock();
		return true;
	};

	// remove client from all groups -> by Nickname
	bool removeFromAllGroups(std::string & sError, std::string & sInfo, std::string sNickname) {
		m_mMtxVector.lock();
		for (std::size_t i = 0; i < m_vGroup.size(); i++) {
			bool bRemoved = m_vGroup[i].removeFromGroup(sError, sInfo, sNickname);
			if (bRemoved) {
				bool bEmpty = m_vGroup[i].isEmpty();
				if (bEmpty) {
					m_vGroup.erase(m_vGroup.begin() + i);
					sInfo += m_vGroup[i].getName() + " group was destroyed";
				}
			}
		}
		m_mMtxVector.unlock();
		return true;
	};
	
	// send message to all clients in group
	bool sendMessageToAll(std::string & sError, std::string sPackage, std::string sGroupName) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (!bGroupExists) {
			sError = sGroupName + " group does not exist";
			return false;
		}
		m_mMtxVector.lock();
		bool bSend = m_vGroup[iIndex].sendMessageToAll(sPackage);
		m_mMtxVector.unlock();
		return bSend;
	};

	// change group leader (if group with that name exist)
	bool changeLeader(std::string & sError, std::string sGLNickname, std::string sGroupName) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (!bGroupExists) {
			sError = sGroupName + " group does not exsits";
			return false;
		}

		m_mMtxVector.lock();
		bool bChange = m_vGroup[iIndex].changeLeader(sError, sGLNickname);
		m_mMtxVector.unlock();
		return bChange;
	};

	// group members list
	bool sendMembersList(std::string sError, std::string sGroupName, SOCKET sClientSock) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (!bGroupExists) {
			sError = sGroupName + " group does not exist";
			return false;
		}

		m_mMtxVector.lock();
		bool bList = m_vGroup[iIndex].sendMembersList(sError, sClientSock); // return false if group is empty
		m_mMtxVector.unlock();
		return bList;
	};

	bool voteKick(std::string & sError, std::string & sInfo, std::string sGroupName, std::string sNickname) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (!bGroupExists) {
			sError = sGroupName + " group does not exist";
			return false;
		}

		m_mMtxVector.lock();
		bool bKick = m_vGroup[iIndex].voteKick(sError, sInfo, sNickname);
		m_mMtxVector.unlock();
		return bKick;
	};

	bool canInivteUser(std::string & sError, std::string sGroupName, std::string sNickname, std::string sWhoInv) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (!bGroupExists) {
			sError = sGroupName + " group does not exist";
			return false;
		}

		m_mMtxVector.lock();
		bool bCanInv = m_vGroup[iIndex].canInviteUser(sError, sNickname, sWhoInv);
		m_mMtxVector.unlock();
		return bCanInv;
	};

	bool canInivteUser(std::string & sError, std::string sGroupName, SOCKET sClientSock, std::string sWhoInv) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (!bGroupExists) {
			sError = sGroupName + " group does not exist";
			return false;
		}

		m_mMtxVector.lock();
		bool bCanInv = m_vGroup[iIndex].canInviteUser(sError, sClientSock, sWhoInv);
		m_mMtxVector.unlock();
		return bCanInv;
	};

	bool requestInviteUser(std::string & sError, std::string sGroupName, std::string sWhoInv, std::string sInvited) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (!bGroupExists) {
			sError = sGroupName + " group does not exist";
			return false;
		}

		m_mMtxVector.lock();
		bool bRequest = m_vGroup[iIndex].requestInviteUser(sError, sWhoInv, sInvited);
		m_mMtxVector.unlock();
		return bRequest;


	};

	bool changeGroupName(std::string & sError, std::string sWhoChange,
		std::string sOldGroupName, std::string sNewGroupName) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sNewGroupName);

		if (bGroupExists) {
			sError = sNewGroupName + ": this group name is in use, try other";
			return false;
		}

		bGroupExists = findGroup(iIndex, sOldGroupName);

		if (!bGroupExists) {
			sError = sOldGroupName + " group does not exist";
			return false;
		}

		m_mMtxVector.lock();
		bool bChange = m_vGroup[iIndex].changeName(sError, sNewGroupName, sWhoChange);
		m_mMtxVector.unlock();
		return bChange;
	};

	bool groupList(std::string & sError, SOCKET sAdminSock) {
		m_mMtxVector.lock();
		if (m_vGroup.empty()) {
			sError = "none of group is existing right now";
			m_mMtxVector.unlock();
			return false;
		}

		std::string sPackageGroupList = "Groups list:\n";
		for (std::size_t i = 0; i < m_vGroup.size(); i++) {
			sPackageGroupList += m_vGroup[i].getName() + " [L]" + m_vGroup[i].getLeader() + "\n";
		}

		sPackageGroupList = CNetworkDevice::compresPackage(STATE_ALL_GROUPS_LIST,
			sPackageGroupList, STATE_ALL_GROUPS_LIST_RECV, "");
		CNetworkDevice::sendPackage(sPackageGroupList, sAdminSock);
		m_mMtxVector.unlock();
		return true;
	};

	bool deleteGroupByAdmin(std::string & sError, std::string sGroupName) {
		int iIndex;
		bool bGroupExists = findGroup(iIndex, sGroupName);

		if (!bGroupExists) {
			sError = sGroupName + " group does not exist";
			return false;
		}

		m_mMtxVector.lock();
		bool bDeleted = m_vGroup[iIndex].deleteGroupByAdmin();
		m_vGroup.erase(m_vGroup.begin() + iIndex);
		m_mMtxVector.unlock();
		return bDeleted;
	};

protected:

	//// variable

	// group-list
	std::vector<CGroup> m_vGroup;

	// mutex to protect m_vGroup
	mutable std::mutex m_mMtxVector;

protected:

	//// function	

	// find group index -> by group name (if group with that name exist)
	bool findGroup(int & iIndex, std::string sGroupName) {
		m_mMtxVector.lock();
		for (std::size_t i = 0; i < m_vGroup.size(); i++) {
			if (sGroupName == m_vGroup[i].getName()) {
				m_mMtxVector.unlock();
				iIndex = i;
				return true;
			}
		}
		m_mMtxVector.unlock();
		return false; // not found
	};

};


// class with user map (login)
class CUserMap {

public:

	//// variable
	/**/

public:

	//// function

	// const. / deconst.
	CUserMap(void) {
	};
	~CUserMap(void) {
	};

	// find client nickname by socket
	bool findClientNnByCs(std::string & sError, std::string & sNickname, SOCKET sClientSock) {
		bool bInGroup = checkIfClientIsInGroupBy(sClientSock);

		if (!bInGroup) {
			sError = "You are not login";
			return false;
		}

		m_mMtxUserMap.lock();
		sNickname = m_mUserMap.find(sClientSock)->second;
		m_mMtxUserMap.unlock();
		return true;
	};

	// find client socket by nickname
	bool findClientCsByNn(std::string & sError, SOCKET & sClientSock, std::string sNickname) {
		sClientSock = SOCKET_ERROR;
		mapType::iterator iter;

		m_mMtxUserMap.lock();
		for (iter = m_mUserMap.begin(); iter != m_mUserMap.end(); iter++) {
			if (iter->second == sNickname) {
				sClientSock = iter->first;
				m_mMtxUserMap.unlock();
				return true;
			}
		}
		m_mMtxUserMap.unlock();
		sError = sNickname + " is not login";
		return false;
	};

	// add client to map
	bool addClient(std::string & sError, SOCKET sClientSock, std::string sNickname) {
		bool bInGroup = checkIfClientIsInGroupBy(sNickname);

		if (bInGroup) {
			sError = sNickname + " is already login";
			return false;
		}

		m_mMtxUserMap.lock();
		m_mUserMap.insert(std::pair<SOCKET, std::string>(sClientSock, sNickname));
		m_mMtxUserMap.unlock();
		return true;
	};

	// remove client from map by socket
	bool removeClient(std::string & sError, SOCKET sClientSock) {
		bool bInGroup = checkIfClientIsInGroupBy(sClientSock);

		if (!bInGroup) {
			sError = "You are not login";
			return false;
		}

		m_mMtxUserMap.lock();
		m_mUserMap.erase(sClientSock); // remove client from map
		m_mMtxUserMap.unlock();
		return true;
	};

	// remove client from map by nickname
	bool removeClient(std::string & sError, std::string sNickname) {
		mapType::iterator iter;
		m_mMtxUserMap.lock();
		for (iter = m_mUserMap.begin(); iter != m_mUserMap.end(); iter++) {
			if (iter->second == sNickname) {
				m_mUserMap.erase(iter->first);
				m_mMtxUserMap.unlock();
				return true;
			}
		}
		m_mMtxUserMap.unlock();
		sError = sNickname + " is not login";
		return false;
	};

	// send message to all login clients (all chat)
	bool sendMessageToAll(std::string sPackage) {
		m_mMtxUserMap.lock();
		mapType::iterator iter;
		for (iter = m_mUserMap.begin(); iter != m_mUserMap.end(); iter++) {
			CNetworkDevice::sendPackage(sPackage, iter->first);
		}
		m_mMtxUserMap.unlock();
		return true;
	};

	// check if client is login by -> SOCKET
	bool isLogin(std::string & sError, SOCKET sClientSock) {
		m_mMtxUserMap.lock();
		if (m_mUserMap.find(sClientSock) == m_mUserMap.end()) {
			sError = "You are not login";
			m_mMtxUserMap.unlock();
			return false;
		}
		m_mMtxUserMap.unlock();
		return true;
	};

	// chcek if client is login by -> Nickname
	bool isLogin(std::string & sError, std::string sNickname) {
		mapType::iterator iter;
		m_mMtxUserMap.lock();
		for (iter = m_mUserMap.begin(); iter != m_mUserMap.end(); iter++) {
			if (iter->second == sNickname) {
				m_mMtxUserMap.unlock();
				return true;
			}
		}
		m_mMtxUserMap.unlock();
		sError = sNickname + " is not login";
		return false;
	};

	bool sendLoginUsersList(std::string & sError, SOCKET sAdminSock) {
		m_mMtxUserMap.lock();

		std::string sPackageList = "Login users:\n";
		mapType::iterator iter;
		for (iter = m_mUserMap.begin(); iter != m_mUserMap.end(); iter++) {
			sPackageList += iter->second + "\n";
		}
		sPackageList = CNetworkDevice::compresPackage(STATE_USERS_LIST, sPackageList, STATE_USERS_LIST_RECV, "");

		CNetworkDevice::sendPackage(sPackageList, sAdminSock);
		m_mMtxUserMap.unlock();
		return true;
	};

protected:

	//// variable

	//// typedef
	typedef std::map<SOCKET, std::string> mapType;

	// map with clients (socket + nickname)
	mapType m_mUserMap;

	// mutex -> secure map
	mutable std::mutex m_mMtxUserMap;

protected:

	//// function

	// check if client is in group by SOCKET
	bool checkIfClientIsInGroupBy(SOCKET sClientSock) {
		m_mMtxUserMap.lock();
		if (m_mUserMap.find(sClientSock) != m_mUserMap.end()) {
			m_mMtxUserMap.unlock();
			return true;
		}
		else {
			m_mMtxUserMap.unlock();
			return false;
		}

	};

	// check if client is in group by NICKNAME
	bool checkIfClientIsInGroupBy(std::string sNickname) {
		m_mMtxUserMap.lock();
		mapType::iterator iter;
		for (iter = m_mUserMap.begin(); iter != m_mUserMap.end(); iter++) {
			if (iter->second == sNickname) {
				m_mMtxUserMap.unlock();
				return true;
			}
		}
		m_mMtxUserMap.unlock();
		return false;
	};

};


// class with admin map (login)
class CAdminMap {

public:

	//// variable
	/**/

public:

	//// function

	// const. / deconst.
	CAdminMap(void) {
	};
	~CAdminMap(void) {
	};

	// find client nickname by socket
	bool findClientNnByCs(std::string & sError, std::string & sNickname, SOCKET sClientSock) {
		bool bInGroup = checkIfClientIsInGroupBy(sClientSock);

		if (!bInGroup) {
			sError = "You are not login";
			return false;
		}

		m_mMtxAdminMap.lock();
		sNickname = m_mAdminMap.find(sClientSock)->second;
		m_mMtxAdminMap.unlock();
		return true;
	};

	// find client socket by nickname
	bool findClientCsByNn(std::string & sError, SOCKET & sClientSock, std::string sNickname) {
		sClientSock = SOCKET_ERROR;
		mapType::iterator iter;

		m_mMtxAdminMap.lock();
		for (iter = m_mAdminMap.begin(); iter != m_mAdminMap.end(); iter++) {
			if (iter->second == sNickname) {
				sClientSock = iter->first;
				m_mMtxAdminMap.unlock();
				return true;
			}
		}
		m_mMtxAdminMap.unlock();
		sError = sNickname + " is not login";
		return false;
	};

	// add client to map
	bool addClient(std::string & sError, SOCKET sClientSock, std::string sNickname) {
		bool bInGroup = checkIfClientIsInGroupBy(sNickname);

		if (bInGroup) {
			sError = sNickname + " is already login";
			return false;
		}

		m_mMtxAdminMap.lock();
		m_mAdminMap.insert(std::pair<SOCKET, std::string>(sClientSock, sNickname));
		m_mMtxAdminMap.unlock();
		return true;
	};

	// remove client from map by socket
	bool removeClient(std::string & sError, SOCKET sClientSock) {
		bool bInGroup = checkIfClientIsInGroupBy(sClientSock);

		if (!bInGroup) {
			sError = "You are not login";
			return false;
		}

		m_mMtxAdminMap.lock();
		m_mAdminMap.erase(sClientSock); // remove client from map
		m_mMtxAdminMap.unlock();
		return true;
	};

	// remove client from map by nickname
	bool removeClient(std::string & sError, std::string sNickname) {
		mapType::iterator iter;
		m_mMtxAdminMap.lock();
		for (iter = m_mAdminMap.begin(); iter != m_mAdminMap.end(); iter++) {
			if (iter->second == sNickname) {
				m_mAdminMap.erase(iter->first);
				m_mMtxAdminMap.unlock();
				return true;
			}
		}
		m_mMtxAdminMap.unlock();
		sError = sNickname + " is not login";
		return false;
	};

	// send message to all login clients (all chat)
	bool sendMessageToAll(std::string sPackage) {
		m_mMtxAdminMap.lock();
		mapType::iterator iter;
		for (iter = m_mAdminMap.begin(); iter != m_mAdminMap.end(); iter++) {
			CNetworkDevice::sendPackage(sPackage, iter->first);
		}
		m_mMtxAdminMap.unlock();
		return true;
	};

	// check if client is login by -> SOCKET
	bool isLogin(std::string & sError, SOCKET sClientSock) {
		m_mMtxAdminMap.lock();
		if (m_mAdminMap.find(sClientSock) == m_mAdminMap.end()) {
			sError = "You are not login";
			m_mMtxAdminMap.unlock();
			return false;
		}
		m_mMtxAdminMap.unlock();
		return true;
	};

	// chcek if client is login by -> Nickname
	bool isLogin(std::string & sError, std::string sNickname) {
		mapType::iterator iter;
		m_mMtxAdminMap.lock();
		for (iter = m_mAdminMap.begin(); iter != m_mAdminMap.end(); iter++) {
			if (iter->second == sNickname) {
				m_mMtxAdminMap.unlock();
				return true;
			}
		}
		m_mMtxAdminMap.unlock();
		sError = sNickname + " is not login";
		return false;
	};

protected:

	//// variable

	//// typedef
	typedef std::map<SOCKET, std::string> mapType;

	// map with clients (socket + nickname)
	mapType m_mAdminMap;

	// mutex -> secure map
	mutable std::mutex m_mMtxAdminMap;

protected:

	//// function

	// check if client is in group by SOCKET
	bool checkIfClientIsInGroupBy(SOCKET sClientSock) {
		m_mMtxAdminMap.lock();
		if (m_mAdminMap.find(sClientSock) != m_mAdminMap.end()) {
			m_mMtxAdminMap.unlock();
			return true;
		}
		else {
			m_mMtxAdminMap.unlock();
			return false;
		}

	};

	// check if client is in group by NICKNAME
	bool checkIfClientIsInGroupBy(std::string sNickname) {
		m_mMtxAdminMap.lock();
		mapType::iterator iter;
		for (iter = m_mAdminMap.begin(); iter != m_mAdminMap.end(); iter++) {
			if (iter->second == sNickname) {
				m_mMtxAdminMap.unlock();
				return true;
			}
		}
		m_mMtxAdminMap.unlock();
		return false;
	};

};


// register list
class CVectorRegister {

public:

	//// variable
		/**/

public:

	//// function

	bool start() {
		m_mMtxVectorRegister.lock();
		m_vRegister.clear();
		std::ifstream ifsFile("register.csv");
		std::string sLine;
		while (std::getline(ifsFile, sLine)){
			std::string sGetLine;
			std::vector<std::string> vGetLine;
			std::stringstream ss(sLine);
			while (std::getline(ss, sGetLine, ',')){
				vGetLine.push_back(sGetLine);
			}
			m_vRegister.emplace_back(vGetLine);
		}
		ifsFile.close();
		m_mMtxVectorRegister.unlock();
		return true;
	};

	bool check(std::string & sError, std::string & sInfo, std::string sLogin, std::string sPassword) {
		m_mMtxVectorRegister.lock();
		for (size_t i = 0; i < m_vRegister.size(); i++) {
			if (m_vRegister[i].at(0) == sLogin) {
				if (m_vRegister[i].at(1) == sPassword) {
					sInfo = m_vRegister[i].at(2);
					m_mMtxVectorRegister.unlock();
					return true;
				} else {
					sError = "invalid login or password";
					m_mMtxVectorRegister.unlock();
					return false; // invalid password
				}
			}
		}
		sError = "invalid login or password";
		m_mMtxVectorRegister.unlock();
		return false; // invalid login
	};

	bool registerUser(std::string & sError, std::string sLogin, std::string sPassword) {
		m_mMtxVectorRegister.lock();

		// create object with login and password
		std::vector<std::string> vLine;
		vLine.push_back(sLogin);
		vLine.push_back(sPassword);
		vLine.push_back("user");

		// add object to m_vRegister;
		m_vRegister.emplace_back(vLine);

		// create string to add line in register.csv
		std::string sLine = sLogin + "," + sPassword + "," + "user";

		// add line in register.csv
		std::ofstream ofsFile(
			"register.csv", std::ios_base::out | std::ios_base::app);
		ofsFile << sLine << std::endl;
		ofsFile.close();
		m_mMtxVectorRegister.unlock();
		return true;
	};

	bool unregisterUser(std::string & sError, std::string sLogin) {
		m_mMtxVectorRegister.lock();
		for (size_t i = 0; i < m_vRegister.size(); i++) {
			if (m_vRegister[i].at(0) == sLogin) {
				//sLine = sLogin + "," + m_vRegister[i].at(1) + "," + sUserType;
				m_vRegister.erase(m_vRegister.begin() + i);
			}
		}

		std::ifstream ifsFileOld("register.csv"); // old register
		std::ofstream ofsFileNew("registerBackup.csv"); // new register
		ofsFileNew.clear();
		std::string sLine;
		while (std::getline(ifsFileOld, sLine)) {
			std::string sGetLine;
			std::stringstream ss(sLine);
			std::getline(ss, sGetLine, ',');
			if (sGetLine != sLogin) {
				ofsFileNew << sGetLine << ",";
				while (std::getline(ss, sGetLine)) {
					ofsFileNew << sGetLine;
				}
				ofsFileNew << std::endl;
			}			
		}

		ifsFileOld.close();
		ofsFileNew.close();
		
		int iRename = rename("register.csv", "registerBackup1.csv");
		if (iRename != 0) {
			sError = "can not change file name1";
			m_mMtxVectorRegister.unlock();
			return false;
		}

		iRename = rename("registerBackup.csv", "register.csv");
		if (iRename != 0) {
			sError = "can not change file name2";
			m_mMtxVectorRegister.unlock();
			return false;
		}

		iRename = rename("registerBackup1.csv", "registerBackup.csv");
		if (iRename != 0) {
			sError = "can not change file name3";
			m_mMtxVectorRegister.unlock();
			return false;
		}

		m_mMtxVectorRegister.unlock();
		return true;
	};

protected:

	//// variable
	
	std::vector<std::vector<std::string> > m_vRegister;

	mutable std::mutex m_mMtxVectorRegister;

protected:

	//// function
		/**/

};


// main class
class CServerNetwork {

public:

	//// variable
		/**/

public:

	//// function

	// const. / decont.
	CServerNetwork(void);
	~CServerNetwork(void);

	// put on ice server
	void start();

protected:

	//// variable

	// typedef
	typedef std::map<SOCKET, std::string> mapType;

	// map lists
	CVectorRegister m_mRegisterMap;
	CVectorGroups m_mGroupMap;
	CUserMap m_mUserMap;
	CAdminMap m_mAdminMap;

	// sockets for server to connect with client
	SOCKET m_sListenSocket;
	SOCKET m_sClientSocket;
	
	// send / recv device
	CNetworkDevice * m_pDevice;

	// arrays for protocol information
	std::string m_sProType[PROTOCOL_LENGTH] = { 
	/*0*/	"Register",
	/*1*/	"Login",
	/*2*/	"Logout",
	/*3*/	"Join group",
	/*4*/	"Abandon group",
	/*5*/	"Send message",
	/*6*/	"Create group",
	/*7*/	"Change leader",
	/*8*/	"Kick user from group",
	/*9*/	"Group members list",
	/*10*/	"Vote kick",
	/*11*/	"Invite to group",
	/*12*/	"Request invite to group",
	/*13*/	"Group name change",
	/*14*/	"Users list (Admin tool)",
	/*15*/	"Groups list (Admin tool)",
	/*16*/	"Delete group (Admin tool)",
	/*17*/	"Kick user from group (Admin tool)",
	/*18*/	"Unregister user (Admin tool)"};

	static const int m_iProParLen = 4;
	std::string m_sProPar[m_iProParLen] = { 
	/*0*/	"Rezervated for system",
	/*1*/	"To all",
	/*2*/	"To group",
	/*3*/	"To selected user" };

	static const int m_iProIdParLen = 4;
	std::string m_sProIdPar[m_iProIdParLen] = { 
	/*0*/	"Rezervated for system",
	/*1*/	"Doesnt matter (its send to all)",
	/*2*/	"Group name",
	/*3*/	"User name" };

	std::string m_sProIdType[PROTOCOL_LENGTH] = { 
	/*0*/	"User name",
	/*1*/	"User name",
	/*2*/	"User name",
	/*3*/	"Group name",
	/*4*/	"Group name",
	/*5*/	"User name / Group name",
	/*6*/	"Group name",
	/*7*/	"User name",
	/*8*/	"User name",
	/*9*/	"Group name",
	/*10*/	"User name",
	/*11*/	"User name",
	/*12*/	"User name",
	/*13*/	"Group name",
	/*14*/	"...",
	/*15*/	"...",
	/*16*/	"Group name",
	/*17*/	"User name",
	/*18*/	"User name"};

	// PROTOCOL
	// -> Protocol.h
	// END PROTOCOL

protected:

	//// function

	// main tools
	// put on ice recv
	static DWORD recvLoop(void * pData);
	// server brain
	void brain(CVectorRegister & mRegisterMap, CVectorGroups & mGroupMap,CUserMap & mUserMap,
		CAdminMap & mAdminMap, SOCKET ClientSocket, std::string sPackage);


	// send tools
	bool sendToClient(std::string & sError, CUserMap & mUserMap, std::string sMessage,
		std::string sSendTo, std::string sSendingNick);
	bool sendToAll(std::string & sError, CUserMap & mUserMap, 
		std::string sMessage, std::string sWhoSend);
	bool sendToGroup(std::string & sError, CVectorGroups & mGroupMap, std::string sMessage,
		std::string sWhoSend, std::string sGroupName);


	// group tools
	bool createGroup(std::string & sError, CVectorGroups & mGroupMap, std::string sGroupName, SOCKET sClientSock,
		std::string sNickname);
	bool joinGroup(std::string & sError, CVectorGroups & mGroupMap, std::string sGroupName, SOCKET sClientSock,
		std::string sNickname);
	bool abandonGroup(std::string & sError, std::string & sInfo, 
		CVectorGroups & mGroupMap, std::string sGroupName, SOCKET sClientSocket);
	bool abandonAllGroups(std::string & sError, std::string & sInfo, 
		CVectorGroups & mGroupMap, SOCKET sClientSocket);
	bool changeLeader(std::string & sError, CVectorGroups & mGroupMap, std::string sGroupName,
		std::string sGLName);
	bool kickFromGroup(std::string & sError, std::string & sInfo, 
		CVectorGroups & mGroupMap, std::string sGroupName, SOCKET sRemoveClient);
	bool sendGroupMembersList(std::string & sError, CVectorGroups & mGroupMap, SOCKET sClientSock,
		std::string sGroupName);
	bool voteKick(std::string & sError, std::string & sInfo, CVectorGroups & mGroupMap,
		std::string sNickname, std::string sGroupName);
	bool inviteToGroup(std::string & sError, CVectorGroups & mGroupMap,
		std::string sGroupName, SOCKET sClientSock, std::string sWhoInv);
	bool requestInviteUser(std::string & sError, CVectorGroups & mGroupMap,
		std::string sGroupName, std::string sWhoInv, std::string sInvited);
	bool changeGroupName(std::string & sError, CVectorGroups & mGroupMap,
		std::string sWhoChange, std::string sOldGroupName, std::string sNewGroupName);


	// admin tools
	bool loginUsersList(std::string & sError, CUserMap & mUserMap, SOCKET sAdminSock);
	bool allGroupsList(std::string & sError, CVectorGroups & mGroupMap, SOCKET sAdminSock);
	bool deleteGroupByAdmin(std::string & sError, CVectorGroups & mGroupMap, std::string sGroupName);
	bool kickFromGroupByAdmin(std::string & sError, std::string & sInfo, CVectorGroups & mGroupMap,
		std::string sGroupName, SOCKET sRemoveClientSock);
	bool unregisterUser(std::string & sError, CVectorRegister & mRegisterMap, std::string sLogin);


	// login / logout tools
	bool registerUser(std::string & sError, CVectorRegister & mRegisterMap, std::string sLogin, std::string sPassword);
	// device login client
	bool login(std::string & sError, std::string & sLoginAs, CVectorRegister & mRegisterMap,
		CUserMap & mUserMap, CAdminMap & mAdminMap, SOCKET sClientSock, std::string sNickname, std::string sPassword);
	// device login admin
	bool loginAdmin(std::string & sError, CAdminMap & mAdminMap, SOCKET sClientSock, std::string sNickname);
	// device logout client
	bool logout(std::string & sError, CUserMap & mUserMap, CAdminMap & mAdminMap, SOCKET sClientSock);
	// print log-info about disconnected client and remove them from map
	void disconnected(CVectorGroups & mGroupMap, CUserMap & mUserMap, CAdminMap & mAdminMap, SOCKET sClientSock);


	// log tools
	// print log-info to .txt
	void writeTextToLogFile(const std::string &text);
	// update system time (need for log-ingo)
	void updateTime(std::string & sCurrentTime);
};