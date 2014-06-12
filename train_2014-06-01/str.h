#ifndef STR_H
#define STR_H

#include "stdio.h"

struct str_splited_pair{
    char * start;
    char * end;
};

//max_count >= 2
int _split2(const char * str_begin, const char * str_end, const char * delims, int delim_count, struct str_splited_pair * result, int max_count){
    if(max_count < 2) return -1;
    int char_cnt = 0;
    int counter = 0;
    for(const char * cursor = str_begin; cursor != str_end; cursor++){
        for(int d = 0; d < delim_count; ++d){
            if(*cursor == delims[d]){
                if(char_cnt > 0){
                    result[counter].start = (char*) cursor - char_cnt;
                    result[counter].end = (char*) cursor;
                    counter++;
                    if(counter == max_count){
                        result[counter - 1].end = (char*) str_end;
                        return counter;
                    }
                }
                char_cnt = -1;
            }
        }
        char_cnt++;
    }
    if(char_cnt > 0){
        result[counter].start = (char*) str_end - char_cnt;
        result[counter].end = (char*) str_end;
        counter++;
    }
    return counter;
}

//result - array of pairs of const char *
int _split(const char * str, const char * delims, struct str_splited_pair * result, int max_count){
    return _split2(str, str + strlen(str), delims, strlen(delims), result, max_count);
}


#endif
