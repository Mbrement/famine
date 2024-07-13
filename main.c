#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>


#define _TARGET "./target"

void famine(struct dirent *d, char *path){
	char signature[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0Famine 1.2 by mbrement and mgama";
	if (d->d_type != DT_REG)
		return ;
	char name[sizeof(signature) + 256];
	strcpy(name, path);
	strcat(name, "/");
	strcat(name, d->d_name);
	int fd = open(name, O_RDWR);
	if (fd == -1)
		return ;
	char buf[5];
	int n = read(fd, buf, 4);
	if (n == -1)
		return ;
	buf[4] = '\0';
	char elf[5] = {0x7f, 'E', 'L', 'F', '\0'};
	if (strncmp(buf, elf, 4) == 0)
	{
		size_t pos = lseek(fd, 0, SEEK_END);
		lseek(fd, pos - sizeof(signature), SEEK_SET);
		char buf2[sizeof(signature)];
		int n = read(fd, buf2, sizeof(signature));
		if (n <= 0)
			return ;
		for(size_t i = 0; i < sizeof(signature); i++)
		{
			if ((signature[i] == buf2[i] && i == sizeof(signature) - 2)){
				close(fd);
				return ;
			}
			if (signature[i] != buf2[i])
				break;
		}
		int fd2=open(name, O_WRONLY | O_APPEND);
		if (fd2 == -1)
		{
			close(fd);
			return ;
		}
		write(fd2, signature, sizeof(signature));
		close(fd2);
	}
	if (fd != -1)
		close(fd);
}

int main(int argc, char **argv){
		DIR *dir;
		dir = opendir(_TARGET);
		if (dir && argc == 1)
		{
			for (struct dirent *d = readdir(dir); d != NULL; d = readdir(dir))
				famine(d, _TARGET);
		}
		else
		for (int i = 1; i < argc; i++)
		{
			for (struct dirent *d = readdir(dir); d != NULL; d = readdir(dir))
			{
				if (strcmp(d->d_name, argv[i]) == 0)
					famine(d, _TARGET);
			}
			closedir(dir);
			dir = opendir(_TARGET);
		}
	closedir(dir);
	return 0;
}