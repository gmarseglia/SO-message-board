#include "common-header.h"

#define PRINT_DEBUG
#define MAX_BACKLOG 5

int main(int argc, char const *argv[])
{
	printf("Server active.\n");

	char buffer[BUFFER_SIZE];
	int ds_sock, ds_sock_accept;
	int byteRead;

	if((ds_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){							// Creo la socket che usa TCP
		perror("Error in server on socket creation attempt.\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	// #TODO: handle port already used
	// Solved different port number:
	// https://www.geeksforgeeks.org/explicitly-assigning-port-number-client-socket/
	addr.sin_port = htons(SERV_PORT);												// Uso come porta SERV_PORT in common-header.h = 6990
	addr.sin_addr.s_addr = INADDR_ANY;												// Accetto connessioni da tutti 

	if(bind(ds_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){				// Faccio il bind
		perror("Error in server on bind attempt.\n");
		exit(EXIT_FAILURE);
	}

	#ifdef PRINT_DEBUG
	printf("Server bound on %s:%d\n", inet_ntoa(addr.sin_addr), SERV_PORT);
	#endif

	if(listen(ds_sock, MAX_BACKLOG) < 0){											// Mi metto in listen
		perror("Error in server on listen attempt.\n");
		exit(EXIT_FAILURE);
	}

	#ifdef PRINT_DEBUG
	printf("Server is on LISTEN\n");
	#endif

	struct sockaddr_in client_addr;
	socklen_t client_addr_len;
	ds_sock_accept = accept(ds_sock, (struct sockaddr *)&client_addr, &client_addr_len);				// Accetto una connessione da client
	if(ds_sock_accept < 0){
		perror("Error in server on accept attempt.\n");
		exit(EXIT_FAILURE);
	}

	if(close(ds_sock) < 0) 
		perror("Not exiting error in server on close socker attempt\n");																					// Chiudo il socket per le connessioni

	printf("Server accepted connesion from %s\n", inet_ntoa(client_addr.sin_addr));

	do{
		memset(buffer, '\0', BUFFER_SIZE);
		byteRead = read(ds_sock_accept, buffer, BUFFER_SIZE);
		printf("%s", buffer);
	} while(byteRead == BUFFER_SIZE);
	printf("\n");

	if(close(ds_sock_accept) < 0 && errno != EBADF) perror("Error in server on close accepted socket\n");

	return 0;
}