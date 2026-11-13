// SPDX-License-Identifier: GPL-2.0
// vim: set ts=8 sw=8 noet tw=80 cc=80 fo+=t :

/**
 * @file      shell.h
 * @author    midnight walker
 * @brief     Shell execution APIs.
 * @version   0.1
 * @date      2026-11-13
 *
 * @copyright GNU General Public License v2.0
 */

#ifndef INCLUDE_SHELL_H
#define INCLUDE_SHELL_H

int execve_shell_script(const char *pathname, char *const argv[]);

#endif /* INCLUDE_SHELL_H */