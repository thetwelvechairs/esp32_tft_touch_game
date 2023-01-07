#define LGFX_USE_V1

#include "freertos/FreeRTOS.h"
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include <map>
#include <vector>
#include "setup.hpp"
#include "certs.h"
#include "palette.h"
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <WiFi.h>
#include <EEPROM.h>
//#include <aws_iot.h>
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

static lv_obj_t* tabView;
static lv_obj_t* kb;

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

/*A static or global variable to store the buffers*/
static lv_disp_draw_buf_t disp_buf;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf1[480 * 10];
static lv_color_t buf2[480 * 10];

static lv_disp_drv_t disp_drv;

int font = 1;

static lv_obj_t* list;

static std::string ssid;

static int debug;

const std::vector<lv_img_dsc_t> images = {apple, chromecast, ps4, nintendo_switch, blank, blank, speaker};

IRrecv irrecv(17);
IRsend irsend(18);
decode_results results;

#define AUTOSOUND   0xF66ACC3
#define VOLUP       0x240C
#define VOLDOWN     0x640C
#define MUTE        0x140C
#define INPUTTV     0xA50
#define INPUTUP     0x2F0
#define INPUTDOWN   0xAF0
#define INPUTSEL    0xA70

void message_box_cb(lv_event_t* e){
    lv_obj_t * obj = lv_event_get_current_target(e);
    LV_UNUSED(obj);
    LV_LOG_USER("Button %s clicked", lv_msgbox_get_active_btn_text(obj));
}

void screenSetup(){
    tft.init();
    tft.setRotation(3);
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
    tft.printf("FPS: %d SSID: %s IP: %s IR: %s", fps, wifi.ssidName.c_str(), wifi.ip.c_str(), resultToHumanReadableBasic(&results).c_str());
    tft.setCursor(320,0);
    tft.printf("DEBUG: %d", debug);
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

void my_disp_flush(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p){
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
    tft.endWrite();

    lv_disp_flush_ready(disp_drv);
}

void declareImages(){
    for (const auto& image : images){
        LV_IMG_DECLARE(image)
    }
}

void publishEvent(){
    if (wifi.status) {
//        StaticJsonDocument<512> json;
//        JsonObject stateObj = json.createNestedObject("state");
//        JsonObject reportedObj = stateObj.createNestedObject("reported");
//        reportedObj["state"] = "It's alive!!!";
//
//        iot.aws.publishMessage(json, iot.topic);
    }
}

static void btn_event_cb_default(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSED) {
        lv_obj_set_style_outline_color(btn, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_outline_width(btn, 1, LV_STATE_DEFAULT);
    }
    else if (code == LV_EVENT_RELEASED){
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_DEFAULT);
    }
}

static void btn_event_cb_voldown(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSED) {
        lv_obj_set_style_outline_color(btn, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_outline_width(btn, 1, LV_STATE_DEFAULT);
    }
    else if (code == LV_EVENT_RELEASED){
        publishEvent();
        irsend.sendSony(VOLDOWN, 15, 3);

        lv_obj_set_style_outline_width(btn, 0, LV_STATE_DEFAULT);
    }
}

static void btn_event_cb_volup(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSED) {
        lv_obj_set_style_outline_color(btn, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_outline_width(btn, 1, LV_STATE_DEFAULT);
    }
    else if (code == LV_EVENT_RELEASED){
        publishEvent();
        irsend.sendSony(VOLUP, 15, 3);

        lv_obj_set_style_outline_width(btn, 0, LV_STATE_DEFAULT);
    }
}

static void btn_event_cb_volauto(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSED) {
        lv_obj_set_style_outline_color(btn, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_outline_width(btn, 1, LV_STATE_DEFAULT);
    }
    else if (code == LV_EVENT_RELEASED){
        publishEvent();
        irsend.sendSony(AUTOSOUND, 10, 3);

        lv_obj_set_style_outline_width(btn, 0, LV_STATE_DEFAULT);
    }
}

static void btn_event_cb_volmute(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSED) {
        lv_obj_set_style_outline_color(btn, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_outline_width(btn, 1, LV_STATE_DEFAULT);
    }
    else if (code == LV_EVENT_RELEASED){
        publishEvent();
        irsend.sendSony(MUTE, 15, 3);

        lv_obj_set_style_outline_width(btn, 0, LV_STATE_DEFAULT);
    }
}

void select_wifi_cb(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* text_area = lv_event_get_target(e);
    auto keyboard = lv_event_get_user_data(e);
}

void event_handler(lv_event_t* e){

}

void toggleWifi(lv_event_t* e){
//    if (e->code == LV_EVENT_RELEASED) {
//        if (!wifi.status) {
//            WiFi.begin(wifi.ssid, wifi.password);
//            while (WiFiClass::status() != WL_CONNECTED) {
//                Serial.print('.');
//                delay(500);
//            }
//            wifi.status = true;
//            wifi.ssidName = WiFi.SSID().c_str();
//            wifi.ip = WiFi.localIP().toString().c_str();
//        } else {
//            WiFi.disconnect();
//            while (WiFiClass::status() == WL_CONNECTED) {
//                delay(500);
//            }
//            wifi.status = false;
//            wifi.ssidName = "N/A";
//            wifi.ip = "0.0.0.0";
//        }
//    }
}

void scan_wifi_async(){

        int n = WiFi.scanNetworks();
        delay(15000);
        static std::vector<std::basic_string<char>> ssids{};
        lv_obj_set_size(list, lv_pct(100), lv_pct(100));
        if (n > 0) {
            for (int i = 0; i < n; ++i) {
                ssids.emplace_back(WiFi.SSID(i).c_str());
//            rssi = WiFi.RSSI(i);
//            sprintf(str, "%d", rssi);
//            ssids.emplace_back(str);
//            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            }
        }

        lv_obj_t *list_btn;

        for (int i = 0; i < ssids.size(); i++) {
            list_btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, ssids.at(i).c_str());
            lv_obj_set_width(list_btn, lv_pct(100));
            const char *temp = ssids.at(i).c_str();
            lv_obj_add_event_cb(list_btn, select_wifi_cb, LV_EVENT_CLICKED, &temp);
            lv_group_remove_obj(list_btn);
        }

        lv_obj_center(list);

}

void scanWifi(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
//        auto t = lv_async_call(scan_wifi_async, NULL);
    }
}

void show_kb(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_FOCUSED) {
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }

    if(code == LV_EVENT_DEFOCUSED) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

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
    static lv_style_t panelStyle;
    lv_style_init(&panelStyle);
    lv_style_reset(&panelStyle);
    lv_style_set_pad_all(&panelStyle, 0);

    static lv_style_t buttonStyle;
    lv_style_init(&buttonStyle);
    lv_style_reset(&buttonStyle);
    lv_style_set_pad_all(&buttonStyle, 8);

    tabView = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 45);
    static lv_obj_t* tabMain = lv_tabview_add_tab(tabView, "Display");
    static lv_obj_t* tab2 = lv_tabview_add_tab(tabView, "Sound");
    static lv_obj_t* tab3 = lv_tabview_add_tab(tabView, "Preferences");

    lv_obj_set_flex_flow(tabMain, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_flow(tab2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_flow(tab3, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* panel = lv_obj_create(tabMain);
    lv_obj_set_width(panel, width - 32);
    lv_obj_set_height(panel, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(panel, &panelStyle, 0);

    lv_obj_t* panel2 = lv_obj_create(tab2);
    lv_obj_set_width(panel2, width - 32);
    lv_obj_set_height(panel2, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel2, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_align(panel2, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(panel2, &panelStyle, 0);

    static lv_obj_t* btn;

    for(int i = 0; i < 4; i++) {
        btn = lv_img_create(panel);
        lv_img_set_src(btn, &images[i]);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_style(btn, &buttonStyle, 0);
        lv_obj_add_event_cb(btn, btn_event_cb_default, LV_EVENT_ALL, nullptr);
    }

    for(int i = 0; i < 4; i++) {
        btn = lv_img_create(panel2);
        lv_img_set_src(btn, &images[4]);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_style(btn, &buttonStyle, 0);
        lv_obj_set_style_text_color(btn, lv_color_black(), 0);
        if (i == 0){
            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_recolor(label, true);
            lv_label_set_text(label, LV_SYMBOL_MINUS);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_event_cb(btn, btn_event_cb_voldown, LV_EVENT_ALL, nullptr);
        }
        else if (i == 1){
            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_recolor(label, true);
            lv_label_set_text(label, LV_SYMBOL_PLUS);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_event_cb(btn, btn_event_cb_volup, LV_EVENT_ALL, nullptr);
        }
        else if (i == 2){
            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_recolor(label, true);
            lv_label_set_text(label, LV_SYMBOL_REFRESH);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_event_cb(btn, btn_event_cb_volauto, LV_EVENT_ALL, nullptr);
        }
        else {
            lv_obj_t* label = lv_label_create(btn);
            lv_label_set_recolor(label, true);
            lv_label_set_text(label, LV_SYMBOL_MUTE);
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
            lv_obj_add_event_cb(btn, btn_event_cb_volmute, LV_EVENT_ALL, nullptr);
        }
    }

    // PREFERENCES
    lv_obj_set_flex_flow(tab3, LV_FLEX_FLOW_ROW);
//    lv_obj_set_flex_align(tab3, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* panel3 = lv_obj_create(tab3);
    lv_obj_set_width(panel3, 216);
    lv_obj_set_height(panel3, 242);
    lv_obj_set_flex_flow(panel3, LV_FLEX_FLOW_COLUMN);
//    lv_obj_set_flex_align(panel3, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * sw;
    lv_obj_t* wifi_label = lv_label_create(panel3);
    lv_label_set_text(wifi_label, "Wi-Fi");
    sw = lv_switch_create(panel3);
    lv_obj_add_event_cb(sw, event_handler, LV_EVENT_ALL, NULL);

    lv_obj_t* oneline_label = lv_label_create(panel3);
    lv_label_set_text(oneline_label, "SSID");
    lv_obj_t* text_area = lv_textarea_create(panel3);
    lv_textarea_set_one_line(text_area, true);
    lv_textarea_add_text(text_area, ssid.c_str());
    lv_textarea_set_password_mode(text_area, false);
    lv_obj_set_width(text_area, lv_pct(100));
    lv_obj_add_event_cb(text_area, scanWifi, LV_EVENT_ALL, NULL);

    lv_obj_t* pwd_label = lv_label_create(panel3);
    lv_label_set_text(pwd_label, "Password");
    static lv_obj_t* pwd_ta = lv_textarea_create(panel3);
    lv_textarea_set_one_line(pwd_ta, true);
    lv_textarea_set_password_mode(pwd_ta, true);
    lv_obj_set_width(pwd_ta, lv_pct(100));
    lv_obj_add_event_cb(pwd_ta, show_kb, LV_EVENT_ALL, NULL);

    lv_obj_t* panel4 = lv_obj_create(tab3);
    lv_obj_set_width(panel4, 216);
    lv_obj_set_height(panel4, 242);

    list = lv_list_create(panel4);
    lv_obj_set_style_pad_all(panel4, 0, NULL);

    kb = lv_keyboard_create(lv_scr_act());
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

}

void setup(){
    try{
        pinMode(17, INPUT);     // IR in
        pinMode(18, OUTPUT);    // IR out
//        pinMode(38, OUTPUT);    // ???
//        pinMode(39, OUTPUT);    // Buzzer

        irrecv.enableIRIn();
        irsend.begin();

        WiFiClass::mode(WIFI_STA);
        WiFiClass::setHostname("ESP32_S3_TOUCH.local");

        declareImages();
        screenSetup();

        tft.setTextFont(font);
        tft.setPivot(static_cast<float>(widthMiddle), static_cast<float>(heightMiddle));

        lv_init();

        /*Initialize the display*/
        lv_disp_drv_init(&disp_drv);

        /*Change the following line to your display resolution*/
        disp_drv.hor_res = static_cast<lv_coord_t>(width);
        disp_drv.ver_res = static_cast<lv_coord_t>(height);
        disp_drv.flush_cb = my_disp_flush;
        disp_drv.draw_buf = &disp_buf;
        disp_drv.full_refresh = 1;
        lv_disp_drv_register(&disp_drv);

        /*Initialize `disp_buf` with the buffer(s). With only one buffer use NULL instead buf_2 */
        lv_disp_draw_buf_init(&disp_buf, buf1, buf2, width * 10);

        /*Initialize the (dummy) input device driver*/
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = my_touchpad_read;
        lv_indev_drv_register(&indev_drv);


        if (wifi.status) {
//            iot.aws.connect();
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
            if (irrecv.decode(&results)) {
                irrecv.resume();  // Receive the next value
            }

            lv_timer_handler(); /* let the GUI do its work */
            frameCount();
        }
        catch (std::exception const &e) {
            tft.print(e.what());
        }
    }
}