server-sources=common/common.c server/server-main.c server/server-thread.c server/server-log-reg.c server/server-operations.c common/pthread-bitmask.c
server-headers=common/common.h server/server.h
# -------------------------------------------------------------
client-sources=common/common.c client/client-main.c client/client-log-reg.c client/client-operations.c
client-headers=common/common.h client/client.h
# ---------------------------------------
rundir=runnable/
# --------------
usrf=$(rundir)users.list
msgf=$(rundir)index.list $(rundir)messages.list $(rundir)free-areas.list

all: init
	@clear
	gcc $(server-sources) -o $(rundir)xserver -pthread -Werror=all -g -DPRINT_DEBUG
	gcc $(client-sources) -o $(rundir)xclient -pthread -Werror=all -g -DPRINT_DEBUG -DDEBUG_SERVER

simple: init
	@clear
	gcc $(server-sources) -o $(rundir)xserver -pthread
	gcc $(client-sources) -o $(rundir)xclient -pthread

fine: init
	@clear
	gcc $(server-sources) -o $(rundir)xserver -pthread -g -Werror=all -DPRINT_DEBUG -DPRINT_DEBUG_FINE
	gcc $(client-sources) -o $(rundir)xclient -pthread -g -Werror=all -DPRINT_DEBUG -DPRINT_DEBUG_FINE -DDEBUG_SERVER

init:
	if [ ! -d $(rundir) ]; then mkdir $(rundir); fi

clear:
	rm -f $(rundir)xserver $(rundir)xclient $(usrf) $(msgf)
	if [ -d $(rundir) ]; then rmdir $(rundir); fi
	clear

subl-all: subl-server subl-client
	subl Makefile

subl-server:
	subl $(server-headers) $(server-sources)

subl-client:
	subl $(client-headers) $(client-sources)