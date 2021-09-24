ser='server'
ser-reg='server-registration'
cli='client'
cli-reg='client-registration'
cmn='common-header'
usrf='users.list'

all: setup
	@clear
	gcc $(ser).c -o x$(ser) -g -pthread -Werror=all -DPRINT_DEBUG
	gcc $(cli).c -o x$(cli) -g -pthread -Werror=all -DPRINT_DEBUG -DDEBUG_SERVER

simple:
	@clear
	gcc $(ser).c -o x$(ser) -pthread
	gcc $(cli).c -o x$(cli) -pthread

fine: setup
	@clear
	gcc $(ser).c -o x$(ser) -g -pthread -Werror=all -DPRINT_DEBUG -DPRINT_DEBUG_FINE
	gcc $(cli).c -o x$(cli) -g -pthread -Werror=all -DPRINT_DEBUG -DPRINT_DEBUG_FINE -DDEBUG_SERVER

setup:
	echo '0000 anonymoususer0000000000000000000 00000000000000000000000000000000' > $(usrf)
	echo '0001 server00000000000000000000000000 00000000000000000000000000000000' >> $(usrf)
	echo '1000 startpointavaibleuserid000000000 00000000000000000000000000000000' >> $(usrf)

clear:
	rm -f x$(ser) x$(cli) $(usrf)

subl:
	subl Makefile $(ser).c $(cli).c $(cmn).h $(cli-reg).h $(ser-reg).h