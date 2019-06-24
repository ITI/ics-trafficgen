dummy: 
	gcc -c -Wall -Werror -nodefaultlibs -fpic dummy.c
	gcc -shared -fPIC -o libdummy.so dummy.o

clean:
	rm -rf libdummy.so
	rm -rf dummy.o
