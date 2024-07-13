#include "stdio.h"
#include <sys/types.h>
#include <dirent.h>

void famine(struct dirent *d, char *path){
	printf("Name: %s\n", d->d_name);
	printf("type: %d\n", d->d_type);

}

int main(void){
	DIR *dir;
	dir = opendir(".");
	if (dir)
	{
	
		for (struct dirent *d = readdir(dir); d != NULL; d = readdir(dir))
			famine(d);

	}
	return 0;
}