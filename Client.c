#include "Client.h"

void PrintLine (const short ConsoleWidth)
{

	printf ("+");
	for (int i = 0; i<ConsoleWidth-2; i++)
		printf ("-");
	printf ("+\n");

}

void Client_Client (Client *client, char name[1024])
{

	strcpy (client->Name, name);
	client->ServerPort = 55001;
	client->ClientPort = 55002;
	client->RunClient = true;
	client->IpAddress = sfIpAddress_getLocalAddress ();

	client->UdpSocket = sfUdpSocket_create();
	client->TcpSocket = sfTcpSocket_create();

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE) , &client->csbi);
	client->ConsoleWidth = client->csbi.dwSize.X;
	client->ConsoleHeight = client->csbi.dwSize.X;

	sfUdpSocket_bind(client->UdpSocket, client->ClientPort, sfIpAddress_Any);
	sfUdpSocket_setBlocking(client->UdpSocket, true);

	client->Messages = NULL;

	Client_ConnectToServer (client);


	client->RunClientMtx = CreateMutex (NULL, FALSE, NULL);

	if (client->RunClientMtx == NULL)
		 printf("CreateMutex error: %d\n", GetLastError());

	client->TypingThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Client_TypingLoop, client, 0, NULL);
}

void Client_Destroy(Client *client)
{

	sfUdpSocket_destroy(client->UdpSocket);
	sfTcpSocket_destroy(client->TcpSocket);

}

bool Client_RegisterMainInfo (Client *client, sfPacket **Packet, sfInt16 CheckCode, sfInt32 *pNumberOfSegInfoPackets)
{

	sfInt16  RcvCheckCode = -1, clientNo = -1;
	sfUint32 uiOwnAddress = -1;

	clientNo = sfPacket_readInt16 (Packet);
	uiOwnAddress = sfPacket_readUint32 (Packet);

	(*pNumberOfSegInfoPackets) = sfPacket_readInt32 (Packet);

	RcvCheckCode = sfPacket_readInt16 (Packet);

	sfIpAddress OwnAddress = sfIpAddress_fromInteger(uiOwnAddress);

	if (sfIpAddress_toInteger (OwnAddress) == sfIpAddress_toInteger (client->IpAddress) && RcvCheckCode == CheckCode)
	{
		
		client->ClientNo = clientNo;
		return true;

	}
	else
		return false;

}

void Client_RegisterMSGInfo (Client *client, sfPacket **Packet)
{

	sfInt32 numberOfMSG = -1;
	numberOfMSG = sfPacket_readInt32 (Packet);
	const int NumberOfMSG = (int) (numberOfMSG);

	for (int i = 0; i < NumberOfMSG; i++)
	{

		char msg[4096], name[4096];
		sfUint32 senderAddress;
		sfUint32 hi_,lo_;
		sfInt32 id;
	
		long long Timestamp;
		unsigned int hi,lo;

		id = sfPacket_readInt32 (Packet);
		sfPacket_readString (Packet, msg);
		senderAddress = sfPacket_readUint32 (Packet);
		sfPacket_readString (Packet, name);
		hi_ = sfPacket_readUint32 (Packet);
		lo_ = sfPacket_readUint32 (Packet);

		sfIpAddress SenderAddress = sfIpAddress_fromInteger(senderAddress);
		unsigned int ID = (unsigned int) (id);

		hi = (unsigned long) (hi_);
		lo = (unsigned long) (lo_);

		// Copying hi & lo variables into timestamp
		memcpy (&Timestamp, &lo, sizeof lo);
		memcpy ((char *) &Timestamp + sizeof lo, &hi, sizeof hi);

		Message _MSG;
		Message_Message (&_MSG, msg, SenderAddress, name, Timestamp);

		Message_SetID (&_MSG, ID);
// linked list to show old messages
		client->Messages = _ll_new (client->Messages, sizeof (*client->Messages));
		*(client->Messages) = _MSG;

	}

}

void Client_ConnectToServer (Client *client)
{

	srand ((unsigned int) (time(NULL)));  //checkcode
	sfInt16 CheckCode = rand()%30000;
	
	bool Connected = false;
	bool Sent = false;

	bool ReceivedMainInfo = false, ReceivedExistingMSGInfo = false;
	sfInt32 NumberOfMSGInfoPackets = 0;
	sfInt32 NumberOfReceivedMSGInfoPackets = 0;

	do
	{
	
		sfIpAddress UserServerAddress;
		bool FirstTime = true;

		char UserInput [1024]= "";
	
		if (!FirstTime)
			printf ("Invalid answer!\n");

		printf ("Please enter the server IP: ");

		FirstTime = false;

		scanf_s ("%s", UserInput, sizeof (UserInput));

			UserServerAddress = sfIpAddress_fromString (UserInput);

			client->ServerIpAddress = UserServerAddress; 

			sfPacket **Packet = sfPacket_create ();
			int iStatusCode = SER_CLIENTCONNECT;
			sfInt8 StatusCode = (sfInt8) (iStatusCode);

			char name[4096];
			
			strcpy (name, client->Name);

			sfPacket_writeInt8 (Packet, StatusCode);
			sfPacket_writeString (Packet, name);
			sfPacket_writeInt16 (Packet, CheckCode);

			if (sfUdpSocket_sendPacket(client->UdpSocket, Packet, UserServerAddress, client->ServerPort) != -1)
				Sent = true;

			sfPacket_destroy(Packet);

		
	
	} while (!Sent);

	system ("cls");


	do 
	{

		sfInt8 packetCode = -1;

		sfIpAddress Sender = sfIpAddress_None;
		unsigned short SenderPort = 0;
		sfPacket **Packet = sfPacket_create();
		
		sfUdpSocket_receivePacket(client->UdpSocket, Packet, &client->ServerIpAddress, &client->ServerPort);

		packetCode = sfPacket_readInt8 (Packet);

		ePacketCodes PacketCode = (ePacketCodes) (packetCode);

		switch (PacketCode)
		{

		case CLI_IND_CLIENTCONNECT:
			if (Client_RegisterMainInfo (client, Packet, CheckCode, &NumberOfMSGInfoPackets))
			{
				ReceivedMainInfo = true;
				if (NumberOfMSGInfoPackets == 0)
					ReceivedExistingMSGInfo = true;
			}
			else
			{
				printf ("Error reading connected client information!\n");
				return;
			}
			break;

		case CLI_IND_MSGINFO:
			Client_RegisterMSGInfo (client, Packet);
			NumberOfReceivedMSGInfoPackets++;
			
			
			break;

		default:
			printf("Read packet with code %i! (d)\n", packetCode);
			break;

		};

		if ((NumberOfReceivedMSGInfoPackets > 0) && ReceivedMainInfo)
		{
			ReceivedExistingMSGInfo = true;
			
		}

		if (ReceivedMainInfo&&ReceivedExistingMSGInfo)
			Connected = true;

		sfPacket_destroy(Packet);

	} while (!Connected);

	char serverIpAddressString[1024];
	sfIpAddress_toString (client->ServerIpAddress, serverIpAddressString);

	printf ("Successfully connected to Server@%s:%i!\n", serverIpAddressString, client->ServerPort);

	PrintLine (client->ConsoleWidth);
	PrintLine (client->ConsoleWidth);
	PrintLine (client->ConsoleWidth);
	
	if (client->Messages != NULL)
	{
		for (Message *it_msg = client->Messages; it_msg; it_msg = _ll_next(it_msg))
			Client_PrintMSG(client, *it_msg);
	}

}

void Client_Loop (Client *client)
{

	while (Client_GetRunClient(client))
	{
	
		sfInt8 packetCode = -1;

		sfIpAddress Sender = sfIpAddress_None;
		unsigned short SenderPort = 0;
		sfPacket **Packet = sfPacket_create();

		sfUdpSocket_receivePacket(client->UdpSocket, Packet, &Sender, &SenderPort);

		packetCode = sfPacket_readInt8 (Packet);

		ePacketCodes PacketCode = (ePacketCodes) (packetCode);

		switch (PacketCode)
		{
		
		// Error
		default:
			char string[1024];
			sfIpAddress_toString (Sender, string);
			printf ("Packet from %s:%i couldn't be read!\n", string, SenderPort);
			break;

		case CLI_NEW_MSG:
			Client_RegisterNewMSG (client, Packet);
			break;

		case CLI_CLIENTCONNECT:
			Client_RegisterNewClient (client, Packet);
			break;

		};

		sfPacket_destroy(Packet);

	}

	CloseHandle (client->TypingThread);

}

void Client_RegisterNewMSG (Client *client, sfPacket **Packet)
{

	sfUint32 senderAddress;
	sfUint32 hi_, lo_;
	sfInt32 id;
	char msg[4096], name[4096];
	
	long long Timestamp;
	unsigned int hi,lo;

	id = sfPacket_readInt32 (Packet);

	bool NoCopy = true;

	unsigned int test = 0;

	for (Message *it_msg = client->Messages; it_msg; it_msg = _ll_next(it_msg))
		if ((test = Message_GetID (it_msg)) == id)
			NoCopy = false;

	if (NoCopy)
	{

		sfPacket_readString (Packet, msg);
		senderAddress = sfPacket_readUint32 (Packet);
		sfPacket_readString (Packet, name);
		hi_ = sfPacket_readUint32 (Packet);
		lo_ = sfPacket_readUint32 (Packet);

		sfIpAddress SenderAddress =	sfIpAddress_fromInteger (senderAddress); 

		hi = (unsigned long) (hi_);
		lo = (unsigned long) (lo_);

		// Copying hi & lo variables into timestamp
		memcpy (&Timestamp, &lo, sizeof lo);
		memcpy ((char *) &Timestamp + sizeof lo, &hi, sizeof hi);
		
		Message _MSG;
		Message_Message (&_MSG, msg, SenderAddress, name, Timestamp);

		Message_SetID (&_MSG, (unsigned int) id);

		client->Messages = _ll_new (client->Messages, sizeof (*client->Messages));
		*(client->Messages) = _MSG;
		
		Client_PrintMSG (client, _MSG);

	}

}

void Client_RegisterNewClient (Client *client, sfPacket **Packet)
{

	sfInt16 clientNo = -1;
	sfUint32 uiNewPlayerAddress = -1;
	char name[4096];

	clientNo = sfPacket_readInt16 (Packet);
	uiNewPlayerAddress = sfPacket_readUint32 (Packet);
	sfPacket_readString (Packet, name);

	sfIpAddress NewPlayerAddress = sfIpAddress_fromInteger (uiNewPlayerAddress);

	if (clientNo != client->ClientNo)
	{
		char string[1024];
		sfIpAddress_toString ( NewPlayerAddress, string);
		printf ("%s (%s) has connected to your chat!\n", name, string);

	}

}

void Client_PrintMSG (Client *client, Message _MSG)
{
	_MSG.Timestamp += 3600;  // convert timezone
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

// -------- typing thread --------------------------------------------------------------------------------------------------------------------------

void Client_TypingLoop (Client *client)
{

	fseek(stdin,0,SEEK_END); // ??
	
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

	while (Client_GetRunClient(client))
	{
	
		char UserInput[1024];

		scanf (" %1024[^\n]", &UserInput);

		if (UserInput == "/exit")
			Client_SetRunClient (client, false);
		else
		{

			Message _MSG;
			Message_Message (&_MSG, UserInput, client->IpAddress, client->Name, (unsigned)time(NULL));
			Client_SendNewMSG (client, _MSG);
		

		}

	}

}
	
void Client_SendNewMSG (Client *client, Message _MSG)
{

	sfPacket **Packet = sfPacket_create();
	unsigned int iStatusCode = (unsigned int) (SER_NEW_MSG);
	sfInt8 StatusCode = (sfInt8) (iStatusCode);

	sfPacket_writeInt8 (Packet, StatusCode);

	sfUint32 senderAddress = (sfUint32) sfIpAddress_toInteger (Message_GetSenderAddress(&_MSG));

	long long TimeStamp = Message_GetTimestamp (&_MSG);

	// Convert timestamp (8 bytes) into two 4 byte chunks
	unsigned long hi, lo;
	memcpy(&lo, &TimeStamp, sizeof lo);
	memcpy(&hi, (char *) &TimeStamp + sizeof lo, sizeof hi);
	sfUint32 hi_ = (sfUint32) (hi);
	sfUint32 lo_ = (sfUint32) (lo);

	char StringMsg[1024];
	strcpy (StringMsg, Message_GetMsg (&_MSG));
	char StringName[1024];
	strcpy (StringName, Message_GetName (&_MSG));

	char name[4096], msg [4096];

	strcpy (msg, StringMsg);
	strcpy (name, StringName);

	sfPacket_writeString (Packet, msg);
	sfPacket_writeUint32 (Packet, senderAddress);
	sfPacket_writeString (Packet, name);
	sfPacket_writeUint32 (Packet, hi_);
	sfPacket_writeUint32 (Packet, lo_);

	if (sfUdpSocket_sendPacket (client->UdpSocket, Packet, client->ServerIpAddress, client->ServerPort) == -1)
		printf ("Could not send new segment data to server!\n");

	sfPacket_destroy(Packet);

}