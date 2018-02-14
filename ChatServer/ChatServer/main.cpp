// main.cpp : Defines the entry point for the console application.
//
#include "ServerNetwork.h"

CServerNetwork * Server;

int main() {

	// revive server 
	Server = new CServerNetwork();

	// start main function
	Server->start();


    return 0;
}

