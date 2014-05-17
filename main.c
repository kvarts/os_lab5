/*
 Фильтр Собела для изображений формата .ppm
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
#include <stdint.h>

#define R 0
#define G 1
#define B 2
#define pixel(i,j,k) *(data_image+(width*3*(i))+(j)*3+k)
#define pixel_new(i,j,k) *(data_new_image+(width*3*(i))+(j)*3+k)

#define debug 0

int rows_for_core, width, height, max_color;
unsigned char *data_image, *data_new_image;
pthread_mutex_t mutex_image;

void *sobel_func(void *arg)
{
	int start_row = *(int *) arg;
	short Gx,Gy,Gres;
	int i, j, color;
	for (i = start_row; i < start_row + rows_for_core; i++) {
		if (i == 0) continue;
		if (i == height-1) break;
		for (j = 1; j < width - 1; j++) {
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

				//pthread_mutex_lock(&mutex_image);
				pixel_new(i, j, color) = (unsigned char)Gres;
				//pthread_mutex_unlock(&mutex_image);
			}
		}	
	}
	pthread_exit(NULL);
}

uint64_t rdtsc()
{
	uint64_t x;
	__asm__ volatile ("rdtsc\n\tshl $32, %%rdx\n\tor %%rdx, %%rax" : "=a" (x) : : "rdx");
	return x;
}

int main(int argc, char *argv[])
{
	if(argc != 2 && argc != 3)
		error_exit(OPT_ERROR);
	
	char name_image[255];
	strcpy(name_image, argv[1]);
	int handle_image;
	handle_image = open(name_image, O_RDONLY);
	if (handle_image == 0)
		error_exit(FILE_NOT_OPEN);
	else if (debug)
		printf("[debug]: Открыт файл\n");

	struct stat buf_stat;
	if (fstat(handle_image, &buf_stat) == -1)
		error_exit(FSTAT);
	if (debug)
		printf("[debug]: Размер изображения = %ld kb\n", 
				buf_stat.st_size / 1024);

	data_image = calloc(buf_stat.st_size, 1);
	data_new_image = calloc(buf_stat.st_size, 1);
	if (data_image == 0 || data_new_image == 0)
		return error_exit(MEM_ERROR);
	else if (debug)
		printf("[debug]: Выделена память\n");
	
	int cnt_read = read(handle_image, data_image, buf_stat.st_size);
	if (cnt_read != buf_stat.st_size)
		error_exit(READ_CNT);

	char version;
	int cursor, count_read;
	count_read = 0;
	sscanf(data_image, "P%c\n%n", &version, &cursor);
	if (debug) {
		printf("[debug]: Версия ррм - P%c\n", version);
		printf("[debug]: Считано %d байт\n", cursor);
	}
	memcpy(data_new_image, data_image, cursor);
	data_image += cursor;
	data_new_image += cursor;

	
	if (version != '6')
		error_exit(VERSION_PPM);

	/*
	printf("++ %s\n", data_image);
	unsigned char *token;
	token = strtok(data_image, "\n");
	printf("++ %s\n", token);
	if (token[0] == '#') {
		while( token != NULL ) {
			if (token[0] != '#')
				break;
			token = strtok(NULL, "\n");
			printf("++ %s\n", token);
		}
	}
	data_image = token;
	printf("++ %s\n", data_image);	
	*/

	sscanf(data_image, "%d %d\n%n", &width, &height, &cursor);
	if (debug) {
		printf("[debug]: Размер изображения - %dx%d px\n", width, height);
		printf("[debug]: Считано еще %d байт\n", cursor);
	}
	memcpy(data_new_image, data_image, cursor);
	data_image += cursor;
	data_new_image += cursor;

	sscanf(data_image, "%d\n%n", &max_color, &cursor);
	if (debug) {
		printf("[debug]: Максимальное значение цвета - %d\n", max_color);
		printf("[debug]: Считано еще %d байт\n", cursor);
	}
	memcpy(data_new_image, data_image, cursor);
	data_image += cursor;
	data_new_image += cursor;

	if (!(width && height))
		error_exit(SIZE_INCORRECT);

	char cores;
	if (argc == 3)
		sscanf(argv[2], "%d", (int*) &cores);
	else
		cores = sysconf(_SC_NPROCESSORS_ONLN);
	printf("Количество ядер процессора:\t%d\n", cores);
	rows_for_core = height / cores;
	if (height % cores != 0)
		rows_for_core += 1;


	int start_points[cores], i;
	pthread_t id[cores];
	pthread_mutex_init(&mutex_image, NULL);
	uint64_t start_time = rdtsc();
	for (i = 0; i < cores; i++) {
		start_points[i] = i * rows_for_core;
		if (pthread_create(&id[i], NULL, sobel_func, &start_points[i]))
			return error_exit(PTHREAD_ERROR);
		if (debug)
			printf("[debug]: Создан поток с id = %d\n", (int) id[i]);
	}

	for (i=0; i<cores; i++) {
		if (debug)
			printf("[debug]: Ожидается поток с id = %d\n", (int) id[i]);
		pthread_join(id[i], NULL);
		if (debug)
			printf("[debug]: Завершился поток с id = %d\n", (int) id[i]);
	}
	uint64_t end_time = rdtsc();
	
	char name_new_image[255] = "test_result.ppm";
	int handle_new_image;
	handle_new_image = creat(name_new_image, 0777);
	if (handle_new_image == 0)
		error_exit(FILE_NOT_OPEN);
	else if (debug)
		printf("[debug]: Открыт файл\n");

	unsigned char buf[20];
	sprintf(buf, "P%c\n%d %d\n%d\n\0", 
			version, width, height, max_color);
	write(handle_new_image, buf, 20);	
	write(handle_new_image, data_new_image, width * height * 3);

	printf("Выполнено наложения оператора Собеля.\n\
			\rРезультат сохранен в изображении %s\n",
			name_new_image);
	close(handle_new_image);

	long long diff = (end_time - start_time) / 2800000;
	int sec = diff / 1000;
	int	msec = diff % 1000;
	printf("Время выполнения распараллеленной части программы = %ds %dms\n",
			sec, msec);

	_exit(EXIT_SUCCESS);
 }


