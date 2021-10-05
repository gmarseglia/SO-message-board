#include "client.h"

int delete_post(int sockfd, user_info client_ui){
	int target_mid;
	char target_mid_str[32];

	operation op;

	// #1: Ask user which post to delete
	printf("Which post do you want to delete?\n");
	while(scanf("%d", &target_mid) == 0)
		fflush(stdin);

	// #2: Send delete request to server
	printf("Asking server to delete post #%d\n", target_mid);
	sprintf(target_mid_str, "%d", target_mid);

	if(send_message_to(sockfd, client_ui.uid, OP_DELETE_REQUEST, target_mid_str) < 0)
		return -1;

	// #3: Wait server response
	if(receive_operation_from(sockfd, &op) < 0)
		return -1;

	// #4: Check operation result
	if(op.uid == UID_SERVER && op.code == OP_OK){
		printf("Post #%d deleted successfully.\n\n", target_mid);
		return 0;
	} else if (op.uid == UID_SERVER && op.code == OP_NOT_ACCEPTED){
		printf("Post #%d not deleted: %s.\n\n", target_mid, op.text);
		free(op.text);
		return 0;
	} else {
		fprintf(stderr, "Unknown Error in Client on delete_post()\n");
		print_operation(&op);
		free(op.text);
		return -1;
	}
}