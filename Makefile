all: popsim

popsim: popsim.c popsim.h popa_misc.h
	gcc popsim.c -o popsim -lgd -g


clean:
	rm popsim

