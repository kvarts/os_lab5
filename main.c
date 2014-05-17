/*
 Фильтр Собела для изображений формата .pbm
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include "error_handle.h"

#define R 0
#define G 1
#define B 2
#define BUF_SIZE 1024
#define MAX_SIZE 1000000
#define uchar unsigned char 
#define pixel(i,j,k) *(data_image+(width*3*(i))+(j)*3+k)
#define pixel_new(i,j,k) *(data_new_image+(width*3*(i))+(j)*3+k)
int rows_for_core, width, height, max_color;
unsigned char *data_image, *data_new_image;
pthread_mutex_t mutex_image;

void *sobel_func(void *arg)
{
	int start_row = *(int *) arg;
	short Gx,Gy,Gres;
	int i, j, color;
	for(i = start_row; i < start_row + rows_for_core; i++) {
		if (i == 0) continue;
		if (i == height-1) break;
		for(j = 1; j < width - 1; j++) {
			for (color = R; color <= B; color++) {
				Gx =(	pixel(i-1, j+1, color) +
					2 * pixel(i-1, j,   color) +
						pixel(i-1, j-1, color) ) -
					(	pixel(i+1, j+1, color) +
					2 * pixel(i+1, j,   color) +
						pixel(i+1, j-1, color) );

				Gy =(	pixel(i-1, j+1, color) +
					2 * pixel(i, j+1,   color) +
						pixel(i+1, j+1, color) ) -
					(	pixel(i-1, j-1, color) +
					2 * pixel(i, j-1,   color) +
						pixel(i+1, j-1, color) );

				Gres = sqrt(Gx*Gx+Gy*Gy);
				Gres = Gres < 0? 0: Gres;
				Gres = Gres > 255? 255: Gres;
				
				pthread_mutex_lock(&mutex_image);
				//printf("[debug]: было %d\n", pixel(i, j, color));
				pixel_new(i, j, color) = (unsigned char)Gres;
				//printf("[debug]: стало %d\n", pixel(i, j, color));
				pthread_mutex_unlock(&mutex_image);
			}
			//printf("[debug]: Новый пиксель номер %d\n", i*j);
		}	
		printf("[debug]: Строка %d\n", i	);
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	if(argc != 2)
		error_exit(OPT_ERROR);

	char name_image[255];
	strcpy(name_image, argv[1]);
	int handle_image;
	handle_image = open(name_image, O_RDONLY);
	if (handle_image == 0)
		error_exit(FILE_NOT_OPEN);
	else
		printf("[debug]: Открыт файл\n");

	struct stat buf_stat;
	if (fstat(handle_image, &buf_stat) == -1)
		error_exit(FSTAT);
	printf("[debug]: Размер изображения = %ld kb\n", buf_stat.st_size/1024);

	data_image = calloc(buf_stat.st_size, 1);
	data_new_image = calloc(buf_stat.st_size, 1);
	if (data_image == 0 || data_new_image == 0)
		return error_exit(MEM_ERROR);
	else
		printf("[debug]: Выделена память\n");
		
	int cnt_read = read(handle_image, data_image, buf_stat.st_size);
	if (cnt_read != buf_stat.st_size)
		error_exit(READ_CNT);
	
	char version;
	int cursor, count_read;
	count_read = 0;
	sscanf(data_image, "P%c\n%n", &version, &cursor);
	printf("[debug]: Версия ррм - P%c\n", version);
	printf("[debug]: Считано %d байт\n", cursor);
	memcpy(data_new_image, data_image, cursor);
	data_image += cursor;
	//data_new_image += cursor;

	
	if (version != '6')
		error_exit(VERSION_PPM);
	
	/*unsigned char *token;
	token = strtok(data_image, "\n");
	while( token != NULL ) {
		if (token[0] != '#')
			break;
		token = strtok(NULL, "\n");
	}
	
	data_image = token;*/
	
	
	sscanf(data_image, "%d %d\n%n", &width, &height, &cursor);
	printf("[debug]: Размер изображения - %dx%d px\n", width, height);
	printf("[debug]: Считано еще %d байт\n", cursor);
	memcpy(data_new_image, data_image, cursor);
	data_image += cursor;
	//data_new_image += cursor;
	
	
	sscanf(data_image, "%d\n%n", &max_color, &cursor);
	printf("[debug]: Максимальное значение цвета - %d\n", max_color);
	printf("[debug]: Считано еще %d байт\n", cursor);
	memcpy(data_new_image, data_image, cursor);
	data_image += cursor;
	//data_new_image += cursor;
	
	if (!(width && height))
		error_exit(SIZE_INCORRECT);
	
	//unsigned char pixel[height][width][3];
	
	char cores;
	cores = sysconf(_SC_NPROCESSORS_ONLN);
	printf("Количество ядер процессора:\t%d\n", cores);

	rows_for_core = height / cores;
	if (height % cores != 0)
		rows_for_core += 1;
	
	int start_points[cores], i;
	pthread_t id[cores];
	pthread_mutex_init(&mutex_image, NULL);
	for (i = 0; i < cores; i++) {
		start_points[i] = i * rows_for_core;
		if(pthread_create(&id[i], NULL, sobel_func, &start_points[i]))
			return error_exit(PTHREAD_ERROR);
		//printf("[debug]: Создан поток с id = %d\n", (int) id[i]);
	}

	for (i=0; i<cores; i++) {
		//printf("[debug]: Ожидается поток с id = %d\n", (int) id[i]);
		pthread_join(id[i], NULL);
		//printf("[debug]: Завершился поток с id = %d\n", (int) id[i]);
	}

	char name_new_image[255] = "test_result.ppm";
	int handle_new_image;
	handle_new_image = creat(name_new_image, 0777);
	if (handle_new_image == 0)
		error_exit(FILE_NOT_OPEN);
	else
		printf("[debug]: Открыт файл\n");
	
	unsigned char buf[20];
	sprintf(buf, "P%c\n%d %d\n%d\n\0", 
			version, width, height, max_color, &cursor);
	write(handle_new_image, buf, 20);	
	write(handle_new_image, data_new_image, width * height * 3);
	close(handle_new_image);
	_exit(EXIT_SUCCESS);
 }


