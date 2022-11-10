#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include <map>
#include "setup.hpp"
#include "parrot.h"
#include "wallpaper.h"
#include "palette.h"
#include <LGFX_TFT_eSPI.hpp>

static LGFX tft;
static LGFX_Sprite mainScene(&tft);
static LGFX_Sprite bg;
static LGFX_Sprite sprite[10];

static std::uint32_t count = 0;
static std::uint32_t rotate = 0;

const static int mainWidth = 320;
static int width;
static int widthMiddle;

const static int mainHeight = 240;
static int height;
static int heightMiddle;

const static int zoom = 1;

static uint32_t sec, psec;
static size_t fps = 0, frame_count = 0;
static bool collision = false;
int32_t x = 20;
int32_t y = 20;
int32_t x_1 = 4;
int32_t y_1 = 8;

int font = 4;

struct button1{
    int x = 10;
    int y = 30;
    int x2 = 100;
    int y2 = 90;
    char label[9] = "Button 1";
    bool pressed = false;
} button1;

void fullScreenBackground(){
    int n = 10;
    while (n < height){
        for (int i = 0; i <= 10; i++) {
            tft.drawFastHLine(0, n + i, width, ZX_YELLOW1);
        }
        n += 30;
    }
}

void screenSetup(void *pvParameters){
    tft.init();
    tft.setRotation(3);
    tft.setColorDepth(16);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_RED);

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
    tft.printf("FPS: %d", fps);
}

void move(){
    if (collision){
        x -= 10;
        y -= 10;
    }
    else {
        x += x_1;
        if (x <= 0) {
            x = 0;
            if (x_1 <= 0) x_1 = -x_1;
        }
        else if (x >= mainWidth) {
            x = mainWidth - 1;
            if (x_1 > 0) x_1 = -x_1;
        }

        y += y_1;
        if (y <= 0) {
            y = 0;
            if (y_1 <= 0) y_1 = -y_1;
        }
        else if (y >= mainHeight) {
            y = mainHeight - 1;
            if (y_1 > 0) y_1 = -y_1;
        }
    }
}

void createParrotSprite(){
    sprite[0].createFromBmp(parrot00);
    sprite[1].createFromBmp(parrot01);
    sprite[2].createFromBmp(parrot02);
    sprite[3].createFromBmp(parrot03);
    sprite[4].createFromBmp(parrot04);
    sprite[5].createFromBmp(parrot05);
    sprite[6].createFromBmp(parrot06);
    sprite[7].createFromBmp(parrot07);
    sprite[8].createFromBmp(parrot08);
    sprite[9].createFromBmp(parrot09);
}

void checkButtonPress(){
    int x;
    int y;
    tft.getTouch(&x, &y);
    if (x >= button1.x && x <= button1.x2 && y >= button1.y &&
         y <= button1.y2){
        if (++font >= 8){
            font = 1;
        }
//        tft.fillRect(button1.x, button1.y, button1.x2-button1.x, button1.y2-button1.y, ZX_YELLOW0);
    }
}

void mainLoop(void *pvParameters);

void setup(){
    try{
        xTaskCreatePinnedToCore(screenSetup, "screenSetup", 1024, NULL, 2, NULL, ARDUINO_RUNNING_CORE);

//        screenSetup();

        tft.fillScreen(ZX_BLUE0);
        tft.setTextFont(font);
        tft.setPivot(widthMiddle, heightMiddle);
        fullScreenBackground();

        mainScene.createSprite(mainWidth, mainHeight);
        mainScene.setSwapBytes(true);
//        mainScene.pushImage(0, 0, mainWidth, mainHeight, wallpaper);

        createParrotSprite();

//        xTaskCreatePinnedToCore(mainLoop, "mainLoop", 4096, NULL, 2, NULL, ARDUINO_RUNNING_CORE);

    }
    catch (std::exception const &e){
        tft.print(e.what());
    }
}

void loop(){
//    while(1) {
//        try {
//            // Draw background
////        mainScene.pushImage(0, 0, mainWidth, mainHeight, wallpaper);
//            mainScene.clear();
//
//            // Update coords
//            move();
//
//            if (++count == 10) count = 0;
//            if (++rotate == 360) rotate = 0;
//            sprite[count].pushRotateZoom(&mainScene, x, y, rotate, 1, 1);
//
//            // Draw in middle of screen and show FPS
//            mainScene.pushSprite(&tft, widthMiddle - (mainWidth >> 1), heightMiddle - (mainHeight >> 1));
//            frameCount();
//        }
//        catch (std::exception const &e) {
//            tft.print(e.what());
//        }
//    }
}
