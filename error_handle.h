#include <string.h>
#include <stdio.h>
#include <errno.h>

#define OPT_ERROR 1
#define FILE_NOT_OPEN 3
#define HASH_ERROR 4
#define PTHREAD_ERROR 5
#define MEM_ERROR 6
#define FSTAT 7
#define READ_CNT 8
#define VERSION_PPM 9
#define SIZE_INCORRECT 10

int error_exit(int id_error)
{
	char text_error[80];
	int i;
	for (i=0; i<80; i++)
		text_error[i]='\0';
	switch (id_error) {
		case OPT_ERROR:
			strcpy(text_error,
				"Ошибка! Проверте кол-во аргументов!\n");
			break;
		case FILE_NOT_OPEN:
			strcpy(text_error,
				"Ошибка! Файл не открывается!\n");
			break;
		case PTHREAD_ERROR:
			strcpy(text_error,
				"Ошибка! Не создается поток!\n");
			break;
		case MEM_ERROR:
			strcpy(text_error,
				"Ошибка! Не получилось выделить память\n");
			break;
		case FSTAT:
			strcpy(text_error,
				"Ошибка! Не смог получить статичтику\n");
			break;
		case READ_CNT:
			strcpy(text_error, 
				"Ошибка! Кол-во считаного не соответсвует размеру файла !\n");
			break;
		case VERSION_PPM:
			strcpy(text_error,
				"Ошибка! Не умею читать ppm такой версии!\n");
			break;
		case SIZE_INCORRECT:
			strcpy(text_error,
				"Ошибка! Один из размеров равен нулю!\n");
			break;
	}
	write(0, text_error, 80);
	if (errno)
		printf("Комментарий: %s\n", strerror(errno));
	_exit(EXIT_FAILURE);
}
