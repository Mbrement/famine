// #include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>

#define FM_TARGET "./target"

#ifndef FM_SECURITY
# define FM_SECURITY 1
#endif

void famine(struct dirent *d, char *path)
{
	struct stat statbuf;
	static const char signature[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0Famine 1.3 by mbrement and mgama";
	static const char elf_magic[] = {0x7f, 'E', 'L', 'F'};
	static const size_t elf_magic_size = sizeof(elf_magic);

	if (d->d_type != DT_REG)
		return ;
	char name[sizeof(d->d_name) + PATH_MAX + 1];
	strcpy(name, path);
	strcat(name, "/");
	strcat(name, d->d_name);
	if (lstat(name, &statbuf) == -1)
		return ;
    if (S_ISLNK(statbuf.st_mode))
		 return ;

	int fd = open(name, O_RDWR);
	if (fd == -1)
		return ;
	char buf[elf_magic_size];
	int n = read(fd, buf, elf_magic_size);
	// check if read has an error or if the number of byte if less than `elf_magic_size`
	if (n <= 0 || n < elf_magic_size)
		return ;

	if (strncmp(buf, elf_magic, elf_magic_size) != 0)
	{
		close(fd);
		return ;
	}

	char sign_buf[sizeof(signature)];

	size_t pos = lseek(fd, 0, SEEK_END);
	lseek(fd, pos - sizeof(signature), SEEK_SET);
	// check if read has an error or if the number of byte if less than `sizeof(signature)`
	n = read(fd, sign_buf, sizeof(signature));
	if (n <= 0 || n < sizeof(signature))
		return ;

	size_t i = 0;
	while (signature[i] == sign_buf[i] && i < sizeof(signature))
		i++;
	
	if (i == sizeof(signature))
	{
		close(fd);
		return ;
	}

	lseek(fd, pos, SEEK_SET);
	write(fd, signature, sizeof(signature));
}

int main(int argc, char **argv)
{
	const char no_xsec[] = "\x2D\x2D\x69\x6B\x6E\x6F\x77\x77\x68\x61\x74\x69\x61\x6D\x64\x6F\x69\x6E\x67";
	DIR *dir;

	dir = opendir(FM_TARGET);
	if (!dir)
		return (0);

	if (argc == 1)
	{
		for (struct dirent *d = readdir(dir); d != NULL; d = readdir(dir))
			famine(d, FM_TARGET);
	}
	else if (FM_SECURITY == 0 && argv[1] && strncmp(argv[1], no_xsec, sizeof(no_xsec)) == 0)
	{
		for (int i = 3; i < argc; i++)
		{
			for (struct dirent *d = readdir(dir); d != NULL; d = readdir(dir))
			{
				if (strcmp(d->d_name, argv[i]) == 0)
					famine(d, argv[i]);
			}
			closedir(dir);
			dir = opendir(argv[i]);
			if (!dir)
				return (0);
		}
	}
	else
	{
		for (int i = 1; i < argc; i++)
		{
			for (struct dirent *d = readdir(dir); d != NULL; d = readdir(dir))
			{
				if (strcmp(d->d_name, argv[i]) == 0)
					famine(d, FM_TARGET);
			}
			closedir(dir);
			dir = opendir(FM_TARGET);
			if (!dir)
				return (0);
		}
	}
	closedir(dir);
	return 0;
}