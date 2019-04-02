rci: rmb.c clientudpRMB.c msgserv.c udp.c messages.c tcp.c
	gcc -o rmb -Wall rmb.c clientudpRMB.c
	gcc -o msgserv -Wall msgserv.c udp.c messages.c tcp.c

clean:
	rm rmb msgserv