sobel:main.o
	gcc -o sobel main.o -lpthread -lm
main.o:main.c error_handle.h
	gcc -c main.c 
clean:
	rm *.o sobel
