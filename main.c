#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "Server.h"
#include "Client.h"

int main ()
{

	enum eState {eNone,eClient,eServer,eExit} State = eNone;
	char UserInput = ' ';
	bool FirstTime = true;

	printf_s ("Do you want to chat (c) or host a server (h)? Enter (e) to exit.\n");

	do
	{
	
		if (!FirstTime)
			printf_s ("Invalid answer!\n");

		UserInput = ' ';

		scanf (" %c", &UserInput);

	
		if (UserInput == 'c' || UserInput == 'C')
			State = eClient;

		if (UserInput == 'h' || UserInput == 'H')
			State = eServer;

		if (UserInput == 'e' || UserInput == 'E')
			State = eExit;


		FirstTime = false;

	} while (State==eNone);

	if (State == eClient)
	{
		
		printf_s ("Enter your name: ");

		char UserInput[1024];

		scanf_s (" %1024[^\n]", UserInput, sizeof (UserInput));

		Client client;
		Client_Client (&client, UserInput);
		Client_Loop (&client);

	}

	if (State == eServer)
	{

		Server server;
		Server_Server(&server);
		Server_Loop (&server);

	}

	return 0;

}