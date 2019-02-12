#ifndef SERVER_H
#define SERVER_H

#include <SFML/Network.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "PacketCodes.h"
#include "MSG.h"
#include "ll.h"

typedef struct Server
{

	unsigned ClientPort, ServerPort;
	sfIpAddress IpAddress;
	sfUdpSocket **UdpSocket;
	sfTcpSocket **TcpSocket;
	unsigned int TotalClientNo;

	Message *Messages;
	unsigned int TotalMessageNo;

} Server;

void Server_Server (Server* server);
void Server_Destroy(Server *server);
void Server_Loop (Server* server);
void Server_AcceptNewClient (Server* server, sfPacket **Packet, sfIpAddress Address);
void Server_RegisterMSG (Server* server, sfPacket **Packet);
void Server_SendMSG (Server* server, Message _MSG);
void Server_PrintMSG(Server *server, Message _MSG);

#endif