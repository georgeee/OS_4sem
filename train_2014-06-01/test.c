#include "unistd.h"
#include "stdio.h"

int main(){
    for(int i = 0; i < 10; ++i){
        int pipefds[2];
        int ret = pipe(pipefds);
        fprintf(stderr, "ret=%d [0]=%d [1]=%d\n", ret, pipefds[0], pipefds[1]);
    }
    return 0;
}
