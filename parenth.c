#include <stdint.h>

static const char* getMatchingBrace(const char* d)
{
    uint32_t num = 1;
    char c;

    while ('\0' != (c = *d)) {
        if (c == '(') {
            num++;
        }
        else if (c == ')') {
            num--;
            if (num == 0) {
                break;
            }
        }
        ++d;
    }
    return d;
}

char* minRemoveToMakeValid(char* ss)
{
    char* s = ss;
    const char* d = ss;
    char c;
    
    while ('\0' != (c = *d)) {
        switch (c) {
            case '(': {
                const char* t = getMatchingBrace(d+1);
                if ('\0' != *t) {
                    uint32_t len = t - d + 1;
                    while (len--) {
                        *s++ = *d++;
                    }
                }
                else {
                    d++;
                }
                break;
            }
            case ')':
                d++;
                break;
            default:
                *s++ = c;
                d++;
        }
    }
    *s = '\0';
    return ss;
}

#include <stdio.h>
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))
int main()
{
    struct {
        char input[32];
        const char* result;
    } t[] = {
        {"leet(c)ode", "leet(c)ode"},
        {"lee(t(c)od(e)", "leet(c)od(e)"},
        {"lee(t(c)o)d(e(d)ab(c)", "lee(t(c)o)de(d)ab(c)" },
        {"))((", ""},
        {"a)b(c)d", "ab(c)d"},
        {"d(", "d"},
    };
    for (int i = 0; i < NELEMS(t); i++) {
        char input[32];
        strcpy(input, t[i].input);
        printf("%s %s => %s\n",
         0 == strcmp(t[i].result, minRemoveToMakeValid(input)) ? "PASS" : "FAIL",
         t[i].input, input);
    }
}