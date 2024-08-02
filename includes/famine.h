/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   famine.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mgama <mgama@student.42lyon.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/25 15:44:10 by mgama             #+#    #+#             */
/*   Updated: 2024/08/02 17:21:11 by mgama            ###   ########.fr       */
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
#include <stdint.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/xattr.h>
#include <sys/ioctl.h>

#include "pcolors.h"
#include "ft_getopt.h"

#define FM_VERSION		"2.5"
#define FM_AUTHOR		"mbrement and mgama"
#define FM_HEADER		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0              .\n           ,'/ \\`.\n          |\\/___\\/|\n          \\'\\   /`/\n           `.\\ /,'\n              |\n              |\n             |=|\n        /\\  ,|=|.  /\\\n    ,'`.  \\/ |=| \\/  ,'`.\n  ,'    `.|\\ `-' /|,'    `.\n,'   .-._ \\ `---' / _,-.   `.\n   ,'    `-`-._,-'-'    `.\n  '                       `\n"
#define FM_SIGNATURE	FM_HEADER"Famine "FM_VERSION" by "FM_AUTHOR

#ifndef FM_TARGET
# define FM_TARGET "./target"
#endif

#ifndef FM_SECURITY
# define FM_SECURITY 1
#endif

#define F_DAEMON		0x001
#define F_ONCE			0x002
#define F_MINSTANCE		0x004
#define F_CUSTOM		0x010
#define F_RECURSIVE		0x020
#define F_ADDSERVICE	0x100

#define FM_SHM_KEY		0x424242

extern struct s_famine {
	char	*name;
	void	*me;
	size_t	len;
} g_famine;

#define FM_MAXMAGIC_SIZE	0x10

struct s_famine_magics {
	const uint8_t	magic[FM_MAXMAGIC_SIZE];
	const size_t	len;
};

#endif /* FAMINE_H */