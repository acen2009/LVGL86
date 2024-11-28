#include <sys/farptr.h>

#include "lvgl86.h"
#include "a9160.h"
#include "lcd.h"
#include "irq.h"

#include "lv_lodepng.h"
#include "lv_freetype.h"
#include "lv_sjpg.h"
#include "lv_bmp.h"
#include "lv_qrcode.h"

#define ALLEGRO_HAVE_STDINT_H
#include <allegro.h> // Need it when we use the USB mouse without touch panel

#ifdef __cplusplus
extern "C" {
#endif
void set_pwm_level(unsigned char level); // this function is in lcdbacklight.c
#ifdef __cplusplus
}
#endif

#if (LV_COLOR_DEPTH == 8)
    #define PIXEL_SIZE 1
#elif (LV_COLOR_DEPTH == 16)
    #define PIXEL_SIZE 2
#elif (LV_COLOR_DEPTH == 32)
    #define PIXEL_SIZE 4
#endif

#if defined (__86DUINO_QEC)
    #define USED_BOARD    (QEC_LCD)
#elif defined (__86DUINO_QEC_M2)
    #define USED_BOARD    (QECM2_LCD)
#elif defined (__86DUINO_QEC_M02)
    #define USED_BOARD    (QECM02_VGA)
#elif defined (__86DUINO_QEC_7)
    #define USED_BOARD    (QEC7_VGA)
#elif defined (__86DUINO_QEC_9)
    #define USED_BOARD    (QEC9_VGA)
#elif defined (__86DUINO_QEC_15)
    #define USED_BOARD    (QEC15_VGA)
#elif defined (__86DUINO_DUO)
    #define USED_BOARD    (DUO_LCD)
#elif defined (__86DUINO_ONE)
    #define USED_BOARD    (VGA_1024X600)
#elif defined (__86DUINO_QEC_PPC104)
    #define USED_BOARD    (QECPPC104_VGA)
#else
    #error "no select any 86Duino board."
#endif

static int selector = -1;
static void getTPCal(void);
static int _vx86_disp = -1;
static bool _touch_used = false;
static bool _kbms_used = false;
static bool initiallv86 = false;
static lv_font_t* globalFont = NULL;

unsigned int DRW	= 1024;  // display resolution width  only supports 800 /600 pixels
unsigned int DRH	= 600;   // display resolution height  only supports  600 / 480 line
unsigned int DRC	= 16;    // display resolution color depth only supports 8 / 16 /32 bit

static lv_disp_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;
static lv_indev_drv_t kb_drv;
static lv_indev_drv_t enc_drv;
// static lv_color_t buf[1024*600];

lv_indev_t* mouse_device_object = NULL;
lv_indev_t* keyboard_device_object = NULL;
lv_indev_t* wheel_device_object = NULL;
lv_group_t* _group = NULL;

static bool pointer_event(lv_indev_drv_t * indev, lv_indev_data_t * data);
static bool keyboard_event(lv_indev_drv_t * indev, lv_indev_data_t * data);
static bool wheel_event(lv_indev_drv_t * indev, lv_indev_data_t * data);
static void x86_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
char fontswesomePath[] = "a:\\resource\\font\\icon.ttf";
void lv86_init(int disp, unsigned int hor, unsigned int ver, bool touchPanel, bool showCur, bool useKbMs, int rotate) {
    bool enableMouse;
    
    lv_color_t* buf = (lv_color_t*) malloc(sizeof(lv_color_t)*hor*ver);
    if (buf == NULL) {
        printf("*** lv86_init malloc return NULL ***\n");
        return;
    }
    if (initiallv86) {printf("lv86_init is ready.\n"); return;}
    
    DRW = hor;
    DRH = ver;
    DRC = LV_COLOR_DEPTH;
    
    lv_init();
    
    lv_freetype_init(fontswesomePath); // use FontAwesome Icon
    lv_lodepng_init();
    lv_split_jpeg_init();
    lv_bmp_init();
    
    lv_disp_buf_init(&disp_buf, buf, NULL, hor * ver);
    
    if (disp == DMP_VGA) {
        a9160_Init(hor, ver);
        selector = get_a9160_fb_selector();
        #if defined (__86DUINO_ONE)
        touchPanel = false;
        useKbMs = true;
        #endif
    } else if (disp == DMP_LCD) {
        if (hor == 1280 && ver == 1024) lcd_on(0);
        else if (hor == 1024 && ver == 600) lcd_on(1);
        else if (hor == 800 && ver == 480) lcd_on(2);
        else if (hor == 640 && ver == 480) lcd_on(3);
        else lcd_on(2);
        selector = get_lcd_fb_selector();
    } else if (disp == NO_SCREEN) {
        printf("Do nothing for display init\n");
    } else {
        printf("No find the display device.\n");
        return;
    }
    
    _vx86_disp = disp;
    
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = DRW;
    disp_drv.ver_res = DRH;
    disp_drv.flush_cb = x86_disp_flush;
    if (rotate == 90) {
        disp_drv.sw_rotate = 1;
        disp_drv.rotated = LV_DISP_ROT_90;
    } else if (rotate == 270) {
        disp_drv.sw_rotate = 1;
        disp_drv.rotated = LV_DISP_ROT_270;
    } else if (rotate == 180) {
        disp_drv.sw_rotate = 1;
        disp_drv.rotated = LV_DISP_ROT_180;
    } else {
        disp_drv.sw_rotate = 0;
        disp_drv.rotated = LV_DISP_ROT_NONE;
    }
    /*
    disp_drv.antialiasing = true;
    disp_drv.color_chroma_key = LV_COLOR_TRANSP;
    disp_drv.user_data = NULL;
    disp_drv.set_px_cb = NULL;
    */
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);
    
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = pointer_event;
    mouse_device_object = lv_indev_drv_register(&indev_drv);
    // Is show mouse cursor
    if (useKbMs || showCur) {
        LV_IMG_DECLARE(mouse_cursor_icon);                         /*Declare the image file.*/
        lv_obj_t * cursor_obj =  lv_img_create(lv_scr_act(), NULL); /*Create an image object for the cursor */
        lv_img_set_src(cursor_obj, &mouse_cursor_icon);             /*Set the image source*/
        lv_indev_set_cursor(mouse_device_object, cursor_obj);       /*Connect the image  object to the driver*/
    }

    if (useKbMs) {
        // Initialize the Allegro Mouse Driver
        allegro_init();
        install_mouse();
        install_keyboard();
        set_mouse_range(0, 0, 0x3FFF-1, 0x3FFF-1);
        
        // Initialize KeyBoard
        lv_indev_drv_init(&kb_drv);
        kb_drv.type = LV_INDEV_TYPE_KEYPAD;
        kb_drv.read_cb = keyboard_event;
        keyboard_device_object = lv_indev_drv_register(&kb_drv);

        // Initialize Mouse wheel
        // lv_indev_drv_init(&enc_drv);
        // enc_drv.type = LV_INDEV_TYPE_ENCODER;
        // enc_drv.read_cb = wheel_event;
        // wheel_device_object = lv_indev_drv_register(&enc_drv);

        _kbms_used = useKbMs; // true
    }
    
    if (touchPanel) {
        getTPCal();
        touchpanel_init(); // 86Duino 9" LCD touch
        _touch_used = touchPanel; // true
    }

    _group = lv_group_create();
    if (_group != NULL) {
        if (mouse_device_object)    lv_indev_set_group(mouse_device_object, _group);
        if (keyboard_device_object) lv_indev_set_group(keyboard_device_object, _group);
        // if (wheel_device_object)    lv_indev_set_group(wheel_device_object, _group);
    }
    
    initiallv86 = true;
}


    
void lv86_init(int disp, unsigned int hor, unsigned int ver, int rotate) {
    lv86_init(disp, hor, ver, false, false, true, rotate);
}

void lv86_init(int device, bool touch, bool showCur, int rotate) {
    switch (device) {
        // case TEST_LCD19:
            // lv86_init(DMP_LCD, 1280, 1024, false, showCur, true, rotate); // 19"
        // break;
        case QEC_LCD:
            lv86_init(DMP_LCD, 800, 480, touch, showCur, false, rotate); // 4.3"
        break;
        case QECM2_LCD:
            lv86_init(NO_SCREEN, 800, 480, false, false, false, rotate); // no screen
        break;
        case QEC_VGA:
            lv86_init(DMP_VGA, 800, 480, false, false, false, rotate); // 4.3"
        break;
        case QECM02_VGA:
            // lv86_init(DMP_VGA, 1024, 768, false, true, true, rotate); // VGA to HDMI
            lv86_init(DMP_VGA, 1280, 720, false, true, true, rotate); // VGA to HDMI
        break;
        case QEC7_VGA:
            lv86_init(DMP_VGA, 800, 480, touch, showCur, false, rotate); // 7"
        break;
        case QEC9_VGA:
            lv86_init(DMP_VGA, 800, 480, touch, showCur, false, rotate); // 9"
        break;
        case QEC15_VGA:
            lv86_init(DMP_VGA, 1024, 768, touch, showCur, false, rotate); // 15"
        break;
        case QECPPC104_VGA:
            lv86_init(DMP_VGA, 800, 600, true, false, false, rotate);
        break;
        case DUO_LCD:
            lv86_init(DMP_LCD, 1024, 600, touch, showCur, true, rotate); // 9"
        break;
        case DUO_VGA: //  DUO_VGA is same as VGA_1024X600
            lv86_init(DMP_VGA, 1024, 600, false, false, true, rotate);
        break;
        case VGA_800X480:
            lv86_init(DMP_VGA, 800, 480, false, false, true, rotate);
        break;
        default:
            printf("No find this DMP Device\n");
        break;
    }
}

void lv86_init(int device, bool touch, int rotate) {
    lv86_init(device, touch, false, rotate);
}

void lv86_init(int device, int rotate) {
    lv86_init(device, true, rotate);
}

void lv86_init(int rotate) {
    lv86_init(USED_BOARD, rotate);
}

void lv86_init() {
    lv86_init(USED_BOARD, 0);
}

void lv86_deinit(void) {
    if (!initiallv86) return;
    
    if (_vx86_disp == DMP_VGA) {
        a9160_Close();
    } else if (_vx86_disp == DMP_LCD) {
        lcd_off();
    } else {
        printf("No this display, so do nothing.\n");
    }
}

int lv86_getScreenW(void) {
    return DRW;
}

int lv86_getScreenH(void) {
    return DRH;
}

void lv86_setIconSize(int size) {
    if (!initiallv86) return;
    
    changeIconSize(size);
}

static void screenWrite(int sr, unsigned long offset, void *img, size_t img_size)
{
    unsigned long addr = offset;
    int nb, nd, n;
    char *buf = (char *)img;
    size_t len = img_size;
    
    if (sr == -1) return;
    
    if (len > 64) {
        n = addr & 0x03;
        if (n > 0) {
            n = 4 - n;
            _farsetsel(sr);
            switch (n) {
            case 3: _farnspokeb(addr++, *buf++);
            case 2: _farnspokeb(addr++, *buf++);
            case 1: _farnspokeb(addr++, *buf++);
            }
            len -= n;
        }
    }

    nd = len >> 2;
    nb = len & 0x03;
    
    if (nd > 0) {
        n = (nd + 15) >> 4;
        addr = addr - (((0 - nd) & 15) << 2);
        buf = buf - (((0 - nd) & 15) << 2);
        _farsetsel(sr);
        switch (nd & 15) {
        case 0: do { _farnspokel(addr + 0x00, ((unsigned long*)buf)[0x00]);
        case 15:     _farnspokel(addr + 0x04, ((unsigned long*)buf)[0x01]);
        case 14:     _farnspokel(addr + 0x08, ((unsigned long*)buf)[0x02]);
        case 13:     _farnspokel(addr + 0x0C, ((unsigned long*)buf)[0x03]);
        case 12:     _farnspokel(addr + 0x10, ((unsigned long*)buf)[0x04]);
        case 11:     _farnspokel(addr + 0x14, ((unsigned long*)buf)[0x05]);
        case 10:     _farnspokel(addr + 0x18, ((unsigned long*)buf)[0x06]);
        case 9:      _farnspokel(addr + 0x1C, ((unsigned long*)buf)[0x07]);
        case 8:      _farnspokel(addr + 0x20, ((unsigned long*)buf)[0x08]);
        case 7:      _farnspokel(addr + 0x24, ((unsigned long*)buf)[0x09]);
        case 6:      _farnspokel(addr + 0x28, ((unsigned long*)buf)[0x0A]);
        case 5:      _farnspokel(addr + 0x2C, ((unsigned long*)buf)[0x0B]);
        case 4:      _farnspokel(addr + 0x30, ((unsigned long*)buf)[0x0C]);
        case 3:      _farnspokel(addr + 0x34, ((unsigned long*)buf)[0x0D]);
        case 2:      _farnspokel(addr + 0x38, ((unsigned long*)buf)[0x0E]);
        case 1:      _farnspokel(addr + 0x3C, ((unsigned long*)buf)[0x0F]);
                     addr += 0x40;
                     buf += 0x40;
                } while (--n > 0);
        }
    }
    
    switch (nb) {
    case 3: _farsetsel(sr);
            _farnspokew(addr, ((unsigned short*)buf)[0]);
            _farnspokeb(addr + 2, buf[2]);
            return;
    case 2: _farpokew(sr, addr, ((unsigned short*)buf)[0]);
            return;
    case 1: _farpokeb(sr, addr, buf[0]);
    }
}

extern void __yields(void);
static void x86_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    unsigned long area_x, draw_size, screen_width_size, offset, height;
    
    if (area->x1 == 0 && area->x2 == disp->hor_res) {
        draw_size = disp->hor_res * (area->y2 - area->y1 + 1);
        offset = area->y1 * (disp->hor_res * PIXEL_SIZE);
        
        screenWrite(selector, offset, color_p, draw_size);
    } else {
        screen_width_size = disp->hor_res * PIXEL_SIZE;
        area_x = area->x2 - area->x1 + 1;
        draw_size = area_x * PIXEL_SIZE;
        offset = area->y1 * screen_width_size + area->x1 * PIXEL_SIZE;
        height = area->y2 - area->y1 + 1;
        
        for (; height; height--) {
            screenWrite(selector, offset, color_p, draw_size);
            offset += screen_width_size;
            color_p += area_x;
        }
    }
    __yields();
    lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

/* TOUCHPANEL */
// Default vaule
double DAX = -0.424629, DBX = -0.002177, DDX = 834.603571;
double DAY = -0.000115, DBY = -0.292238, DDY = 537.392779;

/* Reading input device (simulated mouse here) */
static int tmpX = 0, tmpY = 0, pre_dx = 0, pre_dy = 0, tp_pointX = 0, tp_pointY = 0;
static bool tp_press;
static int _dx = 0, _dy = 0; // mouse
static bool pointer_event(lv_indev_drv_t * indev, lv_indev_data_t * data)
{
    int dx, dy;
    int msdx = 0, msdy = 0;
    double newX, newY;
    bool isPress = false;

    if (_touch_used) {
        
        isPress = get_touchXY(&dx, &dy);
        
        if (isPress) {
            newX = dx * DAX + dy * DBX + DDX;
            newY = dx * DAY + dy * DBY + DDY;
            
            if (newX < 0) newX = 0;
            else if (newX > DRW) newX = DRW;
            
            if (newY < 0) newY = 0;
            else if (newY > DRH) newY = DRH;
            
            _dx = pre_dx = newX;
            _dy = pre_dy = newY;
            
            data->point.x = tp_pointX = newX;
            data->point.y = tp_pointY = newY;
        } else {
            data->point.x = tp_pointX = _dx = pre_dx;
            data->point.y = tp_pointY = _dy = pre_dy;
        }
        
        tp_press = isPress;
    }

    if (_kbms_used) {
        
        if (mouse_needs_poll()) poll_mouse();
        
        get_mouse_mickeys(&msdx, &msdy);
        
        _dx += msdx;
        _dy += msdy;
        
        if (_dx < 0) _dx = 0;
        else if (_dx > DRW) _dx = DRW;
        
        if (_dy < 0) _dy = 0;
        else if (_dy > DRH) _dy = DRH;
        
        data->point.x = pre_dx = _dx;
        data->point.y = pre_dy = _dy;
    }
    
    if (isPress || (_kbms_used && (mouse_b & 0x01) == 0x01))
        data->state = LV_INDEV_STATE_PR;
    else
        data->state = LV_INDEV_STATE_REL;
    
    return false;
}

bool keyboard_event(lv_indev_drv_t * indev, lv_indev_data_t * data)
{
    if (keyboard_needs_poll()) poll_keyboard();
    
    data->state = keypressed() ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

    if (keypressed()) {
        if (key[KEY_UP]) data->key = LV_KEY_UP;
        else if (key[KEY_DOWN]) data->key = LV_KEY_DOWN;
        else if (key[KEY_RIGHT]) data->key = LV_KEY_RIGHT;
        else if (key[KEY_LEFT]) data->key = LV_KEY_LEFT;
        else if (key[KEY_ESC]) data->key = LV_KEY_ESC;
        else if (key[KEY_DEL]) data->key = LV_KEY_DEL;
        else if (key[KEY_BACKSPACE]) data->key = LV_KEY_BACKSPACE;
        else if (key[KEY_ENTER]) data->key = LV_KEY_ENTER;
        else if (key[KEY_HOME]) data->key = LV_KEY_HOME;
        else if (key[KEY_END]) data->key = LV_KEY_END;
        else {
            if (key_shifts & KB_SHIFT_FLAG) { // 有按住 shift
                if (key[KEY_TAB]) data->key = LV_KEY_PREV;
                else data->key = readkey() & 0xFF;
            } else {
                if (key[KEY_TAB]) data->key = LV_KEY_NEXT;
                else data->key = readkey() & 0xFF;
            }
        }
    }

    return false;
}

static int allego_mousewheel_value = 0;
bool wheel_event(lv_indev_drv_t * indev, lv_indev_data_t * data)
{
    if (mouse_needs_poll()) poll_mouse();
    
    data->state = ((mouse_b & 0x01) == 0x01) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    Serial.println(mouse_w);
    data->enc_diff = mouse_z; // mouse_w // allego_mousewheel_value;
    allego_mousewheel_value = 0;
    
    return false;
}

lv_group_t* lv86GetGroup(void) {
    if (_group != NULL) return _group;
    
    return NULL;
}

extern char* bootloaderPath;
char* pathAdd(char* path, char* filename) {
    if (filename == NULL) {
        printf("ERROR: your filename is NULL\n");
        return NULL;
    }
    
    char* str = (char*)malloc(sizeof(char) * 128);
    
    if (path == NULL)
        sprintf(str, "%s", filename);
    else
        sprintf(str, "%s%s", path, filename);
    
    return str;
}

static void getTPCal(void) {
    int i = 0, j, k;
    char c;
    int data1 = 0, data2 = 0;
    char floatchr[128] = {'\0'};
    char* tpCalPath = NULL;
    
    tpCalPath = pathAdd(bootloaderPath, "TP_CAL3.txt");
    FILE* fp = fopen(tpCalPath, "r");
    if (fp != NULL) {
        for (i=0; i<128 && (c = fgetc(fp)) != ','; i++) floatchr[i] = c;
        floatchr[i] = '\0';
        DAX = atof(floatchr);
        for (int i=0; i<128 && (c = fgetc(fp)) != ','; i++) floatchr[i] = c;
        floatchr[i] = '\0';
        DBX = atof(floatchr);
        for (int i=0; i<128 && (c = fgetc(fp)) != '\n'; i++) floatchr[i] = c;
        floatchr[i] = '\0';
        DDX = atof(floatchr);
        for (i=0; i<128 && (c = fgetc(fp)) != ','; i++) floatchr[i] = c;
        floatchr[i] = '\0';
        DAY = atof(floatchr);
        for (int i=0; i<128 && (c = fgetc(fp)) != ','; i++) floatchr[i] = c;
        floatchr[i] = '\0';
        DBY = atof(floatchr);
        for (int i=0; i<128 && (c = fgetc(fp)) != '\n'; i++) floatchr[i] = c;
        floatchr[i] = '\0';
        DDY = atof(floatchr);
        
        fclose(fp);
        
        // 假如讀進來的校正的資料和預設值偏差太多，就直接開啟校正模式
    } else {
        // 沒有檔案，直接開啟校正模式
        // while (!touchCalBegin(pathAdd(bootloaderPath, "TP_CAL3.txt")));
        // DAX = -0.424835; DBX = -0.002177; DDX = 834.603571;
        // DAY = -0.000115; DBY = -0.292238; DDY = 537.392779;
    }
    
    if (tpCalPath != NULL) free(tpCalPath);
}


/* FUNCTIONS */
lv_font_t* lv86SetFont(const char* font_path, unsigned short size) {
    if (globalFont != NULL || font_path == NULL || size == 0) return NULL;
    
    globalFont = (lv_font_t*) malloc(sizeof(lv_font_t));
    if (globalFont != NULL) {
        if (lv_freetype_font_init(globalFont, font_path, size) == FT_Err_Ok) {
            return globalFont;
        } else
            return NULL;
    }
    
    return NULL;
}

lv_style_t* lv86FontSet(lv_obj_t* obj, lv_font_t* font) {
    if (!initiallv86 || obj == NULL || font == NULL) return NULL;
    
    lv_style_t* style = (lv_style_t*) malloc(sizeof(lv_style_t));
    lv_style_init(style);
    lv_style_set_text_font(style, LV_STATE_DEFAULT, font);
    lv_obj_add_style(obj, 0, style);
    return style;
}

lv_obj_t* lv86LoadImg(const char* img_path, lv_obj_t* obj) {
    lv_obj_t* icon = NULL;
    if (!initiallv86 || img_path == NULL) return NULL;
    
    if (obj == NULL)
        icon = lv_img_create(lv_scr_act(), NULL);
    else
        icon = lv_img_create(obj, NULL);

    if (icon != NULL)
        lv_img_set_src(icon, img_path);
    else printf("lv86LoadImg() return NULL.\n");
    
    return icon;
}

void lv86ChangeImg(lv_obj_t* obj, const char* img_path) {
    if (!initiallv86 || img_path == NULL) return;
    if (obj != NULL) lv_img_set_src(obj, img_path);
    else printf("lv86ChangeImg() get NULL.\n");
}

unsigned int get_x86_hor_resolution(void) {
    return DRW;
}

unsigned int get_x86_ver_resolution(void) {
    return DRH;
}

bool get_x86_touchXY(int* x, int* y) {
    return get_touchXY(x, y);
}

void get_x86_tp_point(int* x, int* y, bool* press) {
    *x = tp_pointX;
    *y = tp_pointY;
    *press = tp_press;
}

// QR Code
lv_obj_t* lv86MakeQRCode(lv_obj_t * parent, lv_coord_t size, const void * data, unsigned long data_len) {
    lv_obj_t* qrobj = lv_qrcode_create(parent, size, LV_COLOR_BLACK, LV_COLOR_WHITE);
    lv_qrcode_update(qrobj, data, data_len);
    lv_obj_align(qrobj, NULL, LV_ALIGN_CENTER, 0, 0);
    return qrobj;
}

void lv86UpdateQRCode(lv_obj_t* qrobj, const void * data, unsigned long data_len) {
    if (qrobj != NULL) lv_qrcode_update(qrobj, data, data_len);
}

void lv86ClearQRCode(lv_obj_t* qrobj) {
    if (qrobj != NULL) lv_qrcode_delete(qrobj);
}

// Backlight
void lv86SetBacklight(unsigned char percent) {
    int ret;
    if (percent > 100) percent = 100;
    if (_vx86_disp == -1) return;
    
    if (_vx86_disp == DMP_VGA) {
        ret = map(percent, 0, 100, 0, 255);
        set_pwm_level(ret);
    } else if (_vx86_disp == DMP_LCD) {
        ret = map(percent, 0, 100, 0x50, 0x5F); // no effect
        lcd_backlight(ret);
    }
}


// Console class
Console::Console() {ta1 = NULL; begined = false; consoleFont = NULL; font_color = LV_COLOR_WHITE;}

bool Console::begin(unsigned long color, bool graduation, lv_obj_t* parent) {
    if (!initiallv86) return false;
    
    if (!parent)
        ta1 = lv_textarea_create(lv_scr_act(), NULL);
    else
        ta1 = lv_textarea_create(parent, NULL);
    lv_textarea_set_scrollbar_mode(ta1, LV_SCRLBAR_MODE_OFF);
    taMaxLength = 2000;
    taIndex = 0;
    
    ta_style = (lv_style_t*) malloc(sizeof(lv_style_t));
    lv_style_init(ta_style);
    lv_style_set_radius(ta_style, LV_STATE_DEFAULT, 10);
    lv_style_set_border_width(ta_style, LV_STATE_DEFAULT, 5);
    lv_style_set_text_color(ta_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_border_color(ta_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_bg_opa(ta_style, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_bg_color(ta_style, LV_STATE_DEFAULT, LV_COLOR_MAKE(((color & 0x00FF0000L) >> 16), ((color & 0x0000FF00L) >> 8), (color & 0x000000FFL)));
    if (graduation) {
        lv_style_set_bg_grad_color(ta_style, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x00, 0x00, 0x38));
        lv_style_set_bg_grad_dir(ta_style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
        lv_style_set_bg_main_stop(ta_style, LV_STATE_DEFAULT, 10);
        lv_style_set_bg_grad_stop(ta_style, LV_STATE_DEFAULT, 250);
    }
    lv_obj_add_style(ta1, LV_LABEL_PART_MAIN, ta_style);
    lv_obj_set_size(ta1, DRW, DRH);
    lv_obj_align(ta1, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
    // lv_textarea_set_text_sel(ta1, true);
    lv_textarea_set_text(ta1, ""); // clear all text
    if (globalFont != NULL) consoleFont = globalFont;
    if (consoleFont != NULL) lv86FontSet(ta1, consoleFont); // default Font
    begined = true;
    return true;
}

bool Console::beginEX(unsigned long color, unsigned int w, unsigned int h, unsigned int x, unsigned int y, bool graduation, lv_obj_t* parent) {
    bool ret = begin(color, graduation, parent);
    if (ret) {
        if (x > DRW) x = DRW;
        if (y > DRH) y = DRH;
        if ((x + w) > DRW) w = DRW - x;
        if ((y + h) > DRH) h = DRH - y;
        
        lv_obj_set_size(ta1, w, h);
        lv_obj_align(ta1, NULL, LV_ALIGN_IN_TOP_LEFT, x, y);
    }
    
    return ret;
}

bool Console::begin() {
    return begin(0x7878D0, true, NULL);
}

void Console::end(void) {
    if (begined == false) return;
    memset(myWord, 0, sizeof(myWord));
    lv_textarea_set_text(ta1, ""); // clear all text
    free(ta_style);
    lv_obj_del(ta1);
    ta1 = NULL;
}

void Console::scrollbarON() {
    if (begined == false) return;
    lv_textarea_set_scrollbar_mode(ta1, LV_SCRLBAR_MODE_AUTO);
}

void Console::scrollbarOFF() {
    if (begined == false) return;
    lv_textarea_set_scrollbar_mode(ta1, LV_SCRLBAR_MODE_OFF);
}

void Console::setBufSize(unsigned int s) {
    if (begined == false) return;
    // lv_textarea_set_max_length(ta1, s*2); // uncomment this because it will let you get a very low update rate on screen
    if (s > 2000) taMaxLength = 2000; else taMaxLength = s;
}

bool Console::setFont(const char* font_path, unsigned short size) {
    if (begined == false) return false;
    
    consoleFont = (lv_font_t*) malloc(sizeof(lv_font_t)); // new Font
    if (consoleFont != NULL) {
        if (lv_freetype_font_init(consoleFont, font_path, size) == FT_Err_Ok) {
            font_style = lv86FontSet(ta1, consoleFont);
            lv_style_set_text_color(font_style, LV_STATE_DEFAULT, font_color);
            return true;
        } else {
            free(consoleFont);
            consoleFont = NULL;
            return false;
        }
    }
    
    return false;
}

void Console::setFontColor(unsigned long color) {
    font_color = LV_COLOR_MAKE(((color & 0x00FF0000L) >> 16), ((color & 0x0000FF00L) >> 8), (color & 0x000000FFL));
    if (font_style != NULL) {
        lv_style_set_text_color(font_style, LV_STATE_DEFAULT, font_color);
        lv_obj_add_style(ta1, 0, font_style);
    }
}

size_t Console::write(unsigned char c) {
    int i;
    if (begined == false) return 0;
    if ((taIndex != 0) && (taMaxLength != 0) && (taIndex == taMaxLength)) {
        taIndex -= 1;
        for (i=0; i<taIndex; i++)
            myWord[i] = myWord[i + 1];
        for (; i<taMaxLength; i++)
            myWord[i] = '\0';
    }
    myWord[taIndex++] = c;
    lv_textarea_set_text(ta1, (const char*) myWord);
    // lv_textarea_add_char(ta1, c);
    // lv_task_handler();
    return 1;
}

// If all text length > taMaxLength, it will be match as taMaxLength
size_t Console::write(const unsigned char *str, size_t str_size) {
    int i;
    unsigned long empty;
    unsigned long back;
    if (begined == false || str == NULL) return 0;
    // lv_textarea_add_text(ta1, (const char*) str);

    empty = taMaxLength - taIndex;
    back = (empty > str_size) ? 0 : (str_size - empty);
    if (str_size != 0) {
        if (back > taIndex) {
            for (; taIndex > 0; taIndex--)
                myWord[taIndex] = '\0';
            taIndex = 0;
        } else if (back > 0) {
            taIndex -= back;
            for (i=0; i<taIndex; i++)
                myWord[i] = myWord[i + back];
            for (; i<taMaxLength; i++)
                myWord[i] = '\0';
        }
        
        for (i=0; i<str_size; i++)
            myWord[taIndex++] = str[i];
        
        lv_textarea_set_text(ta1, (const char*) myWord);
    }
    // printf("taIndex=%ld taMaxLength=%ld\n", taIndex, taMaxLength);
    lv_task_handler();
    return str_size;
}

void Console::fflush(void) {
    lv_task_handler();
}

int Console::available(void) {

}

int Console::peek(void) {

}

int Console::read(void) {

}

Console console;
