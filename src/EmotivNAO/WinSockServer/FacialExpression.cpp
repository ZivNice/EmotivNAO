/****************************************************************************
**
** Copyright 2015 by Emotiv. All rights reserved
**
** Example - FacialExpressionDemo
**
** This example demonstrates how an application can use the ExpressivTM
** detection suite to control an animated head model called BlueAvatar.
** The model emulates the facial expressions made by the user wearing an
** Emotiv headset.
** FacialExpressionDemo connects to Emotiv EmoEngineTM and retrieves EmoStatesTM
** for all attached users. The EmoState is examined to determine which
** facial expression best matches the user�s face.
** FacialExpressionDemo communicates the detected expressions to the separate
** BlueAvatar application by sending a UDP packet which follows a simple,
** pre-defined protocol
****************************************************************************/
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <map>
#include <sstream>
#include <cassert>
#include <stdexcept>
#include <cstdlib>
#include <stdio.h>

#include "IEmoStateDLL.h"
#include "Iedk.h"
#include "IedkErrorCode.h"
#include "FacialCommands.h"
#include "FacialExpression.h"

#ifdef _WIN32
#include <conio.h>
#pragma comment(lib, "Ws2_32.lib")
#endif
#if __linux__ || __APPLE__
#include <unistd.h>
#include <termios.h>
#endif

std::string numberToString(const int);
void sendFacialExpressionAnimation(EmoStateHandle);
void handleFacialExpressionEvent(std::ostream&, EmoEngineEventHandle);
bool handleUserInput();
void promptUser();

#if __linux__ || __APPLE__
int _kbhit(void);
int _getch(void);
#endif
#ifdef __APPLE__
int _kbhit(void)
{
	struct timeval tv;
	fd_set rdfs;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&rdfs);
	FD_SET(STDIN_FILENO, &rdfs);

	select(STDIN_FILENO + 1, &rdfs, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &rdfs);
}

int _getch()
{
	int r;
	unsigned char c;
	if ((r = read(0, &c, sizeof(c))) < 0)
	{
		return r;
	}
	else
	{
		return c;
	}
}
#endif

int startExpression() {

	char *myargv[1];
	int myargc = 1;
	myargv[0] = _strdup("Gyro Tracking");

	// location of the machine running the "BlueGirl" head model
	std::string receiverHost = "localhost";

	if (myargc > 2) {
		std::cout << "Usage: " << myargv[0] << " <hostname>" << std::endl;
		std::cout << "The arguments specify the host of the head model "
			"(Default: localhost)" << std::endl;
		return 1;
	}

	if (myargc > 1) {
		receiverHost = std::string(myargv[1]);
	}

	EmoEngineEventHandle eEvent = IEE_EmoEngineEventCreate();
	EmoStateHandle eState = IEE_EmoStateCreate();
	unsigned int userID = 0;
	const int CONTROL_PANEL_PORT = 3008;

	try {

		/*Connect with EmoEngine*/
		if (IEE_EngineConnect() != EDK_OK) {
			throw std::runtime_error("Emotiv Driver start up failed.");
		}
		/*************************************************************/
		/*Connect with Emocomposer app on port 1726*/
		/*Connect with Control Panel app on port 3008*/
		//        if (IEE_EngineRemoteConnect("127.0.0.1",1726) != EDK_OK) {
		//            throw std::runtime_error("Emotiv Driver start up failed.");
		//        }
		/*************************************************************/
		else {
			std::cout << "Emotiv Driver started!" << std::endl;
		}

		int startSendPort = 30000;
		//std::map<unsigned int, SocketClient> socketMap;

		std::cout << "Type \"exit\" to quit, \"help\" to list available commands..."
			<< std::endl;
		promptUser();

		while (true) {
			// Handle the user input
			if (_kbhit()) {
				if (!handleUserInput()) {
					break;
				}
			}

			int state = IEE_EngineGetNextEvent(eEvent);

			// New event needs to be handled
			if (state == EDK_OK) {

				IEE_Event_t eventType = IEE_EmoEngineEventGetType(eEvent);
				IEE_EmoEngineEventGetUserId(eEvent, &userID);

				switch (eventType) {

					// New headset connected
					// create a new socket to send the animation
				case IEE_UserAdded:
				{
					std::cout << std::endl << "New user " << userID
						<< " added, sending FacialExpression animation to ";
					std::cout << receiverHost << ":" << startSendPort << "..."
						<< std::endl;
					promptUser();

					//socketMap.insert(std::pair<unsigned int, SocketClient>(
						//userID, SocketClient(receiverHost, startSendPort, UDP)));

					startSendPort++;
					break;
				}

				// Headset disconnected, remove the existing socket
				case IEE_UserRemoved:
				{
					std::cout << std::endl << "User " << userID
						<< " has been removed." << std::endl;

					promptUser();
					//std::map<unsigned int, SocketClient>::iterator iter;
					//iter = socketMap.find(userID);
					//if (iter != socketMap.end()) {
						//socketMap.erase(iter);
					//}
					break;
				}

				// Send the FacialExpression animation
				//if EmoState has been updated
				case IEE_EmoStateUpdated:
				{
					IEE_EmoEngineEventGetEmoState(eEvent, eState);

					//std::map<unsigned int, SocketClient>::iterator iter;
					//iter = socketMap.find(userID);
					//if (iter != socketMap.end()) {
					sendFacialExpressionAnimation(eState);
					//}
					break;
				}

				// Handle FacialExpression training event
				case IEE_FacialExpressionEvent:
				{
					handleFacialExpressionEvent(std::cout, eEvent);
				}

				default:
					break;
				}
			}
			else if (state != EDK_NO_EVENT) {
				std::cout << std::endl << "Internal error in Emotiv Engine!"
					<< std::endl;
				break;
			}

#ifdef _WIN32
			//Sleep(15);
#endif
#if __linux__  || __APPLE__
			usleep(10000);
#endif
		}
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		std::cout << "Press 'Enter' to exit..." << std::endl;
		getchar();
	}

	IEE_EngineDisconnect();
	IEE_EmoStateFree(eState);
	IEE_EmoEngineEventFree(eEvent);

	return 0;
}


std::string numberToString(const int n) {
	char buf[80];
	sprintf(buf, "%d", n);
	return std::string(buf);
}


void sendFacialExpressionAnimation(EmoStateHandle eState) {
	std::ostringstream output;

	IEE_FacialExpressionAlgo_t upperFaceType =
		IS_FacialExpressionGetUpperFaceAction(eState);
	IEE_FacialExpressionAlgo_t lowerFaceType =
		IS_FacialExpressionGetLowerFaceAction(eState);

	float upperFaceAmp = IS_FacialExpressionGetUpperFaceActionPower(eState);
	float lowerFaceAmp = IS_FacialExpressionGetLowerFaceActionPower(eState);

	if (IS_FacialExpressionIsBlink(eState)) {
		std::cout << "BLINKING" << std::endl;
		output << "B,";
	}

	if (IS_FacialExpressionIsLeftWink(eState)) {
		output << "l,";
	}

	if (IS_FacialExpressionIsRightWink(eState)) {
		output << "r,";
	}


	if (upperFaceAmp > 0.55) {
		switch (upperFaceType) {
		case FE_SURPRISE:	
			std::cout << "SURPRISE" << std::endl;	
			faceCommand = "SURPRISE";
			break;
		case FE_FROWN:   
			std::cout << "FROWN" << std::endl; 
			faceCommand = "FROWN";
			break;
		default:			break;
		}
		output << numberToString(static_cast<int>(upperFaceAmp*100.0f)) << ",";
	}

	if (lowerFaceAmp > 0.4) {
		switch (lowerFaceType) {
		case FE_SMILE:	
			std::cout << "SMILE" << std::endl;	
			faceCommand = "SMILE";
			break;
		default:			break;
		}
		output << numberToString(static_cast<int>(lowerFaceAmp*100.0f)) << ",";
	}

	if (lowerFaceAmp > 0.25) {
		switch (lowerFaceType) {
		case FE_CLENCH:	
			std::cout << "TEETH CLENCH" << std::endl;	
			faceCommand = "TEETH CLENCH";
			break;
		default:			break;
		}
		output << numberToString(static_cast<int>(lowerFaceAmp*100.0f)) << ",";
	}

	std::string outString = output.str();

	// Remove the last comma
	if (outString.length()) {
		outString.resize(outString.length() - 1);
	}

	if (!outString.length()) {
		outString = std::string("neutral");
		std::cout << "NEUTRAL" << std::endl;
		faceCommand = "NEUTRAL";
	}

	//sock.SendBytes(outString);
}


void handleFacialExpressionEvent(std::ostream& os,
	EmoEngineEventHandle expressivEvent) {
	unsigned int userID = 0;
	IEE_EmoEngineEventGetUserId(expressivEvent, &userID);
	IEE_FacialExpressionEvent_t eventType =
		IEE_FacialExpressionEventGetType(expressivEvent);

	switch (eventType) {

	case IEE_FacialExpressionTrainingStarted:
	{
		os << std::endl << "FacialExpression training for user " << userID
			<< " STARTED!" << std::endl;
		break;
	}

	case IEE_FacialExpressionTrainingSucceeded:
	{
		os << std::endl << "FacialExpression training for user " << userID
			<< " SUCCIEEDED!" << std::endl;
		break;
	}

	case IEE_FacialExpressionTrainingFailed:
	{
		os << std::endl << "FacialExpression training for user " << userID
			<< " FAILED!" << std::endl;
		break;
	}

	case IEE_FacialExpressionTrainingCompleted:
	{
		os << std::endl << "FacialExpression training for user " << userID
			<< " COMPLETED!" << std::endl;
		break;
	}

	case IEE_FacialExpressionTrainingDataErased:
	{
		os << std::endl << "FacialExpression training data for user " << userID
			<< " ERASED!" << std::endl;
		break;
	}

	case IEE_FacialExpressionTrainingRejected:
	{
		os << std::endl << "FacialExpression training for user " << userID
			<< " REJECTED!" << std::endl;
		break;
	}

	case IEE_FacialExpressionTrainingReset:
	{
		os << std::endl << "FacialExpression training for user " << userID
			<< " RESET!" << std::endl;
		break;
	}

	case IEE_FacialExpressionNoEvent:
	default:
		//@@ unhandled case
		assert(0);
		return;
	}
	promptUser();
}


bool handleUserInput() {

	static std::string inputBuffer;

	char c = _getch();
#if __linux__ || __APPLE__
	if (c == '\n') {
#else
	if (c == '\r') {
#endif
		std::cout << std::endl;
		std::string command;

		const size_t len = inputBuffer.length();
		command.reserve(len);

		// Convert the input to lower case first
		for (size_t i = 0; i < len; i++) {
			command.append(1, tolower(inputBuffer.at(i)));
		}

		inputBuffer.clear();

		bool success = parseCommand(command, std::cout);
		promptUser();
		return success;
	}
	else {
		if (c == '\b') { // Backspace key
			if (inputBuffer.length()) {
				putchar(c);
				putchar(' ');
				putchar(c);
				inputBuffer.erase(inputBuffer.end() - 1);
			}
		}
		else {
			inputBuffer.append(1, c);
			std::cout << c;
		}
	}

	return true;
	}

void promptUser()
{
	std::cout << "FacialExpressionDemo> ";
}

#ifdef __linux__
int _kbhit(void)
{
	struct timeval tv;
	fd_set read_fd;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&read_fd);
	FD_SET(0, &read_fd);

	if (select(1, &read_fd, NULL, NULL, &tv) == -1)
		return 0;

	if (FD_ISSET(0, &read_fd))
		return 1;

	return 0;
}

int _getch(void)
{
	struct termios oldattr, newattr;
	int ch;

	tcgetattr(STDIN_FILENO, &oldattr);
	newattr = oldattr;
	newattr.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);

	return ch;
}
#endif
