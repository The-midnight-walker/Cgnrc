// SPDX-License-Identifier: GPL-2.0
// vim: set ts=8 sw=8 noet tw=80 cc=80 fo+=t :

/**
 * @file      test01.c
 * @brief     Comprehensive test suite for the cgnrc module framework.
 *
 * Covers:
 * - Direct option registration & argument matching
 * - Submodule tree resolution & command dispatch
 * - POSIX sh & bash shell completion script generation
 * - Shell script execution wrapper (execve_shell_script)
 * - Multi-level logging (console & file logger, level masks, colored output)
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "debug.h"
#include "lists.h"
#include "module.h"
#include "shell.h"

/* ------------------------------------------------------------------------- */
/* Helper state tracking for actions                                         */
/* ------------------------------------------------------------------------- */
static int g_root_action_called = 0;
static int g_sub_action_called = 0;
static int g_leaf_action_called = 0;
static char g_last_opt_val[64] = {0};
static int g_format_side_effects = 0;
static int g_root_replacement_rejected = 0;

static int next_format_value(void)
{
    g_format_side_effects++;
    return 17;
}

static void reset_test_state(void)
{
    g_root_action_called = 0;
    g_sub_action_called = 0;
    g_leaf_action_called = 0;
    memset(g_last_opt_val, 0, sizeof(g_last_opt_val));
}

static int root_action(int optc, module_option_t **optv)
{
    g_root_action_called++;
    for (int i = 0; i < optc; i++) {
        if (optv[i]->is_set && optv[i]->arg_val) {
            snprintf(
                g_last_opt_val, sizeof(g_last_opt_val), "%s", optv[i]->arg_val);
        }
    }
    return 0;
}

static int sub_action(int optc, module_option_t **optv)
{
    g_sub_action_called++;
    for (int i = 0; i < optc; i++) {
        if (optv[i]->is_set && optv[i]->arg_val) {
            snprintf(
                g_last_opt_val, sizeof(g_last_opt_val), "%s", optv[i]->arg_val);
        }
    }
    return 0;
}

static int leaf_action(int optc, module_option_t **optv)
{
    (void)optc;
    (void)optv;
    g_leaf_action_called++;
    return 0;
}

static int try_replace_root_action(int optc, module_option_t **optv)
{
    (void)optc;
    (void)optv;
    g_root_replacement_rejected =
        set_root_module("replacement", NULL, NULL, 0, root_action) == NULL;
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Test 1: Intrusive Doubly Linked List                                      */
/* ------------------------------------------------------------------------- */
typedef struct test_item {
    int id;
    list_node_t node;
} test_item_t;

static void test_intrusive_list(void)
{
    printf("[TEST] Running test_intrusive_list...\n");

    list_node_t head;
    list_init(&head);
    assert(list_empty(&head));

    test_item_t i1 = {.id = 10};
    test_item_t i2 = {.id = 20};
    test_item_t i3 = {.id = 30};

    list_add_tail(&head, &i1.node);
    list_add_tail(&head, &i2.node);
    list_add_tail(&head, &i3.node);
    assert(!list_empty(&head));

    int expected[] = {10, 20, 30};
    int idx = 0;
    list_node_t *pos;
    list_for_each(pos, &head)
    {
        test_item_t *item = container_of(pos, test_item_t, node);
        assert(item->id == expected[idx++]);
    }
    assert(idx == 3);

    /* Test list_del */
    list_del(&i2.node);
    idx = 0;
    int expected_after_del[] = {10, 30};
    list_for_each(pos, &head)
    {
        test_item_t *item = container_of(pos, test_item_t, node);
        assert(item->id == expected_after_del[idx++]);
    }
    assert(idx == 2);

    /* Test const container_of */
    const list_node_t *cpos = head.next;
    const test_item_t *citem = container_of_const(cpos, test_item_t, node);
    assert(citem->id == 10);

    /* Cleanup remaining */
    list_del(&i1.node);
    list_del(&i3.node);
    assert(list_empty(&head));

    printf("[PASS] test_intrusive_list\n");
}

/* ------------------------------------------------------------------------- */
/* Test 2: Top-level module execution & option matching                      */
/* ------------------------------------------------------------------------- */
static void test_module_simple(void)
{
    printf("[TEST] Running test_module_simple...\n");
    reset_test_state();

    module_option_t opts[] = {
        {.s_opt = 'v',
         .l_opt = "verbose",
         .has_arg = no_argument,
         .desc = "Verbose mode"},
        {.s_opt = 'c',
         .l_opt = "config",
         .has_arg = required_argument,
         .arg_name = "FILE",
         .desc = "Config file"}};

    module_t *m = register_module("net", "net [OPTIONS]", opts, 2, sub_action);
    assert(m != NULL);

    assert(launch_cli(NULL) == -1);

    char *argv[] = {"cgnrc", "net", "-v", "--config", "server.conf", NULL};
    int ret = launch_cli(argv);
    assert(ret == 0);
    assert(g_sub_action_called == 1);
    assert(strcmp(g_last_opt_val, "server.conf") == 0);

    printf("[PASS] test_module_simple\n");
}

/* ------------------------------------------------------------------------- */
/* Test 3: Invalid option arguments are rejected without dispatch            */
/* ------------------------------------------------------------------------- */
static void test_module_invalid_args(void)
{
    printf("[TEST] Running test_module_invalid_args...\n");
    reset_test_state();

    module_option_t opts[] = {
        {.s_opt = 'c', .l_opt = "config", .has_arg = required_argument}};
    assert(register_module("net", "net [OPTIONS]", opts, 1, sub_action));

    char *missing_arg[] = {"cgnrc", "net", "--config", NULL};
    assert(launch_cli(missing_arg) == -1);
    assert(g_sub_action_called == 0);

    module_option_t no_arg_opts[] = {
        {.s_opt = 'v', .l_opt = "verbose", .has_arg = no_argument}};
    assert(register_module("net", "net [OPTIONS]", no_arg_opts, 1, sub_action));
    char *unknown_opt[] = {"cgnrc", "net", "--unknown", NULL};
    assert(launch_cli(unknown_opt) == -1);
    assert(g_sub_action_called == 0);

    printf("[PASS] test_module_invalid_args\n");
}

/* ------------------------------------------------------------------------- */
/* Test 4: Option definitions are validated and copied as clean metadata     */
/* ------------------------------------------------------------------------- */
static void test_option_registration_validation(void)
{
    printf("[TEST] Running test_option_registration_validation...\n");

    module_option_t duplicates[] = {
        {.s_opt = 'x', .l_opt = "first", .has_arg = no_argument},
        {.s_opt = 'x', .l_opt = "second", .has_arg = no_argument}};
    assert(
        register_module("invalid", NULL, duplicates, 2, leaf_action) == NULL);

    module_option_t bad_argument = {
        .s_opt = 'b', .l_opt = "bad", .has_arg = 42};
    assert(
        register_module("invalid", NULL, &bad_argument, 1, leaf_action) ==
        NULL);

    module_option_t caller_state = {
        .s_opt = 'v',
        .l_opt = "verbose",
        .has_arg = no_argument,
        .is_set = true,
        .arg_val = "borrowed"};
    module_t *registered =
        register_module("clean", NULL, &caller_state, 1, leaf_action);
    assert(registered != NULL);
    assert(!registered->options[0]->is_set);
    assert(registered->options[0]->arg_val == NULL);

    module_option_t mixed_options[] = {
        {.s_opt = 'q', .has_arg = no_argument},
        {.l_opt = "long-only", .has_arg = no_argument}};
    assert(register_module("mixed", NULL, mixed_options, 2, leaf_action));
    char *long_only_argv[] = {"cgnrc", "mixed", "--long-only", NULL};
    assert(launch_cli(long_only_argv) == 0);

    assert(set_root_module("app", NULL, NULL, 0, root_action));
    assert(register_module("trigger", NULL, NULL, 0, try_replace_root_action));
    g_root_replacement_rejected = 0;
    char *replace_root_argv[] = {"cgnrc", "trigger", NULL};
    assert(launch_cli(replace_root_argv) == 0);
    assert(g_root_replacement_rejected == 1);

    printf("[PASS] test_option_registration_validation\n");
}

/* ------------------------------------------------------------------------- */
/* Test 3: Root module and hierarchical submodules                           */
/* ------------------------------------------------------------------------- */
static void test_hierarchical_modules(void)
{
    printf("[TEST] Running test_hierarchical_modules...\n");
    reset_test_state();

    module_option_t root_opts[] = {
        {.s_opt = 'd',
         .l_opt = "debug",
         .has_arg = no_argument,
         .desc = "Debug flag"}};

    module_t *root = set_root_module(
        "cgnrc_root", "[GLOBAL OPTIONS] <cmd>", root_opts, 1, root_action);
    assert(root != NULL);

    module_option_t svc_opts[] = {
        {.s_opt = 's',
         .l_opt = "status",
         .has_arg = no_argument,
         .desc = "Service status"}};
    module_t *svc =
        register_module("service", "service <subcmd>", svc_opts, 1, sub_action);
    assert(svc != NULL);

    module_t *start =
        register_submodule(svc, "start", "start service", NULL, 0, leaf_action);
    assert(start != NULL);

    /* Invoking root option + service submodule + start leaf */
    char *argv[] = {"cgnrc", "-d", "service", "-s", "start", NULL};
    int ret = launch_cli(argv);
    assert(ret == 0);
    assert(g_root_action_called == 1);
    assert(g_sub_action_called == 1);
    assert(g_leaf_action_called == 1);

    printf("[PASS] test_hierarchical_modules\n");
}

/* ------------------------------------------------------------------------- */
/* Test 4: Shell completions generation (POSIX sh & Bash)                    */
/* ------------------------------------------------------------------------- */
static void test_shell_completions(void)
{
    printf("[TEST] Running test_shell_completions...\n");

    const char injected_marker[] = "/tmp/cgnrc_completion_injected";
    unlink(injected_marker);
    module_set_prog_name("odd name;touch /tmp/cgnrc_completion_injected'");

    module_option_t opts[] = {
        {.s_opt = 'p',
         .l_opt = "port",
         .has_arg = required_argument,
         .arg_name = "NUM",
         .desc = "Port number"}};
    module_t *m =
        register_module("server", "server options", opts, 1, sub_action);
    assert(m != NULL);

    const char *sh_file = "/tmp/test_completion.sh";
    const char *bash_file = "/tmp/test_completion.bash";

    int ret_sh = generate_sh_completion(sh_file);
    assert(ret_sh == 0);
    assert(access(sh_file, F_OK) == 0);

    int ret_bash = generate_bash_completion(bash_file);
    assert(ret_bash == 0);
    assert(access(bash_file, F_OK) == 0);

    char syntax_check[256];
    int command_len = snprintf(
        syntax_check,
        sizeof(syntax_check),
        "sh -n '%s' && bash -n '%s'",
        sh_file,
        bash_file);
    assert(command_len > 0 && (size_t)command_len < sizeof(syntax_check));
    assert(system(syntax_check) == 0);
    assert(system("sh -c '. /tmp/test_completion.sh'") == 0);
    assert(system("bash -c '. /tmp/test_completion.bash'") == 0);
    assert(access(injected_marker, F_OK) == -1);

    /* Clean up registry after test */
    char *argv[] = {"test_cgnrc", NULL};
    launch_cli(argv);

    unlink(sh_file);
    unlink(bash_file);

    module_set_prog_name("test_cgnrc");
    assert(register_module("bad name", "unsafe", NULL, 0, leaf_action));
    const char *unsafe_file = "/tmp/test_unsafe_completion.sh";
    unlink(unsafe_file);
    assert(generate_sh_completion(unsafe_file) == -1);
    assert(access(unsafe_file, F_OK) == -1);
    char *cleanup_argv[] = {"test_cgnrc", NULL};
    assert(launch_cli(cleanup_argv) == 0);

    printf("[PASS] test_shell_completions\n");
}

/* ------------------------------------------------------------------------- */
/* Test 5: Usage rendering                                                   */
/* ------------------------------------------------------------------------- */
static void test_usage_rendering(void)
{
    printf("[TEST] Running test_usage_rendering...\n");

    module_option_t opts[] = {
        {.s_opt = 'h',
         .l_opt = "help",
         .has_arg = no_argument,
         .desc = "Show help message"}};
    module_t *m =
        register_module("help_mod", "help_mod usage", opts, 1, sub_action);
    assert(m != NULL);

    module_t unrelated_module = {.name = "external"};
    assert(pr_module_usage(stdout, &unrelated_module) == -1);

    int u_ret = pr_module_usage(stdout, m);
    assert(u_ret == 0);

    pr_module_short_usage(stdout, m);

    /* Cleanup */
    char *argv[] = {"cgnrc", NULL};
    launch_cli(argv);

    printf("[PASS] test_usage_rendering\n");
}

/* ------------------------------------------------------------------------- */
/* Test 6: Shell execution wrapper (execve_shell_script)                     */
/* ------------------------------------------------------------------------- */
static void test_shell_exec(void)
{
    printf("[TEST] Running test_shell_exec...\n");

    /* Create temporary executable script */
    char script_path[] = "/tmp/cgnrc_test_script_XXXXXX";
    int fd = mkstemp(script_path);
    assert(fd != -1);

    const char script_content[] = "#!/bin/sh\nexit 42\n";
    ssize_t written = write(fd, script_content, sizeof(script_content) - 1);
    assert(written == (ssize_t)(sizeof(script_content) - 1));
    close(fd);

    int chmod_ret = chmod(script_path, 0755);
    assert(chmod_ret == 0);

    char *argv[] = {script_path, NULL};
    int ret = execve_shell_script(script_path, argv);
    assert(ret == 42);

    /* Test fallback when argv is NULL */
    int ret_fallback = execve_shell_script(script_path, NULL);
    assert(ret_fallback == 42);

    const char signaled_script[] = "#!/bin/sh\nkill -TERM $$\n";
    fd = open(script_path, O_WRONLY | O_TRUNC);
    assert(fd != -1);
    written = write(fd, signaled_script, sizeof(signaled_script) - 1);
    assert(written == (ssize_t)(sizeof(signaled_script) - 1));
    close(fd);
    assert(execve_shell_script(script_path, argv) == 128 + SIGTERM);

    assert(execve_shell_script("/definitely/not/a/real/script", NULL) == -1);

    /* Test invalid pathname */
    int ret_inval = execve_shell_script(NULL, NULL);
    assert(ret_inval == -1);

    unlink(script_path);
    printf("[PASS] test_shell_exec\n");
}

/* ------------------------------------------------------------------------- */
/* Test 7: Logging API (Console & File Logger)                               */
/* ------------------------------------------------------------------------- */
static void test_logging(void)
{
    printf("[TEST] Running test_logging...\n");

    assert(get_cli_log_mask() == LOG_MASK_ALL);
    assert(get_file_log_mask() == 0);
    assert(set_cli_log_mask(0) == LOG_ENOINIT);
    assert(set_file_log_mask(0) == LOG_ENOINIT);

    char log_path[] = "/tmp/cgnrc_test_log_XXXXXX";
    int fd = mkstemp(log_path);
    assert(fd != -1);
    close(fd);

    /* Initialise logger with INFO level on console and ALL on file */
    int init_ret = init_logging_ctx(
        LOG_LEVEL_INFO | LOG_LEVEL_ERROR,
        log_path,
        O_TRUNC,
        0644,
        LOG_MASK_ALL);
    assert(init_ret == LOG_SUCCESS);

    /* Duplicate init should be safely rejected */
    assert(init_logging_ctx(0, NULL, 0, 0, 0) == LOG_EALREADYINIT);

    /* Test mask getters and setters */
    assert(get_cli_log_mask() == (LOG_LEVEL_INFO | LOG_LEVEL_ERROR));
    assert(get_file_log_mask() == LOG_MASK_ALL);

    set_cli_log_mask(LOG_MASK_ALL);
    assert(get_cli_log_mask() == LOG_MASK_ALL);

    /* Write log messages */
    pr_info("Informational message: %d", 100);
    pr_warn("Warning message: %s", "caution");
    pr_error("Error message");
    pr_debug("Debug message");

    /* Test file logger direct write */
    int lw_ret = log_write("Direct raw log message: test=%s\n", "ok");
    assert(lw_ret == LOG_SUCCESS);

    /* A long formatted message must be safely truncated, not indexed by its
     * untruncated vsnprintf() length. */
    char large_message[4096];
    memset(large_message, 'x', sizeof(large_message) - 1);
    large_message[sizeof(large_message) - 1] = '\0';
    assert(log_write("%s", large_message) == LOG_SUCCESS);

    /* Formatting arguments are evaluated once even when both outputs are on. */
    set_cli_log_mask(LOG_MASK_ALL);
    set_file_log_mask(LOG_MASK_ALL);
    g_format_side_effects = 0;
    pr_info("single-evaluation=%d", next_format_value());
    assert(g_format_side_effects == 1);

    /* Print colorized log check */
    int print_ret = print_colored_log(log_path);
    assert(print_ret == 0);

    unlink(log_path);
    printf("[PASS] test_logging\n");
}

/* ------------------------------------------------------------------------- */
/* Main test runner                                                          */
/* ------------------------------------------------------------------------- */
int main(void)
{
    printf("========================================\n");
    printf("   cgnrc Test Suite       \n");
    printf("========================================\n");

    test_intrusive_list();
    test_module_simple();
    test_module_invalid_args();
    test_option_registration_validation();
    test_hierarchical_modules();
    test_shell_completions();
    test_usage_rendering();
    test_shell_exec();
    test_logging();

    printf("========================================\n");
    printf("   All tests completed successfully!    \n");
    printf("========================================\n");

    return 0;
}
