com-dir=common/
ser-dir=server/
cli-dir=client/
run-dir=runnable/

server-sources=$(com-dir)common.c $(ser-dir)server-main.c $(ser-dir)server-thread.c \
	$(ser-dir)server-log-reg.c $(ser-dir)server-operations.c $(com-dir)pthread-bitmask.c
server-headers=$(com-dir)/common.h $(ser-dir)/server.h
# -------------------------------------------------------------
client-sources=$(com-dir)common.c $(cli-dir)client-main.c $(cli-dir)client-log-reg.c $(cli-dir)client-operations.c
client-headers=$(com-dir)common.h $(cli-dir)client.h
# ---------------------------------------
# --------------
usrf=$(run-dir)users.list
msgf=$(run-dir)index.list $(run-dir)messages.list $(run-dir)free-areas.list

all: init
	@clear
	gcc $(server-sources) -o $(run-dir)xserver -pthread -Werror=all -g -DPRINT_DEBUG
	gcc $(client-sources) -o $(run-dir)xclient -pthread -Werror=all -g -DPRINT_DEBUG -DDEBUG_SERVER

simple: init
	@clear
	gcc $(server-sources) -o $(run-dir)xserver -pthread
	gcc $(client-sources) -o $(run-dir)xclient -pthread

fine: init
	@clear
	gcc $(server-sources) -o $(run-dir)xserver -pthread -g -Werror=all -DPRINT_DEBUG -DPRINT_DEBUG_FINE
	gcc $(client-sources) -o $(run-dir)xclient -pthread -g -Werror=all -DPRINT_DEBUG -DPRINT_DEBUG_FINE -DDEBUG_SERVER

init:
	if [ ! -d $(run-dir) ]; then mkdir $(run-dir); fi

clear:
	rm -f $(run-dir)xserver $(run-dir)xclient $(usrf) $(msgf)
	if [ -d $(run-dir) ]; then rmdir $(run-dir); fi
	clear

subl-all: subl-server subl-client
	subl Makefile

subl-server:
	subl $(server-headers) $(server-sources)

subl-client:
	subl $(client-headers) $(client-sources)