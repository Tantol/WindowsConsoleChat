#pragma once
// --- CLIENT ---
// define CLIENT STATE (PROTOCOL)
// ->>>
// LOGIN:
#define STATE_LOGGED_OUT 0
#define STATE_LOGIN_AS_USER 1
#define STATE_LOGIN_AS_ADMIN 2
// ->>>
// GROUP:
#define STATE_OUT_OF_GROUP 0
#define STATE_IN_GROUP 1
#define STATE_IN_GROUP_LEADER 2

// define STATE KEYBOARD MENU
// ->>>
// SEND MESSAGE:
#define STATE_OPERATION_SEND_MESSAGE true
#define STATE_OPERATION_DONT_SEND_MESSAGE false
// ->>>
// OPERATION:
#define STATE_OPERATION_NONE 0
// ->>> STATE_LOGGED_OUT
#define STATE_OPERATION_LOGIN 1
#define STATE_OPERATION_LOGIN_PASSWORD 2
#define STATE_OPERATION_REGISTER 3
#define STATE_OPERATION_REGISTER_PASSWORD 4
#define STATE_OPERATION_REGISTER_PASSWORD_VALIDATE 5
// ->>> STATE_LOGIN_AS_USER
#define STATE_OPERATION_SEND_MESSAGE_TO_ALL 1
#define STATE_OPERATION_TYPE_MESSAGE_TO_USER 2
#define STATE_OPERATION_SEND_MESSAGE_TO_USER 3
#define STATE_OPERATION_SEND_MESSAGE_TO_GROUP 4
#define STATE_OPERATION_JOING_GROUP 5
#define STATE_OPERATION_CREATE_GROUP 6
#define STATE_OPERATION_CHANGE_LEADER 7
#define STATE_OPERATION_KICK_USER_FROM_GROUP 8
#define STATE_OPERATION_VOTE_KICK 9
#define STATE_OPERATION_INVITE_USER_TO_GROUP 10
#define STATE_OPERATION_INVITE_USER_TO_GROUP_ACCEPT 11
#define STATE_OPERATION_REQUEST_INVITE_USER_TO_GROUP 12
#define STATE_OPERATION_REQUEST_INVITE_USER_TO_GROUP_ACCEPT 13
#define STATE_OPERATION_CHANGE_GROUP_NAME 14
// ->>> STATE_LOGIN_AS_ADMIN
#define STATE_OPERATION_DELETE_GROUP 15
#define STATE_OPERATION_KICK_USER_FROM_GROUP_BY_ADMIN_TYPE_GROUP_NAME 16
#define STATE_OPERATION_KICK_USER_FROM_GROUP_BY_ADMIN_TYPE_USER_NICKNAME 17
#define STATE_OPERATION_UNREGISTER_USER 18
// ->>> TODO -- MODERATOR OPERATION // (...)

// --- CLIENT / SERVER ---
// define STATE RECEIVE MENU
// ->>>
// TYPE:
#define STATE_REGISTER 0
#define STATE_LOGIN 1
#define STATE_LOGOUT 2
#define STATE_JOIN_GROUP 3
#define STATE_ABANDON_GROUP 4
#define STATE_SEND_MESSAGE 5
#define STATE_CREATE_GROUP 6
#define STATE_CHANGE_LEADER 7 // only for leader
#define STATE_KICK_USER_FROM_GROUP 8 // only for leader
#define STATE_GROUP_MEMBERS_LIST 9
#define STATE_VOTE_KICK 10
#define STATE_INVITE_USER_TO_GROUP 11
#define STATE_REQUEST_INVITE_USER_TO_GROUP 12
#define STATE_CHANGE_GROUP_NAME 13
#define STATE_USERS_LIST 14
#define STATE_ALL_GROUPS_LIST 15
#define STATE_DELETE_GROUP 16
#define STATE_KICK_USER_FROM_GROUP_BY_ADMIN 17
#define STATE_UNREGISTER_USER 18
// ->>> TYPE LENGTH:
#define PROTOCOL_LENGTH 19
// ->>>
// PAR:
// default param
#define STATE_DEFAULT_PAR 0
// succssed / failed
#define STATE_SUCCSSED 1
#define STATE_FAILED 2
// send parm
#define STATE_ERROR 0
#define STATE_TO_ALL 1
#define STATE_TO_GROUP 2
#define STATE_TO_USER 3
// kicked / promoted / group list
#define STATE_KICKED 3
#define STATE_PROMOTED 3
#define STATE_GROUP_LIST 3
// inv parm
#define STATE_INVITE_ACCEPT 1
#define STATE_INVITE_REFUSE 2
#define STATE_INVITE 3
// request parm
#define STATE_REQUEST_ACCEPT 1
#define STATE_REQUEST_REFUSE 2
#define STATE_REQUEST 3
// recv list
#define STATE_USERS_LIST_RECV 3
#define STATE_ALL_GROUPS_LIST_RECV 3
// group deleted
#define STATE_DELETE_GROUP_DELETED 3
// kicked by admin
#define STATE_KICKED_BY_ADMIN 3
// unregister by admin -> loggout 
#define STATE_UNREGISTER_BY_ADMIN 3

// --- SERVER ---
// OPERATION:
#define STATE_OPERATION_SUCCEED true
#define STATE_OPERATION_FAILED false

// PROTOCOL
//
// ---- SERVER / CLIENT ----
// TYPE:
// 0 -> rezervated for system 
// 1 -> login
// 2 -> logout
// 3 -> join group
// 4 -> abandon group
// 5 -> send message
// 6 -> create group
// 7 -> change leader
// 8 -> remove from group
//
// PAR:
// 0 -> rezervated for system
// 1 -> send to all
// 2 -> send to group
// 3 -> send to selected user
//
// ID (if par):
// IF (par == 0) -> rezervated for system
// IF (par == 1) -> how cares
// IF (par == 2) -> groupId
// IF (par == 3) -> userId (selected to send message)
// ->>>
// ID (if type):
// IF (type == 0) -> rezervated for system
// IF (type == 1) -> userNick
// IF (type == 2) -> userNick
// IF (type == 3) -> groupName
// IF (type == 4) -> groupName
// IF (type == 5) -> groupName (if par == 2) / userName (if par == 3)
// IF (type == 6) -> groupName
//
// ---- CLIENT ----
//
// AUTHORIZATION:
// TRUE -> succssed authorization
// FALSE -> failed authorization
//
// LOGIN:
// 0 -> logout
// 1 -> simple user (login)
// 2 -> admin (login)
// 
// GROUP:
// 0 -> without group
// 1 -> group member
// 2 -> group leader
//
// END PROTOCOL

// --- pomysly ---
//////////////////////////////////////////////////////////////////////
// ...
// ...
//////////////////////////////////////////////////////////////////////

// --- *** ----
//////////////////////////////////////////////////////////////////////
// TODO:
// 0!. Polepszyc hashowanie i wykombinowac moze inne rozwiazanie z baza danych, sam plik / pliki csv. to *chyba* za malo.
// 1. Zapis binarnych logow.
// 2. Odczyt binarnych logow (szczegolowy odczyt / mniej szczegolowy / odczyt dla wybranej osoby w wybranym czasie ect.)
// 3. Strona webowa ktora bedzie oparta o obecny serwer.
// 4. *Aplikacja okienkowa oparta na serwerze.
// 5. *Aplikacja przenosna wykorzystujaca obecny serwer.
//
//////////////////////////////////////////////////////////////////////

// --- CLIENT AND SERVER ---
//////////////////////////////////////////////////////////////////////
// TODO:
// 2. *Wszedzie*:
// -> utworzyc metody dla administracji:
//    - nakladanie kar czasowych na: (...) // *chyba* potrzebna jakas fajna baza danych
//		 - tworzenie nowych grup
//		 - logowanie sie
//       - pisanie
//
// 3. *Wszedzie*:
// -> utworzyc metody dla zwyklych uzytkownikow: (...)
//    - ignorowanie wybranej osoby / osob (->> utworzyc mape ignorowanych osob) // *chyba* potrzebna jakas fajna baza danych
//    - zglaszanie graczy za: // *chyba* potrzebna jakas fajna baza danych
//       - nekanie
//		 - niewlasciwe zachowanie
//		 - niewlasciwe tresci
//		 - niewlasciwy nickname / nazwa grupy
//		 - obrazliwe zachowanie
//		 - oszukiwanie
//		 - inne
//	  - dodawnie znajomych i ich usuwanie (->> utworzyc mape znajomych) // *chyba* potrzebna jakas fajna baza danych
//    - wyswietlanie dostepnych znajomych // *chyba* potrzebna jakas fajna baza danych
//
//////////////////////////////////////////////////////////////////////

// --- SERVER ---
//////////////////////////////////////////////////////////////////////
// TODO:
//
// 5. *Wszedzie*: (... ??)
// -> otworzyc funkcje systemowe (automatyczne):
//    - usuwanie nieaktywnych clientow z listy (po okreslonym czasie) // nie mam pomyslu JAK
//    - zdejmowanie nalozonych blokad (sprawdzanie czy czas juz uplynol, jezeli tak to "uwolnij clienta") // nie mam pomyslu JAK
//
// 6. CServerNetowrk.cpp / CServerNetwork.h (opcjonalnie):
// -> niszczyc nieaktywne thready (zwalnianie pamieci) // funkcja DWORD wyrzuca 0 i to chyba zamyka thread (UPERNIC SIE)
//
/////////////////////////////////////////////////////////////////////

// --- CLIENT ---
//////////////////////////////////////////////////////////////////////
// TODO:
//  ...done
//////////////////////////////////////////////////////////////////////