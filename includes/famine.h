/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   famine.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbrement <mbrement@student.42lyon.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/07/25 15:44:10 by mgama             #+#    #+#             */
/*   Updated: 2024/07/25 16:47:38 by mbrement         ###   ########lyon.fr   */
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

#include "become_daemon.h"
#include "ft_getopt.h"

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

#define F_DAEMON	0x01
#define F_ONCE		0x02
#define F_CUSTOM	0x10

/**
 * Utils
 */

char	**ft_split(const char *str, char *charset);

#endif /* FAMINE_H */