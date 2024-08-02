/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   daemon.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mgama <mgama@student.42lyon.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/25 15:44:10 by mgama             #+#    #+#             */
/*   Updated: 2024/08/02 05:15:43 by mgama            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BECOME_DAEMON_H
#define BECOME_DAEMON_H

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <spawn.h>

#include <sys/wait.h>
#include <sys/stat.h>

#define D_NO_CHDIR          01 /* Don't chdir ("/") */
#define D_NO_CLOSE_FILES    02 /* Don't close all open files */
#define D_NO_REOPEN_STD_FDS 04 /* Don't reopen stdin, stdout, and stderr to /dev/null */
#define D_NO_UMASK0        010 /* Don't do a umask(0) */
#define D_MAX_CLOSE       8192 /* Max file descriptors to close if sysconf(_SC_OPEN_MAX) is indeterminate */

// returns 0 on success -1 on error
int	become_daemon(int flags);

#define D_SERVICENAME	"famine"
#define D_SERVICEARGS	"<%= @args %>"
#define D_BINPATH		"/usr/local/bin"

// returns 0 on success -1 on error
int setup_daemon(int argc, char **argv, const void *prog_data, size_t prog_size, char *const *envp);

#endif /* BECOME_DAEMON_H */