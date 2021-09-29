#include "server-routines.h"

int read_response(int acceptfd, user_info client_ui){

	printf("(%s, %d): read request.\n", client_ui.username, client_ui.uid);

	if(send_message_to(acceptfd, UID_SERVER, OP_OK, NULL) < 0)
		return -1;

	return 0;
}