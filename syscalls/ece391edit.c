#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

#define NULL 0

int main ()
{
    int32_t fd;
    uint8_t buf[1024];
    uint8_t fname_buf[32];
    uint8_t data_buf[992];
    uint16_t i, j;
    uint8_t data_buf_idx = 0;
    //uint8_t start_data_buf = 0;

    for (i = 0; i < 32; i++) {
        fname_buf[i] = NULL;
    }
    for (i = 0; i < 992; i++) {
        data_buf[i] = NULL;
    }

    if (0 != ece391_getargs (buf, 1024)) {
        ece391_fdputs (1, (uint8_t*)"could not read arguments\n");
	    return 3;
    }

    // int8_t flag = 0;
    // for (i = 0; i < 128; i++) {
    //     if (buf[i] != ' ' && flag == 0) {
    //         fname_buf[i] = buf[i];
    //         start_data_buf = i;
    //         continue;
    //     }
    //     flag = 1;
    //     data_buf[i - start_data_buf - 1] = buf[i];
    // }

    i = 0;

    //ece391_fdputs (1, buf);
    while(buf[i] != ' '){
        if(i >= 128){
            break;
        }
        fname_buf[i] = buf[i];
        i++;
    }
    //ece391_fdputs (1, fname_buf);

    for(j = i + 1; j < 128; j++){
        if(buf[j] == 0){
            data_buf[data_buf_idx] = '\n';
            break;
        }
        data_buf[data_buf_idx] = buf[j];
        data_buf_idx++;
    }
    //ece391_fdputs (1, data_buf);

    if (-1 == (fd = ece391_open (fname_buf))) {
        ece391_fdputs (1, (uint8_t*)"File does not exist. Please 'touch' first.\n");
        return 2;
    }

    if (-1 == ece391_write(fd,data_buf,992)){
        ece391_fdputs (1, (uint8_t*)"file write failed\n");
        return 1; 
    }

    return 0;
}
