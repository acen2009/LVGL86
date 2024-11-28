#ifndef _X86_H
#define _X86_H

#include <stdio.h>
#include "Arduino.h"
#include "../lv_conf.h"
#include "lvgl.h"
#include "src/lv_hal_indev.h"
#include "src/lv_hal_disp.h"
#include "touchpanel.h"
#include "lv_gif.h"
#include "theme.h"
#include "imgbtnplugin.h"
#include "lv_osc.h"
#include "lv_filebrowser.h"
#include "lv_textlist.h"

#define DMP_LCD 0
#define DMP_VGA 1

#define QEC_LCD          (0)
#define QECM2_LCD        (1)
#define QEC_VGA          (2)
#define QEC7_VGA         (3)
#define QEC9_VGA         (4)
#define QEC15_VGA        (5)
#define DUO_LCD          (6)
#define DUO_VGA          (7)
#define VGA_1024X600     (7) // same as DUO_VGA
#define VGA_800X480      (8)
#define QECM02_VGA       (9)
#define QECPPC104_VGA    (10)
#define NO_SCREEN        (99)

#define LVGL86_DEFAULT_FONT "a:\\resource\\font\\microhei.ttc"

void lv86_init();
void lv86_init(int rotate);
void lv86_init(int device, int rotate);
void lv86_init(int device, bool touch, int rotate);
void lv86_init(int device, bool touch, bool showCur, int rotate);
void lv86_init(int disp, unsigned int hor, unsigned int ver, int rotate);
void lv86_init(int disp, unsigned int hor, unsigned int ver, bool touch, bool showCur, bool touchmirror, int rotate);
void lv86_deinit(void);

int lv86_getScreenW(void);
int lv86_getScreenH(void);

void lv86_setIconSize(int size=14);
lv_font_t* lv86SetFont(const char* font_path, unsigned short size);
lv_style_t* lv86FontSet(lv_obj_t* obj, lv_font_t* font);
// lv_obj_t* lv86LoadImg(const char * img_path, lv_obj_t* obj=NULL);
// void lv86ChangeImg(lv_obj_t* obj, const char * img_path);
lv_obj_t* lv86MakeQRCode(lv_obj_t * parent, lv_coord_t size, const void * data, unsigned long data_len);
void lv86UpdateQRCode(lv_obj_t *qrobj, const void * data, unsigned long data_len);
void lv86ClearQRCode(lv_obj_t *qrobj);
void lv86SetBacklight(unsigned char percent);
lv_group_t* lv86GetGroup();

class Console : public Stream
{
    public:
        Console();
        bool begin(unsigned long color, bool graduation=true, lv_obj_t* parent=NULL);
        bool beginEX(unsigned long color, unsigned int w, unsigned int h, unsigned int x, unsigned int y, bool graduation=true, lv_obj_t* parent=NULL);
        bool begin(void);
        void end();
        void scrollbarON(void);
        void scrollbarOFF(void);
        void setBufSize(unsigned int s);
        bool setFont(const char* font_path, unsigned short size);
        void setFontColor(unsigned long color);
        void fflush(void);
        size_t write(const unsigned char *buffer, size_t size);
        virtual size_t write(unsigned char);
        virtual int available(void);
        virtual int peek(void);
        virtual int read(void);
        using Print::write; // pull in write(str) and write(buf, size) from Print
        // operator bool();
    private:
        char myWord[2500] = {'\0'};
        lv_obj_t * ta1;
        lv_style_t* ta_style;
        lv_style_t* font_style;
        lv_color_t font_color;
        bool begined;
        unsigned long taMaxLength;
        unsigned long taIndex;
        lv_font_t* consoleFont;
};
extern Console console;

unsigned int get_x86_hor_resolution(void);
unsigned int get_x86_ver_resolution(void);

bool get_x86_touchXY(int* x, int* y);
void get_x86_tp_point(int* x, int* y, bool* press);

#endif
