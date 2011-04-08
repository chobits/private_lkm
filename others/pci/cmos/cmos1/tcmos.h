#ifndef _TCMOS_H
#define _TCMOS_H
#include <linux/cdev.h>

#define NUM_CMOS_BANKS          2
#define CMOS_BANK_SIZE          (0xff * 8)
#define DEVICE_NAME             "cmos"
#define CMOS_BANK0_ADDR_PORT    0X70
#define CMOS_BANK0_DATA_PORT    0X71
#define CMOS_BANK1_ADDR_PORT    0X72
#define CMOS_BANK1_DATA_PORT    0X73

struct cmos_struct {
        unsigned short current_pointer;
        unsigned int size;
        int bank_num;
        struct cdev cmos_cdev;
        char name[10];
};


#endif /* tcmos.h */
