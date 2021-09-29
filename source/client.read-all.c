#include "client-routines.h"

int save(int sockfd, user_info client_ui){
	operation op;

	if(send_message_to(sockfd, client_ui.uid, OP_READ_REQUEST, NULL) < 0)
		return -1;

	while(receive_operation_from(sockfd, &op) == 0){
		print_operation(&op);
		if(op.code == OP_OK || op.code == OP_NOT_ACCEPTED)
			break;
	}

	return 0;
}
