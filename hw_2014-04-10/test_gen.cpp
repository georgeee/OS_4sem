#include <cstdlib>
#include <cstdio>

int main(int argc, char ** argv){
    int len;
    sscanf(argv[1], "%d", &len);
    printf("first line\n");
    for(int j=0; (1<<j)<=len; ++j){
        for(int i=0; i < (len >> j); ++i){
            for(char ch = 'a'; ch <='z'; ch++)
                putchar(ch);
        }
        putchar('\n');
    }
    printf("\n\nlast line");
    return 0;
}
