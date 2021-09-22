ser='server'
ser-reg='server-registration'
cli='client'
cli-reg='client-registration'
cmn='common-header'

all:
	@clear
	gcc $(ser).c -o x$(ser) -g -pthread -Werror=all -DPRINT_DEBUG
	gcc $(cli).c -o x$(cli) -g -pthread -Werror=all -DPRINT_DEBUG -DDEBUG_SERVER

simple:
	@clear
	gcc $(ser).c -o x$(ser) -pthread
	gcc $(cli).c -o x$(cli) -pthread

fine:
	@clear
	gcc $(ser).c -o x$(ser) -g -pthread -Werror=all -DPRINT_DEBUG -DPRINT_DEBUG_FINE
	gcc $(cli).c -o x$(cli) -g -pthread -Werror=all -DPRINT_DEBUG -DPRINT_DEBUG_FINE -DDEBUG_SERVER

clear:
	rm -f x$(ser) x$(cli)

subl:
	subl Makefile $(ser).c $(cli).c $(cmn).h $(cli-reg).h $(ser-reg).h