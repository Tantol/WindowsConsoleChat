// main.cpp : Defines the entry point for the console application.
//
#include <iostream>
#include <string>
#include <stdio.h>
#include <conio.h>
#include <thread>

#include "Client.h"

CClient * cClient;

// loop with 2 functions:
// -> recvQuestion() ; keep one's ear to the ground
// -> sendQuestion() ; if client typed somthing -> send message to server // else -> do nothing 
void loopQuestion() {
	while ("") {
		Sleep(10);
		cClient->recvQuestion();
		cClient->sendQuestion();
	}
}

// loop with 1 function:
// -> loopKeyBoard() ; take chars from user keyboard (after -> Enter)
void loopKey() {
	while ("") {
		cClient->loopKeyboard();
	}
}

int main() {

	cClient = new CClient();
	
	// new thread with keyboard (read) loop 
	std::thread t(loopKey);

	// start listen
	loopQuestion();
	
	return 0;
}

