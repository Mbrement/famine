/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   famine.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mgama <mgama@student.42lyon.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/25 15:44:10 by mgama             #+#    #+#             */
/*   Updated: 2024/07/26 19:43:10 by mgama            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FAMINE_H
#define FAMINE_H

#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "become_daemon.h"
#include "ft_getopt.h"

#define FM_VERSION		"1.6"
#define FM_AUTHOR		"mbrement and mgama"
#define FM_HEADER		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0              .\n           ,'/ \\`.\n          |\\/___\\/|\n          \\'\\   /`/\n           `.\\ /,'\n              |\n              |\n             |=|\n        /\\  ,|=|.  /\\\n    ,'`.  \\/ |=| \\/  ,'`.\n  ,'    `.|\\ `-' /|,'    `.\n,'   .-._ \\ `---' / _,-.   `.\n   ,'    `-`-._,-'-'    `.\n  '                       `\n"
#define FM_SIGNATURE	FM_HEADER"Famine "FM_VERSION" by "FM_AUTHOR

#ifndef FM_TARGET
# define FM_TARGET "./target"
#endif

#ifndef FM_SECURITY
# define FM_SECURITY 1
#endif

extern struct s_famine {
	char *name;
	void *me;
	size_t len;
} g_famine;

extern int g_exit;

#define F_DAEMON	0x001
#define F_ONCE		0x002
#define F_MINSTANCE	0x004
#define F_CUSTOM	0x010

#define SHM_KEY		0x42424242

#endif /* FAMINE_H */