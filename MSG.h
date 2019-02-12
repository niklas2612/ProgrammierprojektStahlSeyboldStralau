#ifndef MSG_H
#define MSG_H

#include <SFML\Network.h>
#include <string.h>
#include <stdbool.h>

typedef struct Message
{

	char Msg[1024], Name[1024];
	sfIpAddress SenderAddress;
	long long Timestamp;
	unsigned int ID;


} Message;

void Message_Message (Message *message, char msg[1024], const sfIpAddress senderAddress,const char name[1024], const long long timestamp)
{

	strcpy (message->Msg, msg);
	message->SenderAddress = senderAddress;
	strcpy (message->Name, name);
	
	message->SenderAddress = senderAddress;

	message->Timestamp = timestamp;


	message->ID = 0;

}

const char* Message_GetMsg (Message *message) {return message->Msg;};
const sfIpAddress Message_GetSenderAddress (Message *message) {return message->SenderAddress;};
const char* Message_GetName (Message *message) {return message->Name;};
const long long Message_GetTimestamp (Message *message) {return message->Timestamp;};

void Message_SetID (Message *message, const unsigned int id) {message->ID = id;};
const unsigned int Message_GetID (Message *message) {return message->ID;};

#endif