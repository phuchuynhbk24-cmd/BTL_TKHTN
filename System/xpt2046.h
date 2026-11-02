#ifndef XPT2046_H
#define XPT2046_H

#include "sys_types.h"
#include <stdint.h>

/* Orientation options (Change to 1 if your touch glass axis is inverted or swapped) */
#define TOUCH_SWAP_XY    0  /* 1 if X and Y axes are swapped */
#define TOUCH_INVERT_X   0  /* 1 if X coordinate is mirrored (Right to Left) */
#define TOUCH_INVERT_Y   1  /* 1: Invert Y axis (Fixes Top-Bottom flipped touch) */

sys_status_t xpt2046_init(void);
/* Returns SYS_OK if screen is currently pressed and coordinates are valid, SYS_ERROR otherwise */
sys_status_t xpt2046_read(uint16_t *x, uint16_t *y);

#endif /* XPT2046_H */
