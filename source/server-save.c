#include "server-routines.h"

int save(int acceptfd, user_info *client_ui, operation *op){
	char *subject, *body;
	subject = op->text;

	if(receive_operation_from(acceptfd, op) < 0)
		return -1;

	body = op->text;

	if(client_ui->uid != op->uid){
		printf("client_ui->uid != op->uid, client_ui->uid=%d op->uid=%d\n", client_ui->uid, op->uid);
		return -1;
	}

	printf("BEGIN%s\n(%s, %d) sent:\n%s\n%s\n%s\n%s\n%sEND\n", SEP,
		client_ui->username, client_ui->uid, SEP,
		subject, SEP,
		body, SEP);

	free(subject);
	free(body);

	if(send_message_to(acceptfd, UID_SERVER, OP_OK, "0") < 0)
		return -1;

	return 0;
}