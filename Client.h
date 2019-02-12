#ifndef CLIENT_H
#define CLIENT_H

#include <SFML\Network.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "PacketCodes.h"
#include "MSG.h"
#include "ll.h"

typedef struct
{

	sfTcpSocket **TcpSocket;
	sfUdpSocket **UdpSocket;
	unsigned short ClientPort, ServerPort;
	sfIpAddress IpAddress, ServerIpAddress;

	short ConsoleWidth, ConsoleHeight;
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	unsigned int ClientNo;
	sfBool RunClient;
	
	
	HANDLE RunClientMtx;

	
	HANDLE TypingThread;
	HANDLE OutputMtx;

	Message *Messages;
	

	char Name[1024];

} Client;

void Client_Client (Client *client, char name [1024]);

void Client_Destroy(Client *client);

void Client_Loop (Client *client);

void Client_ConnectToServer (Client *client);

bool Client_RegisterMainInfo (Client *client, sfPacket **Packet, sfInt16 CheckCode, sfInt32 *pExistingSegmentInfoComing);
void Client_RegisterMSGInfo (Client *client, sfPacket **Packet);

void Client_RegisterNewMSG (Client *client, sfPacket **Packet);
void Client_RegisterNewClient ( Client *client, sfPacket **Packet);

void Client_TypingLoop (Client *client);
void Client_SendNewMSG (Client *client, Message _MSG);

void Client_SetRunClient (Client *client, const bool set) {
	WaitForSingleObject (client->RunClientMtx, INFINITE);client->RunClient=set;ReleaseMutex(client->RunClientMtx);};

sfBool Client_GetRunClient (Client *client) {
	WaitForSingleObject (client->RunClientMtx, INFINITE);sfBool runClient = client->RunClient;ReleaseMutex(client->RunClientMtx);return runClient;};

void Client_PrintMSG (Client *client, Message _MSG);



#endif