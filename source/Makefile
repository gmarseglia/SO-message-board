ser='server'
cli='client'
cmnh='common-header'

all:
	@clear
	gcc $(ser).c -o x$(ser) -g -pthread -Werror=all
	gcc $(cli).c -o x$(cli) -g -pthread -Werror=all

setup:
	touch $(ser).c $(cli).c $(cmnh).h

clear:
	rm -f x$(ser) x$(cli)