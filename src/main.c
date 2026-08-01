/*
 * main.c - command dispatch.
 *
 * Commands live in a table rather than an if/else chain, so adding one is a
 * single line and `help` stays in sync automatically.  Each handler receives
 * argv positioned *after* the command word.
 */
#include "minigit.h"

typedef struct {
    const char *name;
    const char *alias;
    int       (*fn)(int argc, char **argv);
    const char *usage;
    const char *summary;
} Command;

static int cmd_help(int argc, char **argv);
static int cmd_version(int argc, char **argv);

static const Command commands[] = {
    { "init",     NULL,  cmd_init,     "init",
      "Create an empty repository in ./" MG_DIR },
    { "add",      NULL,  cmd_add,      "add <path>... | .",
      "Stage files for the next commit" },
    { "rm",       NULL,  cmd_rm,       "rm [--cached] <path>...",
      "Stage a deletion and drop the file" },
    { "status",   "st",  cmd_status,   "status",
      "Show staged, unstaged and untracked changes" },
    { "commit",   NULL,  cmd_commit,   "commit -m \"message\"",
      "Record the staged snapshot" },
    { "log",      NULL,  cmd_log,      "log [--oneline] [-n <count>]",
      "Show commit history, newest first" },
    { "show",     NULL,  cmd_show,     "show [<commit>]",
      "Show one commit and the paths it touched" },
    { "branch",   "br",  cmd_branch,   "branch [<name> | -d <name>]",
      "List, create or delete branches" },
    { "checkout", "co",  cmd_checkout, "checkout [-b] [-f] <branch|commit>",
      "Switch branches or restore a commit" },
    { "merge",    NULL,  cmd_merge,    "merge <branch|commit>",
      "Three-way merge another line of history" },
    { "diff",     NULL,  cmd_diff,     "diff [<rev> [<rev>]] | <fileA> <fileB>",
      "Show line differences (LCS based)" },
    { "push",     NULL,  cmd_push,     "push <remote-path>",
      "Copy objects and refs to another repository" },
    { "pull",     NULL,  cmd_pull,     "pull <remote-path>",
      "Fetch objects and fast-forward from another repository" },
    { "help",     NULL,  cmd_help,     "help [<command>]",
      "Show this message" },
    { "version",  NULL,  cmd_version,  "version",
      "Print the MiniGit version" },
};

static const size_t ncommands = sizeof(commands) / sizeof(commands[0]);

static const Command *lookup(const char *name)
{
    for (size_t i = 0; i < ncommands; i++)
        if (strcmp(commands[i].name, name) == 0 ||
            (commands[i].alias && strcmp(commands[i].alias, name) == 0))
            return &commands[i];
    return NULL;
}

static int cmd_help(int argc, char **argv)
{
    if (argc > 0) {
        const Command *c = lookup(argv[0]);
        if (!c) {
            mg_warn("no such command '%s'", argv[0]);
            return 1;
        }
        printf("usage: mini_git %s\n\n  %s\n", c->usage, c->summary);
        return 0;
    }

    printf("%sMiniGit %s%s - a small content-addressed version control system\n\n",
           C_BOLD(), MG_VERSION, C_OFF());
    printf("usage: mini_git <command> [args]\n\n");
    for (size_t i = 0; i < ncommands; i++)
        printf("  %s%-9s%s %s\n", C_CYAN(), commands[i].name, C_OFF(),
               commands[i].summary);
    printf("\nRun 'mini_git help <command>' for details.\n");
    return 0;
}

static int cmd_version(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("mini_git %s\n", MG_VERSION);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        cmd_help(0, NULL);
        return 1;
    }

    const Command *c = lookup(argv[1]);
    if (!c) {
        mg_warn("'%s' is not a mini_git command; try 'mini_git help'", argv[1]);
        return 1;
    }

    return c->fn(argc - 2, argv + 2);
}
