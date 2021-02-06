#include <stdio.h>
#include <string.h>

/* reduce a POSIX absolute path, write in place. input must be a
   valid nul terminated string. Must begin with with root
   directory */
char* simplify_path(char* wr)
{
    char* root = wr;
    const char* rd1 = wr;
    const char* rd2;

    if (*root != '/') {
        return NULL;   /* exception */
    }

    ++wr;

    while (*rd1) {
        for (rd2 = ++rd1; *rd2 && *rd2 != '/'; rd2++); /* find end of this fragment */

        if (rd2 == rd1 || 0 == strncmp(".", rd1, rd2 - rd1)) {
            /* nothing to copy */
        }
        else if (0 == strncmp("..", rd1, rd2 - rd1)) {
            /* reverse travel without exiting the root */
            if ((wr-1) != root) {
                wr -= 2;
                while (*wr != '/') {
                    wr--;
                }
                wr++;
            }
        }
        else {
            /* write the fragment */
            while (*rd1 && rd1 <= rd2) {*wr++ = *rd1++;}
        }
        rd1 = rd2;
    }

    if (*wr) {
        *wr = '\0';
    }

    if (root != --wr && *wr == '/') {  /* remove the trailing path separator */
        *wr = '\0';
    }

    return root;
}

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))
int main()
{
    struct {
        char input[32];
        const char* result;
    } t[] = {
        {"/foo", "/foo"},
        {"/...", "/..."},
        {"/../", "/" },
        {"/a//b////c/d//././/..", "/a/b/c"},
        {"/a/./b/../../c/", "/c"},
        {"/home//foo/", "/home/foo"},
        {"/foo/../bar", "/bar"},
        {"/foo/.bar", "/foo/.bar"},
        {"/foo//bar", "/foo/bar"},
        {"/foo/./bar", "/foo/bar"},
        {"/foo/..", "/"},
        {"/foo/.", "/foo"},
    };
    for (int i = 0; i < NELEMS(t); i++) {
        printf("%s\n", 0 == strcmp(t[i].result, simplify_path(t[i].input)) ? "PASS" : "FAIL");
    }
}
