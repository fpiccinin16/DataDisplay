#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   0

/*====================
   MEMORY SETTINGS
 *====================*/

#define LV_MEM_CUSTOM      0
#define LV_MEM_SIZE        (32U * 1024U)
#define LV_LAYER_MAX_MEMORY_USAGE  (24 * 1024)

/*====================
   HAL SETTINGS
 *====================*/

#define LV_USE_GPU_ESP32   0
#define LV_USE_PERF_MONITOR  1

/*====================
 * FEATURE CONFIGURATION
 *====================*/

/*1: Enable the Animations */
#define LV_USE_ANIMATION        1

/*1: Enable shadow drawing*/
#define LV_USE_SHADOW          1

/*1: Enable object groups (for keyboard/encoder navigation)*/
#define LV_USE_GROUP           1

/*1: Enable GPU interface*/
#define LV_USE_GPU            0

/*1: Enable file system (might be required for images)*/
#define LV_USE_FS_STDIO       0
#define LV_FS_STDIO_LETTER    '\0'

/* Enable timer */
#define LV_USE_TIMER          1

/*====================
 * Input device settings
 *====================*/

/* Input device default settings */
#define LV_INDEV_DEF_READ_PERIOD          30
#define LV_INDEV_DEF_DRAG_LIMIT           10
#define LV_INDEV_DEF_DRAG_THROW           10
#define LV_INDEV_DEF_LONG_PRESS_TIME      400
#define LV_INDEV_DEF_LONG_PRESS_REP_TIME  100
#define LV_INDEV_DEF_GESTURE_LIMIT        50
#define LV_INDEV_DEF_GESTURE_MIN_VELOCITY 3

/*====================
 * FONT USAGE
 *====================*/

/* Enable built-in fonts */
#define LV_FONT_MONTSERRAT_8     0
#define LV_FONT_MONTSERRAT_10    0
#define LV_FONT_MONTSERRAT_12    1
#define LV_FONT_MONTSERRAT_14    0
#define LV_FONT_MONTSERRAT_16    1
#define LV_FONT_MONTSERRAT_18    0
#define LV_FONT_MONTSERRAT_20    0
#define LV_FONT_MONTSERRAT_22    0
#define LV_FONT_MONTSERRAT_24    0
#define LV_FONT_MONTSERRAT_26    0
#define LV_FONT_MONTSERRAT_28    0
#define LV_FONT_MONTSERRAT_30    0
#define LV_FONT_MONTSERRAT_32    0
#define LV_FONT_MONTSERRAT_34    0
#define LV_FONT_MONTSERRAT_36    0
#define LV_FONT_MONTSERRAT_38    0
#define LV_FONT_MONTSERRAT_40    0
#define LV_FONT_MONTSERRAT_42    0
#define LV_FONT_MONTSERRAT_44    0
#define LV_FONT_MONTSERRAT_46    0
#define LV_FONT_MONTSERRAT_48    0

/* Default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_16

/* Enable font handling */
#define LV_USE_FONT_COMPRESSED    1
#define LV_USE_FONT_SUBPX        1
#define LV_FONT_SUBPX_BGR        0

/*====================
 * WIDGET USAGE
 *====================*/
#define LV_USE_ARC            1
#define LV_USE_BAR            1
#define LV_USE_BTN            1
#define LV_USE_BTNMATRIX      1
#define LV_USE_CANVAS         1
#define LV_USE_CHECKBOX       1
#define LV_USE_DROPDOWN       1
#define LV_USE_IMG           1
#define LV_USE_LABEL          1
#define LV_USE_LINE           1
#define LV_USE_ROLLER         1
#define LV_USE_SLIDER         1
#define LV_USE_SWITCH        1
#define LV_USE_TEXTAREA      1
#define LV_USE_TABLE         1
#define LV_USE_CHART         1

/*==================
 * EXAMPLES
 *==================*/

/*Enable the examples to be built with the library*/
#define LV_BUILD_EXAMPLES   0

/*--END OF LV_CONF_H--*/
#endif /*LV_CONF_H*/ 