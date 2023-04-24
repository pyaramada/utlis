/******************************************************************************
  @file   shell_token.c
  @brief

  DESCRIPTION: utility to extract ash compatible tokens.
  <command>[ params][operators]

  Token separator is
  - blanks
  - tabs
  - control operators
    {'&', '|', '&&', '||', ';'}
  - redirection operators
    {'>', '>>', '<', '<>'}
  Quoting makes an exception to the token separator. Matching double quotes or single quotes

****************************************************************************/

#include <stdio.h>
#include <string.h>

#if 0
/* Redirection operators */
enum shell_redir {
    SOP_REDIR_NONE,
    SOP_REDIR_OUT_APPEND,   /* >> */
    SOP_REDIR_OUT,          /* > */
    SOP_REDIR_IN,           /* < */
    SOP_REDIR_INOUT,        /* <> */
};
#endif

/* Control operators */
enum shell_operator {
    SOP_NONE,
    SOP_REDIR_OUT_APPEND,   /* >> */
    SOP_REDIR_OUT,          /* > */
    SOP_REDIR_IN,           /* < */
    SOP_REDIR_INOUT,        /* <> */
    SOP_AND,                /* logical && */
    SOP_OR,                 /* logical || */
    SOP_BG,                 /* background */
    SOP_PIPE,               /* | */
    SOP_NEXT,               /* ; */
};

#define EAT_WHITE(p) while (p && (*p == ' ' || *p == '\t')) p++;
#define GET_CHAR(p, c) while (p && *p && *p != c) p++;
#define GET_SEPER(p) while (p && *p && *p != ' ' && *p != '\t' && *p != '>' && *p != '<' && *p != '|' && *p != '&' && *p != ';') { if (*p == '\"') { p++; GET_CHAR(p, '\"'); if (*p) p++; } else p++; }

void shell_next_token(const char* input, const char** token_begin, const char** token_end, const char** context)
{
    const char* cp = (input != NULL) ? input : *context;

    EAT_WHITE(cp);   /* seek */
    *token_end = *token_begin = cp;
    if (*cp) {
        GET_SEPER(cp);
        *token_end = cp;
    }

    *context = cp;
}

enum shell_operator shell_command_param_split(const char* input, const char** cmd_begin, const char** cmd_end, 
    const char** params_begin, const char** params_end, const char** context)
{
    enum shell_operator o = SOP_NONE;
    const char* cp = (input != NULL) ? input : *context;

    *cmd_begin = NULL;
    *cmd_end = NULL;
    *params_begin = NULL;
    *params_end = NULL;

    EAT_WHITE(cp);   /* seek command */
    *cmd_end = *cmd_begin = cp; /* initialize command begin and end here */
    if (*cp) {
        GET_SEPER(cp);  /* seek to end of command */
        *cmd_end = cp;
        if (*cp && *cp == ' ') {
            EAT_WHITE(cp);   /* seek parameter */
            *params_end = *params_begin = cp;
            if (*cp) {
                GET_SEPER(cp);
                *params_end = cp;
                while (*cp && *cp == ' ') {
                    EAT_WHITE(cp);   /* seek next */
                    GET_SEPER(cp);
                    if (*cp && *cp == ' ') {
                        *params_end = cp;
                    }
                }
            }
        }
        switch (*cp) {
        case '&':
            o = SOP_BG;
            cp++;
            if (cp && *cp && *cp == '&') {
                o = SOP_AND;
                cp++;
            }
            break;
        case '|':
            o = SOP_PIPE;
            cp++;
            if (cp && *cp && *cp == '|') {
                o = SOP_OR;
                cp++;
            }
            break;
        case '>':
            o = SOP_REDIR_OUT;
            cp++;
            if (cp && *cp && *cp == '>') {
                o = SOP_REDIR_OUT_APPEND;
                cp++;
            }
            break;
        case '<':
            o = SOP_REDIR_IN;
            cp++;
            if (cp && *cp && *cp == '>') {
                o = SOP_REDIR_INOUT;
                cp++;
            }
            break;
        case ';':
            o = SOP_NEXT;
            cp++;
            break;
        default:
            o = SOP_NONE;
        }
    }

    *context = cp;

    return o;
}

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))
int main()
{

    const char* cmdb;
    const char* cmde;
    const char* paramsb;
    const char* paramse;
    const char* redir_begin;
    const char* redir_end;
    const char* context = NULL;

    enum shell_operator o;

    struct {
        char input[100];
        char cmd[50];
        char params[50];
        enum shell_operator oper;
        char redir[50];
    } t[] = {
        {"",                                 "",     "",                    SOP_NONE,     ""},  /* no cmd */
        {" ",                                "",     "",                    SOP_NONE,     ""},  /* empty cmd */
        {" echo >",                          "echo", "",                    SOP_REDIR_OUT,    ""},  /* no param and no redir value */
        {"   z",                             "z",    "",                    SOP_NONE,     ""},
        {"   zzz  ",                         "zzz",  "",                    SOP_NONE,     ""},
        {"   zzz z+ ",                       "zzz",  "z+",                  SOP_NONE,     ""},
        {"   [ hello there ] ",              "[",    "hello there ]",       SOP_NONE,     ""},
        {"   echo hello there  ",            "echo", "hello there",         SOP_NONE,     ""},
        {"\t echo hello - the=;  ",          "echo", "hello - the=",        SOP_NEXT,     ""},
        {"\t echo hello - \"the=; \" ",      "echo", "hello - \"the=; \"",  SOP_NONE,     ""},
        {"   echo hello there > foo.txt ",   "echo", "hello there",         SOP_REDIR_OUT,    "foo.txt"},
        {" echo   -ne  hello > 1 ",          "echo", "-ne  hello",          SOP_REDIR_OUT,    "1"},
        {" echo hello > foo .txt ; ",         "echo", "hello",              SOP_REDIR_OUT,    "foo"},
        {" echo ttha> foo.txt d ",           "echo", "ttha",                SOP_REDIR_OUT,    "foo.txt"},
        {" echo \"hello there\" >foo.txt & ","echo", "\"hello there\"",     SOP_REDIR_OUT,    "foo.txt"},
        {" echo \"; echo he>l\" >foo.txt 1", "echo", "\"; echo he>l\"",     SOP_REDIR_OUT,    "foo.txt"},
        {" echo \"\" >",                     "echo", "\"\"",                SOP_REDIR_OUT,    ""},  /* no redir value */
        {"|",                                "",     "",                    SOP_PIPE,     ""},  /* empty separator  */
        {"echo>",                            "echo", "",                    SOP_REDIR_OUT,    ""},  /* redir to empty  */
        {"echo>/dev/null",                   "echo", "",                    SOP_REDIR_OUT,    "/dev/null"},  /* no space between separator  */
        {"echo >> /dev/null && cat foo",      "echo", "",                   SOP_REDIR_OUT_APPEND,    "/dev/null"},
        {"more < /dev/null && cat foo",      "more", "",                    SOP_REDIR_IN,    "/dev/null"},
        {"more <> /dev/null && cat foo",      "more", "",                   SOP_REDIR_INOUT,    "/dev/null"},
    };

    for (int i = 0; i < NELEMS(t); i++) {
        o = shell_command_param_split(t[i].input, &cmdb, &cmde, &paramsb, &paramse, &context);
        if (o == SOP_REDIR_OUT || o == SOP_REDIR_OUT_APPEND) {
            shell_next_token(NULL, &redir_begin, &redir_end, &context);
            printf("%s  %.*s\n", 0 == strncmp(t[i].cmd, cmdb, cmde - cmdb)
                && 0 == strncmp(t[i].params, paramsb, paramse - paramsb)
                && 0 == strncmp(t[i].redir, redir_begin, redir_end - redir_begin)
                && o == t[i].oper ? "PASS" : "FAIL", (int)(redir_end - redir_begin), redir_begin);
        }
        else {
            printf("%s\n", 0 == strncmp(t[i].cmd, cmdb, cmde - cmdb) && 0 == strncmp(t[i].params, paramsb, paramse - paramsb) && o == t[i].oper ? "PASS" : "FAIL");
        }
    }

    o = shell_command_param_split("echo 1 > 1 && echo 2 > 2", &cmdb, &cmde, &paramsb, &paramse, &context);

    return 0;
}
