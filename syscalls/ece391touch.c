#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    int32_t fd;
    uint8_t buf[1024];

    if (0 != ece391_getargs (buf, 1024)) {
        ece391_fdputs (1, (uint8_t*)"could not read arguments\n");
	    return 3;
    }

    if (-1 == (fd = ece391_open (buf))) fd = -2;
    else {
        ece391_fdputs (1, (uint8_t*)"file already exists\n");
        return 2; 
    }

    if (fd == -2) {
        if (-1 == ece391_write(fd,buf,32)){
            ece391_fdputs (1, (uint8_t*)"file write failed\n");
            return 1; 
        }
    }

    return 0;
}
