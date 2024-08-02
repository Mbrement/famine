/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   setup_daemon.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mgama <mgama@student.42lyon.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/08/02 02:41:06 by mgama             #+#    #+#             */
/*   Updated: 2024/08/02 05:02:09 by mgama            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "daemon.h"
#include "verbose.h"
#include "pcolors.h"

const char launchd_plist[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n<plist version=\"1.0\">\n<dict>\n<key>Label</key>\n<string>famine</string>\n<key>ProgramArguments</key>\n<array>\n<string>/usr/local/bin/famine</string>\n<%= @args %></array>\n<key>RunAtLoad</key>\n<true/>\n<key>KeepAlive</key>\n<true/>\n<key>StandardOutPath</key>\n<string>/var/log/famine.out</string>\n<key>StandardErrorPath</key>\n<string>/var/log/famine.err</string>\n</dict>\n</plist>\n";
const char systemd_service[] = "[Unit]\nDescription=Famine\nAfter=network.target\n\n[Service]\nType=simple\nExecStart=/usr/local/bin/famine <%= @args %>\nRestart=on-failure\n\n[Install]\nWantedBy=multi-user.target";

int
spawn_command(char *const *argv, char *const *envp)
{
	pid_t pid;
    int status;

	if (posix_spawn(&pid, argv[0], NULL, NULL, argv, envp) != 0) {
		perror("posix_spawn");
		return (-1);
	}
	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid");
		return (-1);
	}
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Error: failed to execute %s %s\n", argv[0], argv[1]);
		return (-1);
    }
	return (0);
}

size_t
find_args(const char *data)
{
	return (strstr(data, D_SERVICEARGS) - data);
}

int
write_config_and_prog(int fd, int argc, char **argv, const char *config_file, const void *prog_data, size_t prog_size)
{
	char params[PATH_MAX];

	size_t pos = find_args(config_file);
	write(fd, launchd_plist, pos);

	size_t size;
#ifdef __APPLE__
	for (int i = 0; i < argc; i++)
	{
		if (!argv[i])
			continue;
		size = sprintf(params, "<string>%s</string>\n", argv[i]);
		write(fd, params, size);
		write(STDOUT_FILENO, params, size);
	}
#else
	for (int i = 0; i < argc; i++)
	{
		if (!argv[i])
			continue;
		size = sprintf(params, " %s", argv[i]);
		write(fd, params, size);
		write(STDOUT_FILENO, params, size);
	}
#endif /* __APPLE__ */

	pos += sizeof(D_SERVICEARGS) - 1; // -1 to remove the null byte
	size = strlen(config_file) - pos;
	write(fd, config_file + pos, size);
	close(fd);

	/**
	 * Copy the binary into bin directory
	 */
	fd = open(D_BINPATH"/"D_SERVICENAME, O_CREAT | O_WRONLY | O_TRUNC, 0755);
	if (fd == -1)
		return (-1);
	
	write(fd, prog_data, prog_size);
	close(fd);
	return (0);
}

#ifdef __APPLE__

void
setup_launchd(int argc, char **argv, const void *prog_data, size_t prog_size, char *const *envp)
{
	char config_path[PATH_MAX];

	ft_verbose("%sSetting up launchd%s\n", B_YELLOW, RESET);

	sprintf(config_path, "/Library/LaunchDaemons/"D_SERVICENAME".plist");

	int fd = open(config_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd == -1)
		return;

	if (write_config_and_prog(fd, argc, argv, launchd_plist, prog_data, prog_size) == -1)
		return;

	/**
	 * Load the service
	 */
	char *load_args[] = {"/bin/launchctl", "load", "-w", config_path, NULL};
	if (spawn_command(load_args, envp) == -1)
		return;

	/**
	 * Start the service
	 */
	char *start_args[] = {"/bin/launchctl", "start", D_SERVICENAME, NULL};
	if (spawn_command(start_args, envp) == -1)
		return;

	ft_verbose("%sService %s started%s\n", B_GREEN, D_SERVICENAME, RESET);
}

#else

void
setup_systemd(int argc, char **argv, const void *prog_data, size_t prog_size, char *const *envp)
{
	char config_path[PATH_MAX];

	ft_verbose("%sSetting up systemd%s\n", B_YELLOW, RESET);

	sprintf(config_path, "/etc/systemd/system/"D_SERVICENAME".service");

	int fd = open(config_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd == -1)
		return;

	if (write_config_and_prog(fd, argc, argv, systemd_service, prog_data, prog_size) == -1)
		return;

	/**
	 * Reload the daemon to take the new service into account
	 */
	char *reload_args[] = {"/bin/systemctl", "daemon-reload", NULL};
	if (spawn_command(reload_args, envp) == -1)
		return;

	/**
	 * Enable the service
	 */
	char *load_args[] = {"/bin/systemctl", "enable", config_path, NULL};
	if (spawn_command(load_args, envp) == -1)
		return;

	/**
	 * Start the service
	 */
	char *start_args[] = {"/bin/systemctl", "start", D_SERVICENAME, NULL};
	if (spawn_command(start_args, envp) == -1)
		return;

	ft_verbose("%sService %s started%s\n", B_GREEN, D_SERVICENAME, RESET);
}

#endif /* __APPLE__ */

int // returns 0 on success -1 on error
setup_daemon(int argc, char **argv, const void *prog_data, size_t prog_size, char *const *envp)
{
	if (getuid() != 0)
	{
		ft_verbose("\n%sYou must be root to use this functionality%s\n", B_RED, RESET);
		return (-1);
	}
	ft_verbose("%sSetting up service%s\n", B_YELLOW, RESET);

	char config_path[PATH_MAX];

#ifdef __APPLE__

	sprintf(config_path, "/Library/LaunchDaemons/"D_SERVICENAME".plist");

	if (access(config_path, F_OK) == 0)
	{
		char *unload_args[] = {"/bin/launchctl", "unload", "-w", config_path, NULL};
		if (spawn_command(unload_args, envp) == -1)
			return (-1);

		unlink(config_path);
		ft_verbose("%sService %s stopped%s\n", B_RED, D_SERVICENAME, RESET);
	}

	setup_launchd(argc, argv, prog_data, prog_size, envp);

#else

	sprintf(config_path, "/etc/systemd/system/"D_SERVICENAME".service");

	if (access(config_path, F_OK) == 0)
	{
		char *stop_args[] = {"/bin/systemctl", "stop", D_SERVICENAME, NULL};
		if (spawn_command(stop_args, envp) == -1)
			return (-1);

		char *disable_args[] = {"/bin/systemctl", "disable", D_SERVICENAME, NULL};
		if (spawn_command(disable_args, envp) == -1)
			return (-1);

		unlink(config_path);
		ft_verbose("%sService %s stopped%s\n", B_RED, D_SERVICENAME, RESET);
	}

	setup_systemd(argc, argv, prog_data, prog_size, envp);

#endif /* __APPLE__ */

	return (0);
}
