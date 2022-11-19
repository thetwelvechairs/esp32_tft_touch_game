#define LGFX_USE_V1

#include "freertos/FreeRTOS.h"
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include <map>
#include <vector>
#include "setup.hpp"
#include "certs.h"
#include "palette.h"
//#include <IRremote.h>
#include <WiFi.h>
#include <aws_iot.h>
#include <iot.h>
#include <LGFX_TFT_eSPI.hpp>

#include "apple.h"
#include "blank.h"
#include "chromecast.h"
#include "ps4.h"
#include "nintendo_switch.h"
#include "speaker.h"

static LGFX tft;
static LGFX_Sprite mainScene(&tft);

const static int mainWidth = 320;
static int width;
static int widthMiddle;

const static int mainHeight = 240;
static int height;
static int heightMiddle;

static uint32_t sec, psec;
static size_t fps = 0, frame_count = 0;

static struct ball {
    bool collision = false;
    int32_t x = 20;
    int32_t y = 20;
    int32_t x_1 = 4;
    int32_t y_1 = 8;
}ball;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[480 * 10];

int font = 1;

const std::vector<lv_img_dsc_t> images = {apple, chromecast, ps4, nintendo_switch, blank, blank, speaker};

void screenSetup(){
    tft.init();
    tft.setRotation(2);
    tft.setColorDepth(16);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    width = tft.width();
    height = tft.height();
    widthMiddle = width >> 1;
    heightMiddle = height >> 1;
}

void frameCount(){
    ++frame_count;
    sec = lgfx::millis() / 1000;
    if (psec != sec) {
        psec = sec;
        fps = frame_count;
        frame_count = 0;
    }
    tft.setCursor(0, 0);
    tft.setTextFont(font);
    tft.printf("FPS: %d SSID: %s IP: %s", fps, wifi.ssidName.c_str(), wifi.ip.c_str());
}

void move(){
    if (ball.collision) {
        ball.x -= 10;
        ball.y -= 10;
    }
    else {
        ball.x += ball.x_1;
        if (ball.x <= 0) {
            ball.x = 0;
            if (ball.x_1 <= 0) ball.x_1 = -ball.x_1;
        }
        else if (ball.x >= mainWidth) {
            ball.x = mainWidth - 1;
            if (ball.x_1 > 0) ball.x_1 = -ball.x_1;
        }

        ball.y += ball.y_1;
        if (ball.y <= 0) {
            ball.y = 0;
            if (ball.y_1 <= 0) ball.y_1 = -ball.y_1;
        }
        else if (ball.y >= mainHeight) {
            ball.y = mainHeight - 1;
            if (ball.y_1 > 0) ball.y_1 = -ball.y_1;
        }
    }
}

void my_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p){
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

void declareImages(){
    for (const auto& image : images){
        LV_IMG_DECLARE(image)
    }
}

void publishEvent(){
    if (wifi.status) {
        StaticJsonDocument<512> json;
        JsonObject stateObj = json.createNestedObject("state");
        JsonObject reportedObj = stateObj.createNestedObject("reported");
        reportedObj["state"] = "It's alive!!!";

        iot.aws.publishMessage(json, iot.topic);
    }
}

static void btn_event_cb(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSED) {
        lv_obj_set_style_outline_color(btn, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_outline_width(btn, 1, LV_STATE_DEFAULT);
    }
    else if (code == LV_EVENT_RELEASED){
        publishEvent();
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_DEFAULT);
    }
}

void toggleWifi(lv_event_t* e){
    if (e->code == LV_EVENT_RELEASED) {
        if (!wifi.status) {
            WiFi.begin(wifi.ssid, wifi.password);
            delay(2000);
            wifi.status = true;
            wifi.ssidName = WiFi.SSID().c_str();
            wifi.ip = WiFi.localIP().toString().c_str();
        } else {
            WiFi.disconnect();
            delay(1000);
            wifi.status = false;
            wifi.ssidName = "N/A";
            wifi.ip = "0.0.0.0";
        }
    }
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t* indev_driver, lv_indev_data_t* data){
    uint16_t touchX;
    uint16_t touchY;
    bool touched = tft.getTouch( &touchX, &touchY);
    if(!touched){
        data->state = LV_INDEV_STATE_REL;
    }
    else{
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = static_cast<lv_coord_t>(touchX);
        data->point.y = static_cast<lv_coord_t>(touchY);
    }
}

void mainScreen(){
    lv_obj_t* cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont, 300, 226);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* cont2 = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont2, 300, 226);
    lv_obj_align(cont2, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_layout(cont2, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont2, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(cont2, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* btn;

    for(int i = 0; i < 4; i++) {
        btn = lv_img_create(cont);
        lv_img_set_src(btn, &images[i]);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, nullptr);
    }

    for(int i = 0; i < 3; i++) {
        btn = lv_img_create(cont2);
        lv_img_set_src(btn, &images[4+i]);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, nullptr);
        if (i == 0){
            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_recolor(label, true);
            lv_label_set_text(label, "\n #000000 -");
            lv_obj_align(label, LV_ALIGN_CENTER, 0, -4);
            lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, nullptr);
        }
        else if (i == 1){
            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_recolor(label, true);
            lv_label_set_text(label, "\n #000000 +");
            lv_obj_align(label, LV_ALIGN_CENTER, 0, -4);
            lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, nullptr);
        }
        else {
            lv_obj_add_event_cb(btn, toggleWifi, LV_EVENT_ALL, nullptr);
        }
    }
}

void setup(){
    try{
        WiFiClass::mode(WIFI_STA);
        declareImages();
        screenSetup();

        tft.setTextFont(font);
        tft.setPivot(static_cast<float>(widthMiddle), static_cast<float>(heightMiddle));

        lv_init();
        lv_disp_draw_buf_init(&draw_buf, buf, nullptr, width * 10);

        /*Initialize the display*/
        static lv_disp_drv_t disp_drv;
        lv_disp_drv_init(&disp_drv);

        /*Change the following line to your display resolution*/
        disp_drv.hor_res = static_cast<lv_coord_t>(width);
        disp_drv.ver_res = static_cast<lv_coord_t>(height);
        disp_drv.flush_cb = my_disp_flush;
        disp_drv.draw_buf = &draw_buf;
        lv_disp_drv_register(&disp_drv);

        /*Initialize the (dummy) input device driver*/
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = my_touchpad_read;
        lv_indev_drv_register(&indev_drv);

        if (wifi.status) {
            iot.aws.connect();
            publishEvent();
        }

        mainScreen();

    }
    catch (std::exception const &e){
        tft.print(e.what());
    }
}

void loop(){
    for (;;){
        try {
            lv_timer_handler(); /* let the GUI do its work */
            frameCount();
        }
        catch (std::exception const &e) {
            tft.print(e.what());
        }
    }
}