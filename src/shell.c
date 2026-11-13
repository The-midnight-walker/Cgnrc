// SPDX-License-Identifier: GPL-2.0
//
// vim: set ts=8 sw=8 noet tw=80 cc=80 fo+=t :

/**
 * @file      shell.c
 * @author    midnight walker
 * @brief     Shell scripts utilities.
 *
 * @version   0.1
 * @date      2026-11-13
 * @copyright GNU General Public License v2.0
 *
 */

#define prfx_fmt "shell: "

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "debug.h"
#include "shell.h"

extern char **environ;

/**
 * @brief Implementation of the shell script execution via fork and execve.
 *
 * @param pathname Path to the script to execute.
 * @param argv Argument vector for the script.
 * @return Exit status of the script, or -1 on failure.
 */
static int execve_shell_script_impl(const char *pathname, char *const argv[])
{
    int error_pipe[2];
    if (pipe(error_pipe) == -1) {
        pr_error("failed to create exec status pipe");
        return -1;
    }

    int fd_flags = fcntl(error_pipe[1], F_GETFD);
    if (fd_flags == -1 ||
        fcntl(error_pipe[1], F_SETFD, fd_flags | FD_CLOEXEC) == -1) {
        int saved_errno = errno;
        close(error_pipe[0]);
        close(error_pipe[1]);
        errno = saved_errno;
        pr_error("failed to configure exec status pipe");
        return -1;
    }

    pid_t sh_pid = fork();
    if (sh_pid == -1) {
        int saved_errno = errno;
        close(error_pipe[0]);
        close(error_pipe[1]);
        errno = saved_errno;
        pr_error("fork process failed");
        return -1;
    }

    if (sh_pid == 0) {
        /* Only async-signal-safe operations are allowed between fork/exec. */
        close(error_pipe[0]);
        execve(pathname, argv, environ);
        int exec_errno = errno;
        const char *bytes = (const char *)&exec_errno;
        size_t remaining = sizeof(exec_errno);
        while (remaining > 0) {
            ssize_t written = write(error_pipe[1], bytes, remaining);
            if (written > 0) {
                bytes += (size_t)written;
                remaining -= (size_t)written;
            } else if (written < 0 && errno == EINTR) {
                continue;
            } else {
                break;
            }
        }
        _exit(127);
    }

    close(error_pipe[1]);

    int exec_errno = 0;
    size_t received = 0;
    while (received < sizeof(exec_errno)) {
        ssize_t count = read(
            error_pipe[0], (char *)&exec_errno + received,
            sizeof(exec_errno) - received);
        if (count > 0) {
            received += (size_t)count;
        } else if (count == 0) {
            break;
        } else if (errno != EINTR) {
            int saved_errno = errno;
            close(error_pipe[0]);
            pid_t waited;
            do {
                waited = waitpid(sh_pid, NULL, 0);
            } while (waited == -1 && errno == EINTR);
            errno = saved_errno;
            pr_error("failed to read exec status");
            return -1;
        }
    }
    close(error_pipe[0]);

    int status;
    pid_t waited;
    do {
        waited = waitpid(sh_pid, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited == -1) {
        pr_error("wait action for child failed");
        return -1;
    }
    if (received > 0) {
        errno = received == sizeof(exec_errno) ? exec_errno : EIO;
        pr_error("execve failed: %s", strerror(errno));
        return -1;
    }

    int ret = WIFEXITED(status) ? WEXITSTATUS(status)
                                : 128 + WTERMSIG(status);
    pr_debug("execute shell script end; ret=%d", ret);
    return ret;
}

/**
 * @brief Wrapper function to validate arguments before executing the shell
 * script.
 * @param pathname Path to the script.
 * @param argv Argument vector.
 * @return Result status code.
 */
int execve_shell_script(const char *pathname, char *const argv[])
{
    if (!pathname) {
        pr_error(
            "script to execute path hasn't been specified script_path=%p",
            (const void *)pathname);
        return -1;
    }
    if (argv && !argv[0]) {
        errno = EINVAL;
        return -1;
    }

    /* Fallback mechanism if argv is NULL to prevent execve crash */
    char *default_argv[] = {(char *)(uintptr_t)(const void *)pathname, NULL};
    char *const *effective_argv = argv ? argv : default_argv;

    if (!argv) {
        pr_debug(
            "args was NULL, falling back to default argv={pathname, NULL}");
    }

    pr_debug("execute shell script");
    return execve_shell_script_impl(pathname, (char *const *)effective_argv);
}
