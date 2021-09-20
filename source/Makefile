ser='server'
cli='client'
cmnh='common-header'

all:
	@clear
	gcc $(ser).c -o x$(ser) -g -pthread -Werror=all
	gcc $(cli).c -o x$(cli) -g -pthread -Werror=all

print_debug_fine:
	@clear
	gcc $(ser).c -o x$(ser) -g -pthread -Werror=all -DPRINT_DEBUG_FINE
	gcc $(cli).c -o x$(cli) -g -pthread -Werror=all -DPRINT_DEBUG_FINE

setup:
	touch $(ser).c $(cli).c $(cmnh).h

clear:
	rm -f x$(ser) x$(cli)

subl:
	@subl Makefile $(ser).c $(cli).c $(cmnh).h