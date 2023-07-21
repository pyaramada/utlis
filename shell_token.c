/******************************************************************************
  @file   shell_token.c
  @brief

  DESCRIPTION: utility to extract ash compatible tokens.
  <command>[params][operators]

****************************************************************************/
#include <stddef.h>

/* Redirection operators */
enum shell_redir {
    SOP_REDIR_NONE,
    SOP_REDIR_OUT_APPEND,   /* >> */
    SOP_REDIR_OUT,          /* > */
    SOP_REDIR_IN,           /* < */
    SOP_REDIR_INOUT,        /* <> */
};

/* Control operators */
enum shell_operator {
    SOP_NONE,
    SOP_AND,                /* logical && */
    SOP_OR,                 /* logical || */
    SOP_BG,                 /* background */
    SOP_PIPE,               /* | */
    SOP_NEXT,               /* ; */
};

/**
 * Split a simple form of shell input command string. Given the input string
 * mark various offsets for command, parameter and i/o redirection and 
 * continuation operators.
 *  
 * Form of input string comprises of 
 *   <command> [ params][redirecton][control operators]
 * 
 *  Token separator is
 *  - blanks : spaces and tabs
 *  - control operators
 *  {'&', '|', '&&', '||', ';'}
 *  - redirection operators
 *  {'>', '>>', '<', '<>'}
 * 
 *  Quoting makes an exception to the token separator. Matching
 *  double quotes or single quotes
 *  
 *  USAGE:
 *  
 *      const char *cmdb, *cmde, *paramsb, *paramse, *redir_begin, *redir_end;
 *      const char* context = NULL;
 *      enum shell_redir r;
 *      enum shell_operator o;
 *
 *      const char* input = "echo 1 > /dev/foo && echo 2 > /dev/bar";
 *      do {
 *          o = shell_command_param_split(input, &cmdb, &cmde, &paramsb, &paramse, &r, &redir_begin, &redir_end, &context);
 *          // handle the command
 *          input = NULL;
 *      } while (o != SOP_NONE);
 *
 * 
 * @param input : input string
 * @param cmd_begin 
 * @param cmd_end 
 * @param params_begin 
 * @param params_end 
 * @param sop_redir 
 * @param redir_begin 
 * @param redir_end 
 * @param context 
 * 
 * @return enum shell_operator 
 */
enum shell_operator shell_command_param_split(const char* input,
    const char** cmd_begin, const char** cmd_end,
    const char** params_begin, const char** params_end,
    enum shell_redir* sop_redir, const char** redir_begin, const char** redir_end,
    const char** context);

/* IMPLEMENTATION */

#define EAT_BLANK(p) while (p && (*p == ' ' || *p == '\t')) p++;
#define GET_CHAR(p, c) while (p && *p && *p != c) p++;
#define GET_SEPER(p) while (p && *p && *p != ' ' && *p != '\t' && *p != '>' && *p != '<' && *p != '|' && *p != '&' && *p != ';') { if (*p == '\"') { p++; GET_CHAR(p, '\"'); if (*p) p++; } else p++; }

enum shell_operator shell_command_param_split(const char* input, const char** cmd_begin, const char** cmd_end,
    const char** params_begin, const char** params_end, enum shell_redir* sop_redir, const char** redir_begin,
    const char** redir_end, const char** context)
{
    enum shell_operator o = SOP_NONE;
    const char* cp = (input != NULL) ? input : *context;

    *cmd_begin = NULL;
    *cmd_end = NULL;
    *params_begin = NULL;
    *params_end = NULL;
    *sop_redir = SOP_REDIR_NONE;
    *redir_begin = NULL;
    *redir_end = NULL;

    EAT_BLANK(cp);   /* seek command */
    *cmd_end = *cmd_begin = cp; /* initialize command begin and end here */
    if (*cp) {
        GET_SEPER(cp);  /* seek to end of command */
        *cmd_end = cp;
        if (*cp && *cp == ' ') {
            EAT_BLANK(cp);   /* seek parameter */
            *params_end = *params_begin = cp;
            if (*cp) {
                GET_SEPER(cp);
                *params_end = cp;
                while (*cp && *cp == ' ') {
                    EAT_BLANK(cp);   /* seek next */
                    GET_SEPER(cp);
                    if (*cp && *cp == ' ') {
                        *params_end = cp;
                    }
                }
            }
        }
        switch (*cp) {
        case '>':
            *sop_redir = SOP_REDIR_OUT;
            cp++;
            if (cp && *cp && *cp == '>') {
                *sop_redir = SOP_REDIR_OUT_APPEND;
                cp++;
            }
            break;
        case '<':
            *sop_redir = SOP_REDIR_IN;
            cp++;
            if (cp && *cp && *cp == '>') {
                *sop_redir = SOP_REDIR_INOUT;
                cp++;
            }
            break;
        default:
            *sop_redir = SOP_REDIR_NONE;
        }

        if (*sop_redir != SOP_REDIR_NONE && *cp) {
            EAT_BLANK(cp);   /* seek */
            *redir_end = *redir_begin = cp;
            if (*cp) {
                GET_SEPER(cp);
                *redir_end = cp;
                EAT_BLANK(cp);
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

#ifdef BUILD_TEST
#if 0
void shell_next_token(const char* input, const char** token_begin, const char** token_end, const char** context)
{
    const char* cp = (input != NULL) ? input : *context;

    EAT_BLANK(cp);   /* seek */
    *token_end = *token_begin = cp;
    if (*cp) {
        GET_SEPER(cp);
        *token_end = cp;
    }

    *context = cp;
}
#endif

#include <stdio.h>
#include <string.h>

#define MAX_CMDS_IN_ONE_INPUT 4
#define MAX_INPUT_BUFSIZ      400
#define MAX_FRAG_BUFSIZ       200
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))
int main()
{

    const char* cmdb;
    const char* cmde;
    const char* paramsb;
    const char* paramse;
    enum shell_redir r;
    const char* redir_begin;
    const char* redir_end;
    const char* context = NULL;

    enum shell_operator o;

    struct {
        char input[MAX_INPUT_BUFSIZ];
        struct {
            char cmd[MAX_FRAG_BUFSIZ];
            char params[MAX_INPUT_BUFSIZ];
            enum shell_operator oper;
            enum shell_redir redir_kind;
            char redir[MAX_FRAG_BUFSIZ];
        } shell_cmd[MAX_CMDS_IN_ONE_INPUT];
    } t[] = {
        {"echo 2 > /proc/sys/net/ipv4/conf/bridge0.1/arp_ignore",
                                                {"echo", "2",                   SOP_NONE,       SOP_REDIR_OUT,         "/proc/sys/net/ipv4/conf/bridge0.1/arp_ignore"}},
        {"",                                    {"",     "",                    SOP_NONE,       SOP_REDIR_NONE,        ""}}, /* no cmd */
        {" ",                                   {"",     "",                    SOP_NONE,       SOP_REDIR_NONE,        ""}},  /* empty cmd */
        {" echo >",                             {"echo", "",                    SOP_NONE,       SOP_REDIR_OUT,         ""}},  /* no param and no redir value */
        {"   z",                                {"z",    "",                    SOP_NONE,       SOP_REDIR_NONE,        ""}},
        {"   zzz  ",                            {"zzz",  "",                    SOP_NONE,       SOP_REDIR_NONE,        ""}},
        {"   zzz z+ ",                          {"zzz",  "z+",                  SOP_NONE,       SOP_REDIR_NONE,        ""}},
        {"   [ hello there ] ",                 {"[",    "hello there ]",       SOP_NONE,       SOP_REDIR_NONE,        ""}},
        {"   echo hello there  ",               {"echo", "hello there",         SOP_NONE,       SOP_REDIR_NONE,        ""}},
        {"\t echo hello - the=;  ",             {"echo", "hello - the=",        SOP_NEXT,       SOP_REDIR_NONE,        ""}},
        {"\t echo hello - \"the=; \" ",         {"echo", "hello - \"the=; \"",  SOP_NONE,       SOP_REDIR_NONE,        ""}},
        {"   echo hello there > foo.txt ",      {"echo", "hello there",         SOP_NONE,       SOP_REDIR_OUT,         "foo.txt"}},
        {" echo   -ne  hello > 1 ",             {"echo", "-ne  hello",          SOP_NONE,       SOP_REDIR_OUT,         "1"}},
        {" echo hello > foo .txt ; ",           { "echo", "hello",              SOP_NONE,       SOP_REDIR_OUT,         "foo"}},
        {" echo ttha> foo.txt d ",              {"echo", "ttha",                SOP_NONE,       SOP_REDIR_OUT,         "foo.txt"}},
        {" echo \"hello there\" >foo.txt & ",   {"echo", "\"hello there\"",     SOP_BG,         SOP_REDIR_OUT,         "foo.txt"}},
        {" echo \"; echo he>l\" >foo.txt 1",    {"echo", "\"; echo he>l\"",     SOP_NONE,       SOP_REDIR_OUT,         "foo.txt"}},
        {" echo \"\" >",                        {"echo", "\"\"",                SOP_NONE,       SOP_REDIR_OUT,         ""}},  /* no redir value */
        {"|",                                   {"",     "",                    SOP_PIPE,       SOP_REDIR_NONE,        ""}},  /* empty separator  */
        {"echo>",                               {"echo", "",                    SOP_NONE,       SOP_REDIR_OUT,         ""}},  /* redir to empty  */
        {"echo>/dev/null",                      {"echo", "",                    SOP_NONE,       SOP_REDIR_OUT,         "/dev/null"}},  /* no space between separator  */
        {"echo >> /dev/null && cat foo",       {{ "echo", "",                   SOP_AND,        SOP_REDIR_OUT_APPEND,  "/dev/null"}, { "cat", "foo", SOP_NONE, SOP_REDIR_NONE,  ""}}},
        {"more < /dev/null && cat foo",        {{"more", "",                    SOP_AND,        SOP_REDIR_IN,          "/dev/null"}, { "cat", "foo", SOP_NONE, SOP_REDIR_NONE,  ""}}},
        {"more <> /dev/null && cat foo",       {{"more", "",                    SOP_AND,        SOP_REDIR_INOUT,       "/dev/null"}, { "cat", "foo", SOP_NONE, SOP_REDIR_NONE,  ""}}},
        {"echo 1 > /dev/foo "
         " && echo 2 > /dev/bar&&echo 3 >>tree"
        "&& cat foo",                          {{ "echo", "1",                  SOP_AND,        SOP_REDIR_OUT,          "/dev/foo"},
                                                { "echo", "2",                  SOP_AND,        SOP_REDIR_OUT,          "/dev/bar"},
                                                { "echo", "3",                  SOP_AND,        SOP_REDIR_OUT_APPEND,   "tree"},
                                                { "cat",  "foo",                SOP_NONE,       SOP_REDIR_NONE,         ""}}},
        {"echo dnsmasq --conf-file=/etc/data/dnsmasq.conf"
        " --dhcp-leasefile=/var/run/data/dnsmasq.leases"
        " --addn-hosts=/etc/data/hosts --pid-file=/var/run/data/dnsmasq.pid"
        " -i bridge0 -I lo -z --dhcp-script=/bin/dnsmasq_script.sh type_inst=dnsv4"
        " > /var/run/data/dnsmasq_env.conf",    {"echo", "dnsmasq --conf-file=/etc/data/dnsmasq.conf --dhcp-leasefile=/var/run/data/dnsmasq.leases"
                                                         " --addn-hosts=/etc/data/hosts --pid-file=/var/run/data/dnsmasq.pid -i bridge0 -I lo -z"
                                                         " --dhcp-script=/bin/dnsmasq_script.sh type_inst=dnsv4",
                                                                                SOP_NONE,       SOP_REDIR_OUT,          "/var/run/data/dnsmasq_env.conf"}},
        {"echo QCMAP:qcmap_netlink_thread Entry for pid:922, tid:1035, ppid:1 > /dev/kmsg",
                                                {"echo", "QCMAP:qcmap_netlink_thread Entry for pid:922, tid:1035, ppid:1",
                                                                                SOP_NONE,       SOP_REDIR_OUT,          "/dev/kmsg"}},
        {"echo 0 > /proc/sys/net/ipv4/conf/bridge5/proxy_arp"
        " && echo 0 > /proc/sys/net/ipv4/conf/bridge5/forwarding"
        " && echo 1 > /proc/sys/net/ipv4/neigh/default/neigh_probe"
        " && echo 1 > /proc/sys/net/ipv6/neigh/default/neigh_probe",
                                               {{ "echo", "0",                  SOP_AND,        SOP_REDIR_OUT,          "/proc/sys/net/ipv4/conf/bridge5/proxy_arp"},
                                                { "echo", "0",                  SOP_AND,        SOP_REDIR_OUT,          "/proc/sys/net/ipv4/conf/bridge5/forwarding"},
                                                { "echo", "1",                  SOP_AND,        SOP_REDIR_OUT,          "/proc/sys/net/ipv4/neigh/default/neigh_probe"},
                                                { "echo", "1",                  SOP_NONE,       SOP_REDIR_OUT,          "/proc/sys/net/ipv6/neigh/default/neigh_probe"}}},
    };

    for (int i = 0; i < NELEMS(t); i++) {
        int j = 0;
        const char* input = t[i].input;

        printf("\n %d : ", i);
        do {
            o = shell_command_param_split(input, &cmdb, &cmde, &paramsb, &paramse, &r, &redir_begin, &redir_end, &context);
            if (r != SOP_REDIR_NONE) {
                printf(" %s  %.*s", 0 == strncmp(t[i].shell_cmd[j].cmd, cmdb, cmde - cmdb)
                    && 0 == strncmp(t[i].shell_cmd[j].params, paramsb, paramse - paramsb)
                    && 0 == strncmp(t[i].shell_cmd[j].redir, redir_begin, redir_end - redir_begin)
                    && r == t[i].shell_cmd[j].redir_kind
                    && o == t[i].shell_cmd[j].oper ? "PASS" : "FAIL", (int)(redir_end - redir_begin), redir_begin);
            }
            else {
                printf(" %s", 0 == strncmp(t[i].shell_cmd[j].cmd, cmdb, cmde - cmdb) && 0 == strncmp(t[i].shell_cmd[j].params, paramsb, paramse - paramsb) && o == t[i].shell_cmd[j].oper ? "PASS" : "FAIL");
            }
            input = NULL;
        } while (o != SOP_NONE && ++j < MAX_CMDS_IN_ONE_INPUT);
    }

    return 0;
}
#endif