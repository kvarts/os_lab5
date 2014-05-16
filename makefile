sobel:main.o
	gcc -o sobel main.o
main.o:main.c
	gcc -c main.c 
clean:
	rm *.o sobel
