server-sources=common.c server.c server-log-reg.c
server-headers=common.h server-routines.h
# -------------------------------------------------------------
client-sources=common.c client.c client-log-reg.c client-post.c
client-headers=common.h client-routines.h
# ---------------------------------------
usrf='users.list'
# ---------------

all: init
	@clear
	gcc $(server-sources) -o xserver -pthread -Werror=all -DPRINT_DEBUG
	gcc $(client-sources) -o xclient -pthread -Werror=all -DPRINT_DEBUG -DDEBUG_SERVER

simple:
	@clear
	gcc $(server-sources) -o xserver -pthread
	gcc $(client-sources) -o xclient -pthread

fine: setup
	@clear
	gcc $(server-sources) -o xserver -pthread -g -Werror=all -DPRINT_DEBUG -DPRINT_DEBUG_FINE
	gcc $(client-sources) -o xclient -pthread -g -Werror=all -DPRINT_DEBUG -DPRINT_DEBUG_FINE -DDEBUG_SERVER

init:
	echo '0000 anonymoususer0000000000000000000 00000000000000000000000000000000' > $(usrf)
	echo '0001 server00000000000000000000000000 00000000000000000000000000000000' >> $(usrf)
	echo '1000 startpointavaibleuserid000000000 00000000000000000000000000000000' >> $(usrf)

clear:
	rm -f xserver xclient $(usrf)

subl-all: subl-server subl-client
	subl Makefile

subl-server:
	subl $(server-sources) $(server-headers)

subl-client:
	subl $(client-sources) $(client-headers)