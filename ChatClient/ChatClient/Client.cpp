#include "Client.h"
// basic construction
CClient::CClient(void) {
	// set new network for client
	m_pNetwork = new CClientNetwork();

	// set default -> user nickname
	m_sNickname = "NoName";

	// set default -> user group name
	m_sGroupName = "none";

	// set default -> user status
	m_iLogin = STATE_LOGGED_OUT;
	m_iGroup = STATE_OUT_OF_GROUP;

	// set default -> graf variables 
	m_bSendMessage = STATE_OPERATION_DONT_SEND_MESSAGE;
	m_iType = STATE_OPERATION_NONE;
	m_sTypeName = "";

	// hide / show consol input
	m_hHideShowInput = GetStdHandle(STD_INPUT_HANDLE);
	m_mHideShowInput = 0;

	printMenu();
}

// construction with nick name
CClient::CClient(std::string sNickname, std::string sPassword) {
	// set new network for client
	m_pNetwork = new CClientNetwork();

	// set default -> user nickname
	m_sNickname = "";

	// set default -> user group name
	m_sGroupName = "none";

	// set default -> user status
	m_iLogin = STATE_LOGGED_OUT;
	m_iGroup = STATE_OUT_OF_GROUP;

	// set default -> graf variables 
	m_bSendMessage = STATE_OPERATION_DONT_SEND_MESSAGE;
	m_iType = STATE_OPERATION_NONE;
	m_sTypeName = "";

	// hide / show consol input
	m_hHideShowInput = GetStdHandle(STD_INPUT_HANDLE);
	m_mHideShowInput = 0;

	// send nickname to the server
	login(sNickname, sPassword);
	printMenu();

}
CClient::~CClient(void) {
	// deconst.
}


// login and logout devices
bool CClient::login(std::string sNickname, std::string sPassword) {
	return sendPackage(sNickname, STATE_DEFAULT_PAR, sPassword, STATE_LOGIN);
}
bool CClient::logout() {
	return sendPackage("", STATE_DEFAULT_PAR, m_sNickname, STATE_LOGOUT);
}
bool CClient::registerUser(std::string sLogin, std::string sPassword) {
	std::string sPackage;
	return sendPackage(sLogin, STATE_REGISTER, sPassword, STATE_DEFAULT_PAR);
}


// group devices
bool CClient::joinGroup(std::string sGroupName) {
	return sendPackage("", STATE_DEFAULT_PAR, sGroupName, STATE_JOIN_GROUP);
}
bool CClient::abandonGroup() {
	return sendPackage("", STATE_DEFAULT_PAR, m_sGroupName, STATE_ABANDON_GROUP);
}
bool CClient::createGroup(std::string sGroupName) {
	return sendPackage("", STATE_DEFAULT_PAR, sGroupName, STATE_CREATE_GROUP);
}
bool CClient::changeLeader(std::string sNickname) {
	return sendPackage(sNickname, STATE_DEFAULT_PAR, m_sGroupName, STATE_CHANGE_LEADER);
}
bool CClient::removeFromGroup(std::string sNickname) {
	return sendPackage(sNickname, STATE_DEFAULT_PAR, m_sGroupName, STATE_KICK_USER_FROM_GROUP);
}
bool CClient::groupMembersList() {
	return sendPackage("", STATE_DEFAULT_PAR, m_sGroupName, STATE_GROUP_MEMBERS_LIST);
}
bool CClient::voteKick(std::string sNickname) {
	return sendPackage(sNickname, STATE_DEFAULT_PAR, m_sGroupName, STATE_VOTE_KICK);
}
bool CClient::inviteUser(std::string sNickname) {
	return sendPackage(sNickname, STATE_INVITE, m_sGroupName, STATE_INVITE_USER_TO_GROUP);
}
bool CClient::requestInviteUserToGroup(std::string sNickname) {
	return sendPackage(sNickname, STATE_DEFAULT_PAR, m_sGroupName, STATE_REQUEST_INVITE_USER_TO_GROUP);
}
bool CClient::changeGroupName(std::string sNewGroupName) {
	return sendPackage(sNewGroupName, STATE_DEFAULT_PAR, m_sGroupName, STATE_CHANGE_GROUP_NAME);
}


// admin devices
bool CClient::loginUsersList() {
	bool bUsersList = sendPackage("", STATE_DEFAULT_PAR, "", STATE_USERS_LIST);
	return bUsersList;
}
bool CClient::allGroupsList() {
	bool bGroupList = sendPackage("", STATE_DEFAULT_PAR, "", STATE_ALL_GROUPS_LIST);
	return bGroupList;
}
bool CClient::deleteGroup(std::string sGroupName) {
	bool bSendDelete = sendPackage("", STATE_DEFAULT_PAR, sGroupName, STATE_DELETE_GROUP);
	return bSendDelete;
}
bool CClient::removeFromGroupByAdmin(std::string sGroupName, std::string sNickname) {
	return sendPackage(sNickname, STATE_DEFAULT_PAR, sGroupName, STATE_KICK_USER_FROM_GROUP_BY_ADMIN);
}
bool CClient::unregisterUser(std::string sLogin) {
	return sendPackage("", STATE_DEFAULT_PAR, sLogin, STATE_UNREGISTER_USER);
}


// send message devices
bool CClient::sendMessageToAll(std::string sMessage) { // to all
	return sendMessage(sMessage, STATE_TO_ALL, ""); // (message, to all, doesnt matter 0)
}
bool CClient::sendMessageToGroup(std::string sMessage) { // to group
	return sendMessage(sMessage, STATE_TO_GROUP, m_sGroupName); // (message, to group, group id)
}
bool CClient::sendMessageToUser(std::string sMessage, std::string sNickName) { // to other client
	return sendMessage(sMessage, STATE_TO_USER, sNickName); // (message, to user, user id)
}


// send templates
bool CClient::sendMessage(const std::string & sMessage, int iParameter, std::string sID) { // int type = 5;
	std::string sPackage = CNetworkDevice::compresPackage(5, sMessage, iParameter, sID);
	return m_pNetwork->sendPackage(sPackage);
}
bool CClient::sendPackage(const std::string & sMessage, int iParameter, std::string sID, int iType) {
	std::string sPackage = CNetworkDevice::compresPackage(iType, sMessage, iParameter, sID);
	return m_pNetwork->sendPackage(sPackage);
}


// receive package blocking / non blocking
bool CClient::receivePackageBlocking(std::string & sPackage) {
	bool bReceive = m_pNetwork->receivePackage(sPackage, true);
	return bReceive;
}
bool CClient::receivePackageNonBlocking(std::string & sPackage) {
	bool bReceive = m_pNetwork->receivePackage(sPackage, false);
	return bReceive;
}


// check functions
// take chars from user keyboard (after -> Enter)
void CClient::loopKeyboard() {

	std::string sVar;

	std::getline(std::cin, sVar);

	m_vKeyboard.add(sVar);

}
// keep one's ear to the ground 
void CClient::recvQuestion() { 
	std::string sPackage;
	bool bRecv = receivePackageNonBlocking(sPackage);

	if (bRecv) {
		recvBrain(sPackage);
	}
}
// if client typed somthing -> send message to server // else -> do nothing
void CClient::sendQuestion() {
	bool bEmpty = m_vKeyboard.isEmpty();

	if (!bEmpty) {
		sendBrain(m_vKeyboard.getFront());
		m_vKeyboard.remove();
	}
}


void CClient::groupMenu() {
	switch (m_iGroup) {
	case STATE_OUT_OF_GROUP:
		printf("2->join group\n");
		printf("3->create group\n");
		break;
	case STATE_IN_GROUP:
		printf("2->abandon group\n");
		printf("4->vote kick\n");
		printf("6->%s members\n", m_sGroupName.c_str());
		printf("7->Request invite user to group\n");
		break;
	case STATE_IN_GROUP_LEADER:
		printf("2->abandon group\n");
		printf("3->change leader\n");
		printf("4->remove user from group\n");
		printf("6->%s members\n", m_sGroupName.c_str());
		printf("7->Invite user to group\n");
		printf("8-> change group name\n");
		break;
	}
}


// print menu on screan
void CClient::printMenu() {

	printf("0->shut down\n");

	switch (m_iLogin) {
	case STATE_LOGIN_AS_USER: case STATE_LOGIN_AS_ADMIN:
		printf("[%s] Group: %s\n", m_sNickname.c_str(), m_sGroupName.c_str());
		printf("5->sendMessage\n");
		printf("1->logout\n");
		groupMenu();
		if (m_iLogin == STATE_LOGIN_AS_ADMIN) {
			printf("--- ADMIN PANEL ---\n");
			printf("10->users list\n");
			printf("11->groups list\n");
			printf("12->delete group\n");
			printf("13->remove user from selected group\n");
			printf("14->unregister user\n");
			printf("--- ///// ///// ---\n");
		}
		break;
	case STATE_LOGGED_OUT:
		printf("1->login\n");
		printf("2->register\n");
		break;
	}

	// OLD MENU
	/*
	if (m_bAuthorization == STATE_AUTHORIZED) {

	printf("[%s] Group: %s\n", m_sNickname.c_str(), m_sGroupName.c_str());

	if (m_iLogin == STATE_LOGGED_OUT) {
	printf("1->login\n");
	}
	else if (m_iLogin == STATE_LOGIN_AS_USER || m_iLogin == STATE_LOGIN_AS_ADMIN) {
	printf("1->logout\n");
	}

	if (m_iGroup == STATE_OUT_OF_GROUP) {
	printf("2->join group\n");
	printf("3->create group\n");
	} else if (m_iGroup == STATE_IN_GROUP) {
	printf("2->abandon group\n");
	printf("6->%s members\n", m_sGroupName.c_str());
	} else if (m_iGroup == STATE_IN_GROUP_LEADER) {
	printf("2->abandon group\n");
	printf("3->change leader\n");
	printf("4->remove user from group\n");
	printf("6->%s members\n", m_sGroupName.c_str());
	}

	printf("5->sendMessage\n");

	printf("0->shut down\n");
	} else {
	printf("Type ur nickname: ");
	}*/
}


// main functions
// take char / chars from vector -> do somthing with it
void CClient::sendBrain(std::string sVar) {

	switch (m_iLogin) {
	 case STATE_LOGGED_OUT:
		 switch (m_iType) {
		  case STATE_OPERATION_NONE:
			  switch (std::stoi(sVar)) {
			  case 0: // input 0
				  exit(1);
				  break;
			  case 1: // input 1
			  {
				  printf("login: ");
				  m_iType = STATE_OPERATION_LOGIN;
				  // hide (password) input  
				  GetConsoleMode(m_hHideShowInput, &m_mHideShowInput);
				  SetConsoleMode(m_hHideShowInput, m_mHideShowInput & (~ENABLE_ECHO_INPUT));
				  // end hide (password) input
				  break;
			  }
			  case 2: // input 2
				  printf("login: ");
				  m_iType = STATE_OPERATION_REGISTER;
				  // hide (password) input  
				  GetConsoleMode(m_hHideShowInput, &m_mHideShowInput);
				  SetConsoleMode(m_hHideShowInput, m_mHideShowInput & (~ENABLE_ECHO_INPUT));
				  // end hide (password) input
				  break;
			  }
			  break;
		  case STATE_OPERATION_LOGIN:
		  {
			  m_sTypeName = sVar;
			  printf("password: ");
			  m_iType = STATE_OPERATION_LOGIN_PASSWORD;
			  // show (password) input
			  SetConsoleMode(m_hHideShowInput, m_mHideShowInput);
			  // end show (password) input
			  break;
		  }
		  case STATE_OPERATION_LOGIN_PASSWORD:
		  {
			  printf("\n");
			  login(m_sTypeName, sVar);
			  m_iType = STATE_OPERATION_NONE;
			  break;
		  }
		  case STATE_OPERATION_REGISTER:
			  m_sNickname = sVar;
			  printf("password: ");
			  m_iType = STATE_OPERATION_REGISTER_PASSWORD;
			  break;
		  case STATE_OPERATION_REGISTER_PASSWORD:
			  m_sTypeName = sVar;
			  printf("\nvalidate password: ");
			  m_iType = STATE_OPERATION_REGISTER_PASSWORD_VALIDATE;
			  // show (password) input
			  SetConsoleMode(m_hHideShowInput, m_mHideShowInput);
			  // end show (password) input
			  break;
		  case STATE_OPERATION_REGISTER_PASSWORD_VALIDATE:
			  if (m_sTypeName != sVar) {
				  m_iType = STATE_OPERATION_NONE;
				  m_sNickname = "none";
				  m_sTypeName = "";
				  printf("\nRegister failed (incorrect password)\n");
				  printMenu();
				  break;
			  }
			  printf("\n");
			  registerUser(m_sNickname, m_sTypeName);
			  m_iType = STATE_OPERATION_NONE;
			  m_sNickname = "none";
			  m_sTypeName = "";
			  break;
		 }
		 break;
	 case STATE_LOGIN_AS_USER: case STATE_LOGIN_AS_ADMIN:
		 switch (m_iType) {
		 case STATE_OPERATION_NONE:
			 switch (m_bSendMessage) {
			 case STATE_OPERATION_DONT_SEND_MESSAGE:
				 switch (std::stoi(sVar)) {
				 case 0: // input 0
					 exit(1);
					 break;
				 case 1: // input 1
					 logout(); // logout
					 break;
				 case 2: // inptu 2
					 if (m_iGroup == STATE_OUT_OF_GROUP) {
						 printf("group name: ");
						 m_iType = STATE_OPERATION_JOING_GROUP; // join to roup
					 }
					 else if (m_iGroup == STATE_IN_GROUP || m_iGroup == STATE_IN_GROUP_LEADER) {
						 abandonGroup(); // abandon group
					 }
					 break;
				 case 3: // inptu 3
					 if (m_iGroup == STATE_OUT_OF_GROUP) {
						 printf("group name: ");
						 m_iType = STATE_OPERATION_CREATE_GROUP; // create group
					 }
					 else if (m_iGroup == STATE_IN_GROUP_LEADER) {
						 printf("user name: ");
						 m_iType = STATE_OPERATION_CHANGE_LEADER; // change leader (only for leader)
					 }
					 break;
				 case 4: // input 4
					 if (m_iGroup == STATE_IN_GROUP) {
						 printf("user name: ");
						 m_iType = STATE_OPERATION_VOTE_KICK; // vote kick (for users)
					 }
					 else if (m_iGroup == STATE_IN_GROUP_LEADER) {
						 printf("user name: ");
						 m_iType = STATE_OPERATION_KICK_USER_FROM_GROUP; // remove from group (only for leader)
					 }
					 break;
				 case 5: // input 5
					 m_bSendMessage = STATE_OPERATION_SEND_MESSAGE; // send message
					 if (m_iGroup == STATE_OUT_OF_GROUP) { // for users without group
						 printf("1->to all\n2->to user\n0->go to main menu\n");
					 }
					 else if (m_iGroup == STATE_IN_GROUP || m_iGroup == STATE_IN_GROUP_LEADER) { // for group members
						 printf("1->to all\n2->to user\n3->to group\n0->go to main menu\n");
					 }
					 break;
				 case 6: // input 6
					 if (m_iGroup == STATE_IN_GROUP || m_iGroup == STATE_IN_GROUP_LEADER) {
						 groupMembersList();
					 }
					 break;
				 case 7: // input 7
					 if (m_iGroup == STATE_IN_GROUP_LEADER) {
						 printf("user name: ");
						 m_iType = STATE_OPERATION_INVITE_USER_TO_GROUP;
					 }
					 else if (m_iGroup = STATE_IN_GROUP) {
						 printf("user name: ");
						 m_iType = STATE_OPERATION_REQUEST_INVITE_USER_TO_GROUP;
					 }
					 break;
				 case 8: // input 8
					 if (m_iGroup == STATE_IN_GROUP_LEADER) {
						 printf("new group name: ");
						 m_iType = STATE_OPERATION_CHANGE_GROUP_NAME;
					 }
					 break;

					 // ADMIN TOOLS
					 if (m_iLogin == STATE_LOGIN_AS_ADMIN) {
				 case 9: // input 9
					 // TODO STARE LOGOWANIE ADMINISTRACJI, DO WYWALENIE
					 break;
				 case 10: // input 10
					 loginUsersList();
					 break;
				 case 11: // input 11
					 allGroupsList();
					 break;
				 case 12: // input 12
					 printf("group name: ");
					 m_iType = STATE_OPERATION_DELETE_GROUP;
					 break;
				 case 13: // input 13
					 printf("group name: ");
					 m_iType = STATE_OPERATION_KICK_USER_FROM_GROUP_BY_ADMIN_TYPE_GROUP_NAME;
					 break;
				 case 14: // input 14
					 printf("user name: ");
					 m_iType = STATE_OPERATION_UNREGISTER_USER;
					 break;
					 }
					 // END ADMIN TOOLS

				 default:
					 printf("wrong action\n");
					 break;
					 //continue;
				 }
				 break;
			 case STATE_OPERATION_SEND_MESSAGE:
				 switch (std::stoi(sVar)) {
				 case 0: // input 0
					 m_bSendMessage = STATE_OPERATION_DONT_SEND_MESSAGE;
					 printMenu();
					 break;
				 case 1: // input 1
					 printf("message: ");
					 m_iType = STATE_OPERATION_SEND_MESSAGE_TO_ALL;
					 break;
				 case 2: // input 2
					 printf("user name: ");
					 m_iType = STATE_OPERATION_TYPE_MESSAGE_TO_USER;
					 break;
				 case 3: // input 3
					 if (m_iGroup == STATE_IN_GROUP || m_iGroup == STATE_IN_GROUP_LEADER) {
						 printf("message: ");
						 m_iType = STATE_OPERATION_SEND_MESSAGE_TO_GROUP;
					 }
					 break;
				 default:
					 printf("wrong action\n");
					 break;
					 //continue;
				 }
				 break;
			 }
			 break;
		 case STATE_OPERATION_SEND_MESSAGE_TO_ALL:
			 sendMessageToAll(sVar);
			 m_bSendMessage = STATE_OPERATION_DONT_SEND_MESSAGE;
			 m_iType = STATE_OPERATION_NONE;
			 printMenu();
			 break;
		 case STATE_OPERATION_TYPE_MESSAGE_TO_USER:
			 m_sTypeName = sVar;
			 printf("message: ");
			 m_iType = STATE_OPERATION_SEND_MESSAGE_TO_USER;
			 break;
		 case STATE_OPERATION_SEND_MESSAGE_TO_USER:
			 sendMessageToUser(sVar, m_sTypeName);
			 m_sTypeName = "";
			 m_bSendMessage = STATE_OPERATION_DONT_SEND_MESSAGE;
			 m_iType = STATE_OPERATION_NONE;
			 printMenu();
			 break;
		 case STATE_OPERATION_SEND_MESSAGE_TO_GROUP:
			 sendMessageToGroup(sVar);
			 m_bSendMessage = STATE_OPERATION_DONT_SEND_MESSAGE;
			 m_iType = STATE_OPERATION_NONE;
			 printMenu();
			 break;
		 case STATE_OPERATION_JOING_GROUP:
			 joinGroup(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 break;
		 case STATE_OPERATION_CREATE_GROUP:
			 createGroup(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 break;
		 case STATE_OPERATION_CHANGE_LEADER:
			 changeLeader(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 break;
		 case STATE_OPERATION_KICK_USER_FROM_GROUP:
			 removeFromGroup(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 break;
		 case STATE_OPERATION_VOTE_KICK:
			 voteKick(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 break;
		 case STATE_OPERATION_INVITE_USER_TO_GROUP:
			 inviteUser(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 break;
		 case STATE_OPERATION_INVITE_USER_TO_GROUP_ACCEPT:
			 switch (std::stoi(sVar)) {
			 case STATE_INVITE_ACCEPT:
				 joinGroup(m_sGroupName);
				 m_iType = STATE_OPERATION_NONE;
				 break;
			 case STATE_INVITE_REFUSE:
				 m_sGroupName = "none";
				 m_iType = STATE_OPERATION_NONE;
				 printMenu();
				 break;
			 }
			 break;
		 case STATE_OPERATION_REQUEST_INVITE_USER_TO_GROUP:
			 requestInviteUserToGroup(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 break;
		 case STATE_OPERATION_REQUEST_INVITE_USER_TO_GROUP_ACCEPT:
			 switch (std::stoi(sVar)) {
			 case STATE_REQUEST_ACCEPT:
				 inviteUser(m_sNicknameInvUser);
				 m_iType = STATE_OPERATION_NONE;
				 break;
			 case STATE_REQUEST_REFUSE:
				 m_iType = STATE_OPERATION_NONE;
				 printMenu();
				 break;
			 }
			 break;
		 case STATE_OPERATION_CHANGE_GROUP_NAME:
			 changeGroupName(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 break;

			// ADMIN TOOLS
			if (m_iLogin == STATE_LOGIN_AS_ADMIN) {
			 case STATE_OPERATION_DELETE_GROUP:
				 deleteGroup(sVar);
				 m_iType = STATE_OPERATION_NONE;
				 break;
			 case STATE_OPERATION_KICK_USER_FROM_GROUP_BY_ADMIN_TYPE_GROUP_NAME:
				 m_sTypeName = sVar;
				 m_iType = STATE_OPERATION_KICK_USER_FROM_GROUP_BY_ADMIN_TYPE_USER_NICKNAME;
				 printf("user name: ");
				 break;
			 case STATE_OPERATION_KICK_USER_FROM_GROUP_BY_ADMIN_TYPE_USER_NICKNAME:
				 removeFromGroupByAdmin(m_sTypeName, sVar);
				 m_iType = STATE_OPERATION_NONE;
				 break;
			 case STATE_OPERATION_UNREGISTER_USER:
				 unregisterUser(sVar);
				 m_iType = STATE_OPERATION_NONE;
				 break;
			}
			// END ADMIN TOOLS
		 }
		 break;
	}
	// SECOUND OLD MENU
	/*
	switch (m_iLogin) {
	 case STATE_AUTHORIZED:
		switch (m_iType) {
		 case STATE_OPERATION_NONE:
			 switch (m_bSendMessage) {
				case STATE_OPERATION_DONT_SEND_MESSAGE:
					switch (std::stoi(sVar)) {
					 case 0: // input 0
						exit(1);
						return;
					 case 1: // input 1
						 switch (m_iLogin) {
						  case STATE_LOGGED_OUT:
							  //login(STATE_LOGIN_AS_USER); // login TODO
							  return;
						  case STATE_LOGIN_AS_USER:
							  logout(); // logout
							  return;	  
						  case STATE_LOGIN_AS_ADMIN:
							  logout(); // logout
							  return;
						 }
					 case 2: // inptu 2
						 if (m_iGroup == STATE_OUT_OF_GROUP) {
							 printf("group name: ");
							 m_iType = STATE_OPERATION_JOING_GROUP; // join to roup
							 return;
						 }
						 else if (m_iGroup == STATE_IN_GROUP || m_iGroup == STATE_IN_GROUP_LEADER) {
							 abandonGroup(); // abandon group
							 return;
						 }
					 case 3: // inptu 3
						 if (m_iGroup == STATE_OUT_OF_GROUP) {
							 printf("group name: ");
							 m_iType = STATE_OPERATION_CREATE_GROUP; // create group
							 return;
						 }
						 else if (m_iGroup == STATE_IN_GROUP_LEADER) {
							 printf("user name: ");
							 m_iType = STATE_OPERATION_CHANGE_LEADER; // change leader (only for leader)
							 return;
						 }
					 case 4: // input 4
						if (m_iGroup == STATE_IN_GROUP) {
							printf("user name: ");
							m_iType = STATE_OPERATION_VOTE_KICK; // vote kick (for users)
							return;
						} else if (m_iGroup == STATE_IN_GROUP_LEADER) {
							printf("user name: ");
							m_iType = STATE_OPERATION_KICK_USER_FROM_GROUP; // remove from group (only for leader)
							return;
						}
						 return;
					 case 5: // input 5
						 m_bSendMessage = STATE_OPERATION_SEND_MESSAGE; // send message
						 if (m_iGroup == STATE_OUT_OF_GROUP) { // for users without group
							 printf("1->to all\n2->to user\n0->go to main menu\n");
							 return;
						 }
						 else if (m_iGroup == STATE_IN_GROUP || m_iGroup == STATE_IN_GROUP_LEADER) { // for group members
							 printf("1->to all\n2->to user\n3->to group\n0->go to main menu\n");
							 return;
						 }
					 case 6: // input 6
						 if (m_iGroup == STATE_IN_GROUP || m_iGroup == STATE_IN_GROUP_LEADER) {
							 groupMembersList();
							 return;
						 }
						 return;
					 case 7: // input 7
						 if (m_iGroup == STATE_IN_GROUP_LEADER) {
							 printf("user name: ");
							 m_iType = STATE_OPERATION_INVITE_USER_TO_GROUP;
						 } else if (m_iGroup = STATE_IN_GROUP) {
							 printf("user name: ");
							 m_iType = STATE_OPERATION_REQUEST_INVITE_USER_TO_GROUP;
						 }
						 return;
					 case 8: // input 8
						 if (m_iGroup == STATE_IN_GROUP_LEADER) {
							 printf("new group name: ");
							 m_iType = STATE_OPERATION_CHANGE_GROUP_NAME;
						 }
						 return;
					 case 9: // input 9
						 if (m_iGroup == STATE_LOGGED_OUT) {
							 // login(STATE_LOGIN_AS_ADMIN); // TODO
						 }
						 return;
					 case 10: // input 10
						 if (m_iLogin == STATE_LOGIN_AS_ADMIN) {
							 loginUsersList();
						 }
						 return;
					 case 11: // input 11
						 if (m_iLogin == STATE_LOGIN_AS_ADMIN) {
							 allGroupsList();
						 }
						 return;
					 case 12: // input 12
						 if (m_iLogin == STATE_LOGIN_AS_ADMIN) {
							 printf("group name: ");
							 m_iType = STATE_OPERATION_DELETE_GROUP;
						 }
						 return;
					 case 13: // input 13
						 if (m_iLogin == STATE_LOGIN_AS_ADMIN) {
							 printf("group name: ");
							 m_iType = STATE_OPERATION_KICK_USER_FROM_GROUP_BY_ADMIN_TYPE_GROUP_NAME;
						 }
					 default:
						 return;
					}
					break;
				case STATE_OPERATION_SEND_MESSAGE:
					switch (std::stoi(sVar)) {
					 case 0: // input 0
						 m_bSendMessage = STATE_OPERATION_DONT_SEND_MESSAGE;
						 printMenu();
						 return;
					 case 1: // input 1
						 printf("message: ");
						 m_iType = STATE_OPERATION_SEND_MESSAGE_TO_ALL;
						 return;
					 case 2: // input 2
						 printf("user name: ");
						 m_iType = STATE_OPERATION_TYPE_MESSAGE_TO_USER;
						 return;
					 case 3: // input 3
						 if (m_iGroup == STATE_IN_GROUP || m_iGroup == STATE_IN_GROUP_LEADER) {
							 printf("message: ");
							 m_iType = STATE_OPERATION_SEND_MESSAGE_TO_GROUP;
						 }
						 return;
					 default:
						 return;
					}
					return;
			 }
			 return;
		 case STATE_OPERATION_SEND_MESSAGE_TO_ALL:
			 sendMessageToAll(sVar);
			 m_bSendMessage = STATE_OPERATION_DONT_SEND_MESSAGE;
			 m_iType = STATE_OPERATION_NONE;
			 printMenu();
			 return;
		 case STATE_OPERATION_TYPE_MESSAGE_TO_USER:
			 m_sTypeName = sVar;
			 printf("message: ");
			 m_iType = STATE_OPERATION_SEND_MESSAGE_TO_USER;
			 return;
		 case STATE_OPERATION_SEND_MESSAGE_TO_USER:
			 sendMessageToUser(sVar, m_sTypeName);
			 m_sTypeName = "";
			 m_bSendMessage = STATE_OPERATION_DONT_SEND_MESSAGE;
			 m_iType = STATE_OPERATION_NONE;
			 printMenu();
			 return;
		 case STATE_OPERATION_SEND_MESSAGE_TO_GROUP:
			 sendMessageToGroup(sVar);
			 m_bSendMessage = STATE_OPERATION_DONT_SEND_MESSAGE;
			 m_iType = STATE_OPERATION_NONE;
			 printMenu();
			 return;
		 case STATE_OPERATION_JOING_GROUP:
			 joinGroup(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 return;
		 case STATE_OPERATION_CREATE_GROUP:
			 createGroup(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 return;
		 case STATE_OPERATION_CHANGE_LEADER:
			 changeLeader(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 return;
		 case STATE_OPERATION_KICK_USER_FROM_GROUP:
			 removeFromGroup(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 return;
		 case STATE_OPERATION_VOTE_KICK:
			 voteKick(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 return;
		 case STATE_OPERATION_INVITE_USER_TO_GROUP:
			 inviteUser(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 return;
		 case STATE_OPERATION_INVITE_USER_TO_GROUP_ACCEPT:
			 switch (std::stoi(sVar)) {
			 case STATE_INVITE_ACCEPT:
				 joinGroup(m_sGroupName);
				 m_iType = STATE_OPERATION_NONE;
				 break;
			 case STATE_INVITE_REFUSE:
				 m_sGroupName = "none";
				 m_iType = STATE_OPERATION_NONE;
				 printMenu();
				 break;
			 }
			 return;
		 case STATE_OPERATION_REQUEST_INVITE_USER_TO_GROUP:
			 requestInviteUserToGroup(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 return;
		 case STATE_OPERATION_REQUEST_INVITE_USER_TO_GROUP_ACCEPT:
			 switch (std::stoi(sVar)) {
			 case STATE_REQUEST_ACCEPT:
				 inviteUser(m_sNicknameInvUser);
				 m_iType = STATE_OPERATION_NONE;
				 break;
			 case STATE_REQUEST_REFUSE:
				 m_iType = STATE_OPERATION_NONE;
				 printMenu();
				 break;
			 }
			 return;	 
		 case STATE_OPERATION_CHANGE_GROUP_NAME:
			 changeGroupName(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 return;
		 case STATE_OPERATION_DELETE_GROUP:
			 deleteGroup(sVar);
			 m_iType = STATE_OPERATION_NONE;
			 return;
		 case STATE_OPERATION_KICK_USER_FROM_GROUP_BY_ADMIN_TYPE_GROUP_NAME:
			 m_sTypeName = sVar;
			 m_iType = STATE_OPERATION_KICK_USER_FROM_GROUP_BY_ADMIN_TYPE_USER_NICKNAME;
			 printf("user name: ");
			 return;
		 case STATE_OPERATION_KICK_USER_FROM_GROUP_BY_ADMIN_TYPE_USER_NICKNAME:
			 removeFromGroupByAdmin(m_sTypeName, sVar);
			 m_iType = STATE_OPERATION_NONE;
			 return;

		 default:
			 return;
		}
		break;
	 case STATE_UNAUTHORIZED:
		  m_sNickname = sVar;
		  authorization(m_sNickname);
		  return;
	}
	*/

	// OLD MENU
	/*if (m_bAuthorization == STATE_AUTHORIZED) {
		if (m_iType == 0) {
			if (!m_bSendMessage) {
				if (sVar == "0") {
					exit(1); //exit
				} else if (sVar == "1") {
					if (m_iLogin == STATE_LOGGED_OUT) {
						login(); // logoin
						return;
					}
					else if (m_iLogin == STATE_LOGIN_AS_USER || m_iLogin == STATE_LOGIN_AS_ADMIN) {
						logout(); // logout
						return;
					}
				} else if (sVar == "2") {
					if (m_iGroup == STATE_OUT_OF_GROUP) {
						printf("group name: ");
						m_iType = 5; // join to roup
						return;
					}
					else if (m_iGroup == STATE_IN_GROUP || m_iGroup == STATE_IN_GROUP_LEADER) {
						abandonGroup(); // abandon group
						return;
					}
				} else if (sVar == "3") {
					if (m_iGroup == STATE_OUT_OF_GROUP) {
						printf("group name: ");
						m_iType = 6; // create group
						return;
					} else if (m_iGroup == STATE_IN_GROUP_LEADER) {
						printf("user name: ");
						m_iType = 7; // change leader (only for leader)
					}
				} else if (sVar == "4") {
					if (m_iGroup == STATE_IN_GROUP_LEADER) {
						printf("user name: ");
						m_iType = 8; // remove from group (only for leader)
					}
					
				} else if (sVar == "5") {
					m_bSendMessage = true; // send message
					if (m_iGroup == STATE_OUT_OF_GROUP) { // for users without group
						printf("1->to all\n2->to user\n0->go to main menu\n");
						return;
					} else if (m_iGroup == STATE_IN_GROUP || m_iGroup == STATE_IN_GROUP_LEADER) { // for group members
						printf("1->to all\n2->to user\n3->to group\n0->go to main menu\n");
						return;
					}
				}
			}
			else {
				if (sVar == "0") {
					m_bSendMessage = false;
					printMenu();
				} else if (sVar == "1") {
					printf("message: ");
					m_iType = 1;
				} else if (sVar == "2") {
					printf("user name: ");
					m_iType = 2;
				} else if (sVar == "3") {
					if (m_iGroup == STATE_IN_GROUP || m_iGroup == STATE_IN_GROUP_LEADER) {
						printf("message: ");
						m_iType = 4;
					}
				}
			}
		}
		else if (m_iType == 1) {
			sendMessageToAll(sVar);
			m_bSendMessage = false;
			m_iType = 0;
			printMenu();
		}
		else if (m_iType == 2) {
			m_sTypeName = sVar;
			printf("message: ");
			m_iType = 3;
		}
		else if (m_iType == 3) {
			sendMessageToUser(sVar, m_sTypeName);
			m_sTypeName = "";
			m_bSendMessage = false;
			m_iType = 0;
			printMenu();
		}
		else if (m_iType == 4) {
			sendMessageToGroup(sVar);
			m_bSendMessage = false;
			m_iType = 0;
			printMenu();
		}
		else if (m_iType == 5) {
			joinGroup(sVar);
			m_iType = 0;
		}
		else if (m_iType == 6) {
			createGroup(sVar);
			m_iType = 0;
		} 
		else if (m_iType == 7) {
			changeLeader(sVar);
			m_iType = 0;
		}
		else if (m_iType == 8) {
			removeFromGroup(sVar);
			m_iType = 0;
		}

	} else  if (m_bAuthorization == STATE_UNAUTHORIZED) {
		printf("First u need to pass authorization successfully ! !\n");
	}*/

}
// take package -> decompress them -> do somthing
void CClient::recvBrain(std::string sPackage) { // will be harvester !
	//TODO dokonczyc mózg
	int iType = -1;
	std::string sMessage = "";
	int iPar = -1;
	std::string sID = "";
	CNetworkDevice::decompresPackage(iType, sMessage, iPar, sID, sPackage);

	// PROTOCOL
	// -> Protocol.h
	// END PROTOCOL

	if (iType < PROTOCOL_LENGTH && iType >= STATE_ERROR) {
		switch (iType) {
		 case STATE_REGISTER:
			 switch (iPar) {
			  case STATE_SUCCSSED:
				  printf("Register succeed\n");
				  break;
			  case STATE_FAILED:
				  printf("Register failed (%s)\n", sMessage.c_str());
				  break;
			 }
			 printMenu();
			 return;
		 case STATE_LOGIN:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 if (sID == "user") {
					 m_iLogin = STATE_LOGIN_AS_USER;
				 } else if (sID == "admin") {
					 m_iLogin = STATE_LOGIN_AS_ADMIN;
				 }
				 m_sNickname = sMessage;
				 printf("Login succeed\n");
				 printMenu();
				 break;
			 case STATE_FAILED:
				 printf("Login failed (%s)\n", sMessage.c_str());
				 break;
			 }
			 return;
		 case STATE_LOGOUT:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 m_iLogin = STATE_LOGGED_OUT;
				 m_iGroup = STATE_OUT_OF_GROUP;
				 m_sGroupName = "none";
				 m_sNickname = "";
				 printf("Logout succeed\n");
				 printMenu();
				 break;
			 case STATE_FAILED:
				 printf("Logout failed (%s)\n", sMessage.c_str());
				 break;
			 }
			 return;
		 case STATE_JOIN_GROUP:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 m_iGroup = STATE_IN_GROUP;
				 m_sGroupName = sID;
				 printf("Join to group: %s succeed\n", sID.c_str());
				 printMenu();
				 break;
			 case STATE_FAILED:
				 printf("Join to group %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 }
			 return;
		 case STATE_ABANDON_GROUP:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 m_iGroup = STATE_OUT_OF_GROUP;
				 m_sGroupName = "none";
				 printf("Abandon group: %s succeed\n", sID.c_str());
				 printMenu();
				 break;
			 case STATE_FAILED:
				 printf("Abandon group %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 }
			 return;
		 case STATE_SEND_MESSAGE:
			 switch (iPar) {
			  case STATE_DEFAULT_PAR:
				  printf("Send message failed (%s)\n", sMessage.c_str());
				  break;
			  case STATE_TO_ALL:
				  printf("(all chat)%s: %s\n", sID.c_str(), sMessage.c_str());
				  break;
			  case STATE_TO_GROUP:
				  printf("(%s chat)%s\n", sID.c_str(), sMessage.c_str());
				  break;
			  case STATE_TO_USER:
				  printf("(whisper chat)%s: %s\n", sID.c_str(), sMessage.c_str());
				  break;
			 }
			 return;
		 case STATE_CREATE_GROUP:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 m_iGroup = STATE_IN_GROUP_LEADER;
				 m_sGroupName = sID;
				 printf("Create group: %s succeed\n", sID.c_str());
				 printMenu();
				 break;
			 case STATE_FAILED:
				 printf("Create group %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 }
			 return;
		 case STATE_CHANGE_LEADER:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 m_iGroup = STATE_IN_GROUP;
				 printf("Change leader: %s succeed\n", sID.c_str());
				 printMenu();
				 break;
			 case STATE_FAILED:
				 printf("Change leader %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 case STATE_PROMOTED:
				 m_iGroup = STATE_IN_GROUP_LEADER;
				 printf("Group %s -> u have been promoted to leader\n", sID.c_str());
				 printMenu();
				 break;
			 }
			 return;
		 case STATE_KICK_USER_FROM_GROUP:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 printf("Remove from group: %s succeed\n", sID.c_str());
				 break;
			 case STATE_FAILED:
				 printf("Remove from group %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 case STATE_KICKED:
				 m_iGroup = STATE_OUT_OF_GROUP;
				 m_sGroupName = "none";
				 printf("Group %s -> kicked from group\n", sID.c_str());
				 printMenu();
				 break;
			 }
			 return;
		 case STATE_GROUP_MEMBERS_LIST:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 printf("Received group list: %s succeed\n", sID.c_str());
				 break;
			 case STATE_FAILED:
				 printf("Received group list %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 case STATE_GROUP_LIST:
				 printf("Members of %s:\n%s \n", sID.c_str(), sMessage.c_str());
				 break;
			 }
			 return;
		 case STATE_VOTE_KICK:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 printf("Vote kick: %s succeed\n", sID.c_str());
				 break;
			 case STATE_FAILED:
				 printf("Vote kick %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 }
			 return;
		 case STATE_INVITE_USER_TO_GROUP:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 printf("Invite send: %s succeed\n", sID.c_str());
				 break;
			 case STATE_FAILED:
				 printf("Invite send failed (%s)\n", sMessage.c_str());
				 break;
			 case STATE_INVITE:
				 printf("U have been invited to join to %s group |  Accept(1) / Refuse(2)\n", sID.c_str());
				 m_sGroupName = sID;
				 m_iType = STATE_OPERATION_INVITE_USER_TO_GROUP_ACCEPT;
				 break;
			 }
			 return;
		 case STATE_REQUEST_INVITE_USER_TO_GROUP:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 printf("Request send: %s succeed\n", sID.c_str());
				 break;
			 case STATE_FAILED:
				 printf("Request send %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;		 
			 case STATE_REQUEST:
				 printf("%s |  Accept(1) / Refuse(2)\n", sMessage.c_str());
				 m_sNicknameInvUser = sID;
				 m_iType = STATE_OPERATION_REQUEST_INVITE_USER_TO_GROUP_ACCEPT;
				 break;
			 }
			 return;
		 case STATE_CHANGE_GROUP_NAME:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 printf("Group name was changed: %s \n", sID.c_str());
				 m_sGroupName = sID;
				 printMenu();
				 break;
			 case STATE_FAILED:
				 printf("Group name change %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 }
			 return;
		 case STATE_USERS_LIST:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 printf("Request (send users list): %s succeed \n", sID.c_str());
				 break;
			 case STATE_FAILED:
				 printf("Request (send users list): %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 case STATE_USERS_LIST_RECV:
				 printf("%s", sMessage.c_str());
				 break;
			 }
			 return;
		 case STATE_ALL_GROUPS_LIST:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 printf("Request (send groups list): %s succeed \n", sID.c_str());
				 break;
			 case STATE_FAILED:
				 printf("Request (send groups list): %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 case STATE_ALL_GROUPS_LIST_RECV:
				 printf("%s", sMessage.c_str());
				 break;
			 }
			 return;
		 case STATE_DELETE_GROUP:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 printf("Delete group: %s succeed \n", sID.c_str());
				 break;
			 case STATE_FAILED:
				 printf("Delet group: %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 case STATE_DELETE_GROUP_DELETED:
				 m_iGroup = STATE_OUT_OF_GROUP;
				 m_sGroupName = "none";
				 printf("%s\n", sMessage.c_str());
				 printMenu();
				 break;
			 }
			 return;
		 case STATE_KICK_USER_FROM_GROUP_BY_ADMIN:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 printf("Remove from group: %s succeed \n", sID.c_str());
				 break;
			 case STATE_FAILED:
				 printf("Remove from group: %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 case STATE_KICKED_BY_ADMIN:
				 m_iGroup = STATE_OUT_OF_GROUP;
				 m_sGroupName = "none";
				 printf("Group %s -> kicked from group (by server Admin)\n", sID.c_str());
				 printMenu();
				 break;
			 }
			 return;
		 case STATE_UNREGISTER_USER:
			 switch (iPar) {
			 case STATE_SUCCSSED:
				 printf("Unregister user: %s succeed \n", sID.c_str());
				 break;
			 case STATE_FAILED:
				 printf("Unregister: %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				 break;
			 case STATE_UNREGISTER_BY_ADMIN:
				 printf("U was logged out because ur account was deleted by server admin\n");
				 logout();
				 break;
			 }
			 return;
		}
		// OLD BRAIN
		/*
		if (iType < m_iProTypeLen && iType >= STATE_ERROR) {
			if (iType == STATE_AUTHORIZATION) {
				// authorization
				if (iPar == STATE_SUCCSSED) {
					m_bAuthorization = true;
					printf("Authorization succeed\n");
					printMenu();
				}
				else if (iPar == STATE_FAILED) {
					printf("Authorization failed (%s)\n", sMessage.c_str());
				}
				else {
					printf("Authorization failed\n");
				}
			}
			else if (iType == STATE_LOGIN) {
				// login
				if (iPar == STATE_SUCCSSED) {
					m_iLogin = STATE_LOGIN_AS_USER;
					printf("Login succeed\n");
					printMenu();
				}
				else if (iPar == STATE_FAILED) {
					printf("Login failed (%s)\n", sMessage.c_str());
				}
				else {
					printf("Login failed\n");
				}

			}
			else if (iType == STATE_LOGOUT) {
				//logout
				if (iPar == STATE_SUCCSSED) {
					m_iLogin = STATE_LOGGED_OUT;
					printf("Logout succeed\n");
					printMenu();
				}
				else if (iPar == STATE_FAILED) {
					printf("Logout failed (%s)\n", sMessage.c_str());
				}
				else {
					printf("Logout failed\n");
				}
			}
			else if (iType == STATE_JOIN_GROUP) {
				//joinGroup
				if (iPar == STATE_SUCCSSED) {
					m_iGroup = STATE_IN_GROUP;
					m_sGroupName = sID;
					printf("Join to group: %s succeed\n", sID.c_str());
					printMenu();
				}
				else if (iPar == STATE_FAILED) {
					printf("Join to group %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				}
				else {
					printf("Join to group %s failed\n", sID.c_str());
				}

			}
			else if (iType == STATE_ABANDON_GROUP) {
				//abandonGroup
				if (iPar == STATE_SUCCSSED) {
					m_iGroup = STATE_OUT_OF_GROUP;
					m_sGroupName = "none";
					printf("Abandon group: %s succeed\n", sID.c_str());
					printMenu();
				}
				else if (iPar == STATE_FAILED) {
					printf("Abandon group %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				}
				else {
					printf("Abandon group %s failed\n", sID.c_str());
				}

			}
			else if (iType == STATE_SEND_MESSAGE) {
				//sendMessage

				if (iPar == STATE_ERROR) {
					printf("%s\n", sMessage.c_str());
				}
				else if (iPar == STATE_TO_ALL) {

					printf("(all chat)%s: %s\n", sID.c_str(), sMessage.c_str());

				}
				else if (iPar == STATE_TO_GROUP) {

					printf("(%s chat)%s\n", sID.c_str(), sMessage.c_str());

				}
				else if (iPar == STATE_TO_USER) {

					printf("(whisper chat)%s: %s\n", sID.c_str(), sMessage.c_str());

				}
				else {

				}
			}
			else if (iType == STATE_CREATE_GROUP) {
				//createGroup
				if (iPar == STATE_SUCCSSED) {
					m_iGroup = STATE_IN_GROUP_LEADER;
					m_sGroupName = sID;
					printf("Create group: %s succeed\n", sID.c_str());
					printMenu();
				}
				else if (iPar == STATE_FAILED) {
					printf("Create group %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				}
				else {
					printf("Create group %s failed\n", sID.c_str());
				}
			}
			else if (iType == STATE_CHANGE_LEADER) {
				// change leader (only for leader)
				if (iPar == STATE_SUCCSSED) {
					m_iGroup = STATE_IN_GROUP;
					printf("Change leader: %s succeed\n", sID.c_str());
					printMenu();
				}
				else if (iPar == STATE_FAILED) {
					printf("Change leader %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				}
				else if (iPar == STATE_PROMOTED) {
					m_iGroup = STATE_IN_GROUP_LEADER;
					printf("Group %s -> u have been promoted to leader\n", sID.c_str());
					printMenu();
				}
				else {
					printf("Change leader %s failed\n", sID.c_str());
				}
			}
			else if (iType == STATE_KICK_USER_FROM_GROUP) {
				// remove user from group (only for leader)
				if (iPar == STATE_SUCCSSED) {
					printf("Remove from group: %s succeed\n", sID.c_str());
					printMenu();
				}
				else if (iPar == STATE_FAILED) {
					printf("Remove from group %s failed (%s)\n", sID.c_str(), sMessage.c_str());
				}
				else if (iPar == STATE_KICKED) {
					m_iGroup = STATE_OUT_OF_GROUP;
					m_sGroupName = "none";
					printf("Group %s -> kicked from group\n", sID.c_str());
					printMenu();
				}
				else {
					printf("Remove from group %s failed\n", sID.c_str());
				}
			}
		} else {
			printf("ERROR SERVER SENDS INFORMATION WITH WRONG TYPE\n");
		}*/

	} else {
		printf("ERROR SERVER SENDS INFORMATION WITH WRONG TYPE\n");
	}
}

