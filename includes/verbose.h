/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   verbose.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mgama <mgama@student.42lyon.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/08/02 02:42:55 by mgama             #+#    #+#             */
/*   Updated: 2024/08/02 04:53:51 by mgama            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VERBOSE_H
# define VERBOSE_H

#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

# define DIGITS "0123456789abcdef0123456789ABCDEF"

enum
{
	VERBOSE_OFF = 0,
	VERBOSE_ON = 1
};

extern int	verbose_mode;
extern int	verbose_size;

void	ft_verbose(const char *fmt, ...);

#endif /* VERBOSE_H */