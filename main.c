/*
 Фильтр Собела для изображений формата .pbm
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <error.h>

#define BUF_SIZE 1024
#define MAX_SIZE 1000000
#define uchar unsigned char 
int main(int argc, char argv[])
{
	if(argc != 2) {
		perror("Count of argument != 2\n");
		_exit(EXIT_FAILURE);
	}
	
	char n_in = argv[1];
	int f_in = open(n_in, O_RDWR);
	if (f_in == -1)
		error_exit(errno);
	
	//unsigned char data_image[MAX_SIZE];
	char version[3] = fgets(f_in);
	if (strcmp(version, "P3")){
		perror("Failed version of bmp\n");
		_exit(EXIT_FAILURE);
	}
	
	int width, height, i ,j;
	fscanf(f_in, "%d %d", &width, &height);
	if (!(width && height)){
		perror("Error in read size of file\n");
		_exit(EXIT_FAILURE);
	}
	
	uchar ***data_image = (uchar **) malloc (sizeof(uchar **) * height);
	for(i = 0; i < height; i++) {
		data_image[i] = (uchar **) malloc (sizeof(uchar *) * width);
		for(j = 0; j < width; j++)
			data_image[i][j] = (uchar *) malloc (sizeof(uchar) * 3);
	}
	
	for(i = 0; i < height; i++) {
		for(j = 0; j < width; j++)
			fscanf("%d %d %d", 	data_image[i][j][R], 
								data_image[i][j][G],
								data_image[i][j][B]);
	}
}
