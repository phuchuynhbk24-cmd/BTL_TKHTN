#ifndef SYS_TYPES_H
#define SYS_TYPES_H

#include <stdint.h>

/* Independent system status codes (System Software Layer to Application Layer contract) */
typedef enum {
    SYS_OK = 0,
    SYS_ERROR,
    SYS_BUSY,
    SYS_TIMEOUT
} sys_status_t;

#endif /* SYS_TYPES_H */
