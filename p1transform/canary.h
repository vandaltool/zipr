#ifndef __CANARY_H
#define __CANARY_H

struct canary
{
    unsigned int canary_val;
    int ret_offset;//Should be negative, the value to subtract from esp if esp is at ret addr
    int esp_offset;
    int floating_offset;
};

#endif
