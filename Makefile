all: recv gf_rsc

clean:
	rm -f recv, gf_rsc

recv: recv.c
	gcc -I include/ $? -o $@

gf_rsc: gf_rsc.c include/gf256.c
	gcc -I include $? -o $@
