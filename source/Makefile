server-source=common.c server.c server-log-reg.c
client-source=common.c client.c client-log-reg.c client-post.c

usrf='users.list'

all: init
	@clear
	gcc $(server-source) -o xserver -pthread -Werror=all -DPRINT_DEBUG
	gcc $(client-source) -o xclient -pthread -Werror=all -DPRINT_DEBUG -DDEBUG_SERVER

simple:
	@clear
	gcc $(server-source) -o xserver -pthread
	gcc $(client-source) -o xclient -pthread

fine: setup
	@clear
	gcc $(server-source) -o xserver -pthread -g -Werror=all -DPRINT_DEBUG -DPRINT_DEBUG_FINE
	gcc $(client-source) -o xclient -pthread -g -Werror=all -DPRINT_DEBUG -DPRINT_DEBUG_FINE -DDEBUG_SERVER

init:
	echo '0000 anonymoususer0000000000000000000 00000000000000000000000000000000' > $(usrf)
	echo '0001 server00000000000000000000000000 00000000000000000000000000000000' >> $(usrf)
	echo '1000 startpointavaibleuserid000000000 00000000000000000000000000000000' >> $(usrf)

clear:
	rm -f xserver xclient $(usrf)

subl-all:
	subl Makefile common.h server* client*

subl-server:
	subl common.h server*

subl-client:
	subl common.h client*