#include "Server.h"

void Server_Server (struct Server* server)
{
	server->ServerPort = 55001;
	server->ClientPort = 55002;
	server->IpAddress = sfIpAddress_getLocalAddress ();
	
	server->TotalClientNo = 0;
	server->TotalMessageNo = 0;

	server->UdpSocket = sfUdpSocket_create();
	server->TcpSocket = sfTcpSocket_create();

	sfUdpSocket_bind (server->UdpSocket, server->ServerPort, server->IpAddress);
	sfUdpSocket_setBlocking (server->UdpSocket, true);

	server->Messages = NULL;
}
//  free socket 
void Server_Destroy(Server *server) 
{

	sfUdpSocket_destroy(server->UdpSocket);
	sfTcpSocket_destroy(server->TcpSocket);

}

void Server_Loop (struct Server* server)
{
	char *address;
	address = malloc(1024 * sizeof(char));

	sfIpAddress_toString (server->IpAddress, address);
	printf ("Server@%s:%i initialized and started. \nAwaiting clients...\n", address, server->ServerPort);
	
	free(address);

	while (true)
	{

		sfInt8 packetCode = -1;

		sfIpAddress Sender = sfIpAddress_None;
		unsigned short SenderPort = 0;
		sfPacket **Packet = sfPacket_create();     
		
		sfUdpSocket_receivePacket(server->UdpSocket, Packet, &Sender, &SenderPort);
		sfPacket **OriginalPacket = sfPacket_create();
		*OriginalPacket = *Packet;

		packetCode = sfPacket_readInt8 (Packet);

		ePacketCodes PacketCode = (ePacketCodes) (packetCode);

		switch (PacketCode)
		{
		
		// Error
		default:
			char senderAddress[1024];
			sfIpAddress_toString (Sender, senderAddress);
			printf ("Packet from %s:%i couldn't be read!", senderAddress, SenderPort);
			break;

		case SER_NEW_MSG:
			Server_RegisterMSG (server, Packet);
			break;

		case SER_CLIENTCONNECT:
			Server_AcceptNewClient (server, Packet, Sender);
			break;
		

		};

		sfPacket_destroy(Packet);
		sfPacket_destroy(OriginalPacket);

	}

}

void Server_AcceptNewClient (struct Server* server, sfPacket **Packet, sfIpAddress Address)
{

	sfInt16 CheckCode = -1;
	char name[4096];

	sfPacket_readString (Packet, name);
	CheckCode = sfPacket_readInt16 (Packet);

	if (CheckCode != -1)
	{ 

		sfPacket **NewPacket1 = sfPacket_create ();

		unsigned int iStatusCode = (unsigned int) (CLI_IND_CLIENTCONNECT);
		sfInt8 StatusCode = (sfInt8) (iStatusCode);
		sfPacket_writeInt8 (NewPacket1, StatusCode);

		sfInt16 clientNo = (sfInt16) (server->TotalClientNo++);
		sfUint32 address = sfIpAddress_toInteger (Address);

		unsigned int messages_size = 0;
		for (Message *it_msg = server->Messages; it_msg; it_msg = _ll_next(it_msg))
		{
			messages_size++;
		}

		sfInt32 NumberOfMSGInfoPackets = (sfInt32) (messages_size);
		sfPacket_writeInt16 (NewPacket1, clientNo);
		sfPacket_writeUint32 (NewPacket1, address);
		sfPacket_writeInt32 (NewPacket1, NumberOfMSGInfoPackets);
		sfPacket_writeInt16 (NewPacket1, CheckCode);

		sfPacket **NewPacket2 = sfPacket_create();

		if (messages_size > 0)
		{

			unsigned int iStatusCode2 = (unsigned int)(CLI_IND_MSGINFO);
			sfInt8 StatusCode2 = (sfInt8)(iStatusCode2);
			sfPacket_writeInt8(NewPacket2, StatusCode2);
			sfPacket_writeInt32(NewPacket2, NumberOfMSGInfoPackets);
			for (Message *it_msg = server->Messages; it_msg; it_msg = _ll_next(it_msg))
			{

				long long Timestamp = Message_GetTimestamp(it_msg);
				// Convert timestamp (8 bytes) into two 4 byte chunks
				unsigned long hi, lo;
				memcpy(&lo, &Timestamp, sizeof lo);
				memcpy(&hi, (char *)&Timestamp + sizeof lo, sizeof hi);
				sfUint32 hi_ = (sfUint32)(hi);
				sfUint32 lo_ = (sfUint32)(lo);


				char StringMsg[1024];
				strcpy(StringMsg, Message_GetMsg(it_msg));

				char StringName[1024];
				strcpy(StringName, Message_GetName(it_msg));

				char name[4096], msg[4096];

				strcpy(msg, StringMsg);

				strcpy(name, StringName);

				sfUint32 senderAddress = sfIpAddress_toInteger(Message_GetSenderAddress(it_msg));

				sfPacket_writeInt32(NewPacket2, (sfInt32)Message_GetID(it_msg));
				sfPacket_writeString(NewPacket2, msg);
				sfPacket_writeUint32(NewPacket2, senderAddress);
				sfPacket_writeString(NewPacket2, name);
				sfPacket_writeUint32(NewPacket2, hi_);
				sfPacket_writeUint32(NewPacket2, lo_);

			}

		}
		

		sfUdpSocket_sendPacket(server->UdpSocket, NewPacket1, Address, server->ClientPort);

		if (messages_size > 0)
			sfUdpSocket_sendPacket(server->UdpSocket, NewPacket2, Address, server->ClientPort);

		char addressString[1024];
		sfIpAddress_toString (Address, addressString);
		printf ("%s @ %s:%i connected!\n", name, addressString, server->ClientPort);
		 
		sfPacket **NewPacket3 = sfPacket_create();
		unsigned int iStatusCode3 = (unsigned int) (CLI_CLIENTCONNECT);
		sfInt8 StatusCode3 = (sfInt8) (iStatusCode3);
		sfPacket_writeInt8 (NewPacket3, StatusCode3);
		sfPacket_writeInt16 (NewPacket3, clientNo);
		sfPacket_writeUint32 (NewPacket3, address);
		sfPacket_writeString (NewPacket3, name);

		if (sfUdpSocket_sendPacket(server->UdpSocket, NewPacket3, sfIpAddress_Broadcast, server->ClientPort) == -1)
			printf ("Could not send data to clients!\n");

		sfPacket_destroy(NewPacket1);
		sfPacket_destroy(NewPacket2);
		sfPacket_destroy(NewPacket3);

	}

}

void Server_RegisterMSG (struct Server* server, sfPacket **Packet)
{

	sfUint32 senderAddress;
	sfUint32 hi_,lo_;
	char msg[4096], name[4096];
	
	long long Timestamp;
	unsigned int hi,lo;   

	
	sfPacket_readString (Packet, msg);
	senderAddress = sfPacket_readUint32 (Packet);
	sfPacket_readString (Packet, name);
	hi_ = sfPacket_readUint32 (Packet);
	lo_ = sfPacket_readUint32 (Packet);

	sfIpAddress SenderAddress = sfIpAddress_fromInteger (senderAddress);

	hi = (unsigned long) (hi_);
	lo = (unsigned long) (lo_);
	
	// Copying hi & lo variables into timestamp
	memcpy (&Timestamp, &lo, sizeof lo);
	memcpy ((char *) &Timestamp + sizeof lo, &hi, sizeof hi);

	
	Message _MSG;
	Message_Message (&_MSG, msg, SenderAddress, name, Timestamp);

	Message_SetID (&_MSG, (unsigned int) (server->TotalMessageNo++));
	server->Messages = _ll_new (server->Messages, sizeof (*server->Messages));
	*(server->Messages) = _MSG;
	Server_PrintMSG(server, _MSG);
	Server_SendMSG (server, _MSG);

}
void Server_PrintMSG(Server *server, Message _MSG)
{
	_MSG.Timestamp += 3600;
	int sec = _MSG.Timestamp % 86400;
	int stunden = sec / 3600;
	int min = sec % 3600 / 60;

	if (stunden < 10 && min < 10)
		printf_s("<%d%i:%d%i", 0, stunden, 0, min);

	if (stunden < 10 && min >= 10)
		printf_s("<%d%i:%i", 0, stunden, min);

	if (stunden >= 10 && min < 10)
		printf_s("<%i:%d%i", stunden, 0, min);

	if (stunden >= 10 && min >= 10)
		printf_s("<%i:%i", stunden, min);




	printf(" %s> %s\n", Message_GetName(&_MSG), Message_GetMsg(&_MSG));


}
void Server_SendMSG (struct Server* server, Message _MSG)
{

	sfPacket **Packet = sfPacket_create();
	sfPacket_clear(Packet);
	unsigned int iStatusCode = (unsigned int) (CLI_NEW_MSG);
	sfInt8 StatusCode = (sfInt8) (iStatusCode);

	sfPacket_writeInt8 (Packet, StatusCode);
	
	sfUint32 senderAddress = sfIpAddress_toInteger (Message_GetSenderAddress(&_MSG));

	long long TimeStamp = Message_GetTimestamp (&_MSG);

	// Convert timestamp (8 bytes) into two 4 byte chunks
	unsigned long hi, lo;
	memcpy(&lo, &TimeStamp, sizeof lo);
	memcpy(&hi, (char *) &TimeStamp + sizeof lo, sizeof hi);
	sfUint32 hi_ = (sfUint32) (hi);
	sfUint32 lo_ = (sfUint32) (lo);

	char StringMsg[1024];
	strcpy (StringMsg, Message_GetMsg(&_MSG));
	char StringName[1024];
	strcpy (StringName, Message_GetName(&_MSG));

	char name[4096], msg [4096];

	strcpy (msg, StringMsg);

	strcpy (name, StringName);

	sfInt32 ID = (sfInt32) (Message_GetID (&_MSG));

	sfPacket_writeInt32 (Packet, ID);
	sfPacket_writeString (Packet, msg);
	sfPacket_writeUint32 (Packet, senderAddress);
	sfPacket_writeString (Packet, name);
	sfPacket_writeUint32 (Packet, hi_);
	sfPacket_writeUint32 (Packet, lo_);

	if (sfUdpSocket_sendPacket(server->UdpSocket, Packet, sfIpAddress_Broadcast, server->ClientPort) == -1)
		printf ("Could not send new message to clients!\n");

	sfPacket_destroy(Packet);

}