all:
	gcc -O2 -o 3dengine main.c -lm
clean:
	rm -rf *.o
	rm -rf 3dengine
