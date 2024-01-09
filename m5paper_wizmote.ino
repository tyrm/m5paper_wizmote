#include <esp_now.h>
#include "WiFi.h"
#include <M5EPD.h>
#include "Free_Fonts.h"

#define WIZMOTE_BUTTON_ON          1
#define WIZMOTE_BUTTON_OFF         2
#define WIZMOTE_BUTTON_NIGHT       3
#define WIZMOTE_BUTTON_ONE         16
#define WIZMOTE_BUTTON_TWO         17
#define WIZMOTE_BUTTON_THREE       18
#define WIZMOTE_BUTTON_FOUR        19
#define WIZMOTE_BUTTON_BRIGHT_UP   9
#define WIZMOTE_BUTTON_BRIGHT_DOWN 8

#define WIZMOTE_PROGRAM_ON      0x91
#define WIZMOTE_PROGRAM_DEFAULT 0x81

// Hardware
M5EPD_Canvas canvas(&M5.EPD);

// Remote
typedef struct message_structure {
  uint8_t program;      // 0x91 for ON button, 0x81 for all others
  uint8_t seq[4];       // Incremental sequence number 32 bit unsigned integer LSB first
  uint8_t byte5 = 32;   // Unknown
  uint8_t button;       // Identifies which button is being pressed
  uint8_t byte8 = 1;    // Unknown, but always 0x01
  uint8_t byte9 = 100;  // Unknown, but always 0x64

  uint8_t byte10;  // Unknown, maybe checksum
  uint8_t byte11;  // Unknown, maybe checksum
  uint8_t byte12;  // Unknown, maybe checksum
  uint8_t byte13;  // Unknown, maybe checksum
} message_structure;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

class Remote {
private:
  esp_now_peer_info_t peer;
  uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  message_structure remotePacket;
  uint32_t sequence = 0;

  void setSequence() {
    // Split the 32-bit integer into four 8-bit parts
    remotePacket.seq[0] = (uint8_t)(sequence & 0xFF);         // Least Significant Byte (LSB)
    remotePacket.seq[1] = (uint8_t)((sequence >> 8) & 0xFF);
    remotePacket.seq[2] = (uint8_t)((sequence >> 16) & 0xFF);
    remotePacket.seq[3] = (uint8_t)((sequence >> 24) & 0xFF); // Most Significant Byte (MSB)
  }
  
  void send(uint8_t program, uint8_t button) {
    remotePacket.program = program;
    remotePacket.button = button;

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &remotePacket, sizeof(remotePacket));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }

    sequence++;
    setSequence();
  }
public:
  void setup() {
    Serial.println("Seting up Remote.");

    WiFi.mode(WIFI_STA);

    if (esp_now_init() == ESP_OK) {
      Serial.println("ESPNow Init Success");
    } else {
      Serial.println("ESPNow Init Failed");
      // Retry InitESPNow, add a counte and then restart?
      // InitESPNow();
      // or Simply Restart
      ESP.restart();
    }

    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Trasnmitted packet
    esp_now_register_send_cb(OnDataSent);

    // Register broadcast peer
    memcpy(peer.peer_addr, broadcastAddress, 6);
    peer.channel = 1;  
    peer.encrypt = false;

    Serial.print("Processing: ");
    for (int ii = 0; ii < 6; ++ii ) {
      Serial.print((uint8_t) peer.peer_addr[ii], HEX);
      if (ii != 5) Serial.print(":");
    }
    Serial.print(" Status: ");
    // check if the peer exists
    bool exists = esp_now_is_peer_exist(peer.peer_addr);
    if (exists) {
      // Slave already paired.
      Serial.println("Already Paired");
    } else {
      // Slave not paired, attempt pair
      esp_err_t addStatus = esp_now_add_peer(&peer);
      if (addStatus == ESP_OK) {
        // Pair success
        Serial.println("Pair success");
      } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
        // How did we get so far!!
        Serial.println("ESPNOW Not Init");
      } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
        Serial.println("Add Peer - Invalid Argument");
      } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
        Serial.println("Peer list full");
      } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
        Serial.println("Out of memory");
      } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
        Serial.println("Peer Exists");
      } else {
        Serial.println("Not sure what happened");
      }
      delay(100);
    }
  }

  void off() {
    send(WIZMOTE_PROGRAM_DEFAULT, WIZMOTE_BUTTON_OFF);
  }

  void on() {
    send(WIZMOTE_PROGRAM_ON, WIZMOTE_BUTTON_ON);
  }

  void one() {
    send(WIZMOTE_PROGRAM_DEFAULT, WIZMOTE_BUTTON_ONE);
  }

  void two() {
    send(WIZMOTE_PROGRAM_DEFAULT, WIZMOTE_BUTTON_TWO);
  }

  void three() {
    send(WIZMOTE_PROGRAM_DEFAULT, WIZMOTE_BUTTON_THREE);
  }

  void four() {
    send(WIZMOTE_PROGRAM_DEFAULT, WIZMOTE_BUTTON_FOUR);
  }

  void brightUp() {
    send(WIZMOTE_PROGRAM_DEFAULT, WIZMOTE_BUTTON_BRIGHT_UP);
  }

  void brightDown() {
    send(WIZMOTE_PROGRAM_DEFAULT, WIZMOTE_BUTTON_BRIGHT_DOWN);
  }
};

Remote remote = Remote();

// Card
class Card {
private:
  byte gutter    = 20;
  byte radius    = 5;
  byte thiccness = 4;

  uint16_t x, y, w, h;
  uint16_t butOffX, butOffY, butOffW, butOffH;
  uint16_t butOnX, butOnY, butOnW, butOnH;
  uint16_t butOneX, butOneY, butOneW, butOneH;
  uint16_t butTwoX, butTwoY, butTwoW, butTwoH;
  uint16_t butThreeX, butThreeY, butThreeW, butThreeH;
  uint16_t butFourX, butFourY, butFourW, butFourH;
  uint16_t butDimX, butDimY, butDimW, butDimH;
  uint16_t butBrightX, butBrightY, butBrightW, butBrightH;

  void button(String label, int32_t x0, int32_t y0, int32_t w0, int32_t h0,
                       int32_t r, int32_t t, bool pressed) {
    if (pressed) {
      canvas.fillRoundRect(x0, y0, w0, h0, r, canvas.G15);

      canvas.setTextColor(canvas.G0);
    } else {
      drawThiccRoundRect(x0, y0, w0, h0, r, t, canvas.G15);

      canvas.setTextColor(canvas.G15);
    }

    canvas.drawString(label, x0 + (w0 / 2), y0 + (h0 / 2));
  }

  void butOff(uint16_t x0, uint16_t y0, bool pressed) {
    canvas.setFreeFont(FSSB24);
    canvas.setTextSize(2);

    button("Off", x0, y0, butOffW, butOffH, radius, thiccness, pressed);
  }

  void updateOff(bool pressed) {
    canvas.createCanvas(butOffW, butOffH);

    butOff(0, 0, pressed);

    canvas.pushCanvas(butOffX, butOffY, UPDATE_MODE_GLR16);
    canvas.deleteCanvas();
  }

  void butOn(uint16_t x0, uint16_t y0, bool pressed) {
    canvas.setFreeFont(FSSB24);
    canvas.setTextSize(2);

    button("On", x0, y0, butOnW, butOnH, radius, thiccness, pressed);
  }

  void updateOn(bool pressed) {
    canvas.createCanvas(butOnW, butOnH);

    butOn(0, 0, pressed);

    canvas.pushCanvas(butOnX, butOnY, UPDATE_MODE_GLR16);
    canvas.deleteCanvas();
  }

  void butOne(uint16_t x0, uint16_t y0, bool pressed) {
    canvas.setFreeFont(FSSB24);
    canvas.setTextSize(4);

    button("1", x0, y0, butOneW, butOneH, radius, thiccness, pressed);
  }

  void updateOne(bool pressed) {
    canvas.createCanvas(butOneW, butOneH);

    butOne(0, 0, pressed);

    canvas.pushCanvas(butOneX, butOneY, UPDATE_MODE_GLR16);
    canvas.deleteCanvas();
  }

  void butTwo(uint16_t x0, uint16_t y0, bool pressed) {
    canvas.setFreeFont(FSSB24);
    canvas.setTextSize(4);

    button("2", x0, y0, butTwoW, butTwoH, radius, thiccness, pressed);
  }

  void updateTwo(bool pressed) {
    canvas.createCanvas(butTwoW, butTwoH);

    butTwo(0, 0, pressed);

    canvas.pushCanvas(butTwoX, butTwoY, UPDATE_MODE_GLR16);
    canvas.deleteCanvas();
  }

  void butThree(uint16_t x0, uint16_t y0, bool pressed) {
    canvas.setFreeFont(FSSB24);
    canvas.setTextSize(4);

    button("3", x0, y0, butThreeW, butThreeH, radius, thiccness, pressed);
  }

  void updateThree(bool pressed) {
    canvas.createCanvas(butThreeW, butThreeH);

    butThree(0, 0, pressed);

    canvas.pushCanvas(butThreeX, butThreeY, UPDATE_MODE_GLR16);
    canvas.deleteCanvas();
  }

  void butFour(uint16_t x0, uint16_t y0, bool pressed) {
    canvas.setFreeFont(FSSB24);
    canvas.setTextSize(4);

    button("4", x0, y0, butFourW, butFourH, radius, thiccness, pressed);
  }

  void updateFour(bool pressed) {
    canvas.createCanvas(butFourW, butFourH);

    butFour(0, 0, pressed);

    canvas.pushCanvas(butFourX, butFourY, UPDATE_MODE_GLR16);
    canvas.deleteCanvas();
  }

  void butDim(uint16_t x0, uint16_t y0, bool pressed) {
    canvas.setFreeFont(FSSB18);
    canvas.setTextSize(2);

    button("Dim", x0, y0, butDimW, butDimH, radius, thiccness, pressed);
  }

  void updateDim(bool pressed) {
    canvas.createCanvas(butDimW, butDimH);

    butDim(0, 0, pressed);

    canvas.pushCanvas(butDimX, butDimY, UPDATE_MODE_GLR16);
    canvas.deleteCanvas();
  }

  void butBright(uint16_t x0, uint16_t y0, bool pressed) {
    canvas.setFreeFont(FSSB18);
    canvas.setTextSize(2);

    button("Bright", x0, y0, butBrightW, butBrightH, radius, thiccness, pressed);
  }

  void updateBright(bool pressed) {
    canvas.createCanvas(butBrightW, butBrightH);

    butBright(0, 0, pressed);

    canvas.pushCanvas(butBrightX, butBrightY, UPDATE_MODE_GLR16);
    canvas.deleteCanvas();
  }

  void drawThiccRoundRect(int32_t x0, int32_t y0, int32_t w0, int32_t h0,
                        int32_t r, int32_t t, uint32_t color) {
    for (int i = 0; i < t; i++) {
      canvas.drawRoundRect(x0+i, y0+i, w0-(i*2), h0-(i*2), r, color);
    }
  }

  bool inside(uint16_t x1, uint16_t y1, uint16_t x0, uint16_t y0, uint16_t w0, uint16_t h0) {
    if (
      x1 >= x0 &&
      x1 <= x0 + w0 &&
      y1 >= y0 &&
      y1 <= y0 + h0
    ) {
      return true;
    }
    return false;
  }
public:
  Card(uint16_t iX, uint16_t iY, uint16_t iW, uint16_t iH) {
    x = iX;
    y = iY;
    w = iW;
    h = iH;

    uint16_t colW = (w - (gutter * 3)) / 2;
    uint16_t rowH = (h - (gutter * 5)) / 10;

    butOffX = gutter;
    butOffY = gutter;
    butOffW = colW;
    butOffH = rowH * 2;

    butOnX = (gutter * 2) + colW;
    butOnY = gutter;
    butOnW = colW;
    butOnH = rowH * 2;

    butOneX = gutter;
    butOneY = (gutter * 2) + (rowH * 2);
    butOneW = colW;
    butOneH = rowH * 3;

    butTwoX = (gutter * 2) + colW;
    butTwoY = (gutter * 2) + (rowH * 2);
    butTwoW = colW;
    butTwoH = rowH * 3;

    butThreeX = gutter;
    butThreeY = (gutter * 3) + (rowH * 5);
    butThreeW = colW;
    butThreeH = rowH * 3;

    butFourX = (gutter * 2) + colW;
    butFourY = (gutter * 3) + (rowH * 5);
    butFourW = colW;
    butFourH = rowH * 3;

    butDimX = gutter;
    butDimY = (gutter * 4) + (rowH * 8);
    butDimW = colW;
    butDimH = rowH * 2;

    butBrightX = (gutter * 2) + colW;
    butBrightY = (gutter * 4) + (rowH * 8);
    butBrightW = colW;
    butBrightH = rowH * 2;
  }

  void draw() {
    canvas.createCanvas(540, 960);

    //canvas.loadFont("/papyrus.ttf", SD);
    canvas.setTextDatum(MC_DATUM);

    butOff(butOffX, butOffY, false);
    butOn(butOnX, butOnY, false);
    butOne(butOneX, butOneY, false);
    butTwo(butTwoX, butTwoY, false);
    butThree(butThreeX, butThreeY, false);
    butFour(butFourX, butFourY, false);
    butDim(butDimX, butDimY, false);
    butBright(butBrightX, butBrightY, false);

    canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
    canvas.deleteCanvas();
  }

  void pressed(uint16_t x0, uint16_t y0) {
    if (inside(x0, y0, butOffX, butOffY, butOffW, butOffH)) {
      Serial.println("Off");
      updateOff(true);
      remote.off();
      delay(10);
      updateOff(false);
    } else if (inside(x0, y0, butOnX, butOnY, butOnW, butOnH)) {
      Serial.println("On");
      updateOn(true);
      remote.on();
      delay(10);
      updateOn(false);
    } else if (inside(x0, y0, butOneX, butOneY, butOneW, butOneH)) {
      Serial.println("One");
      updateOne(true);
      remote.one();
      delay(10);
      updateOne(false);
    } else if (inside(x0, y0, butTwoX, butTwoY, butTwoW, butTwoH)) {
      Serial.println("Two");
      updateTwo(true);
      remote.two();
      delay(10);
      updateTwo(false);
    } else if (inside(x0, y0, butThreeX, butThreeY, butThreeW, butThreeH)) {
      Serial.println("Three");
      updateThree(true);
      remote.three();
      delay(10);
      updateThree(false);
    } else if (inside(x0, y0, butFourX, butFourY, butFourW, butFourH)) {
      Serial.println("Four");
      updateFour(true);
      remote.four();
      delay(10);
      updateFour(false);
    } else if (inside(x0, y0, butDimX, butDimY, butDimW, butDimH)) {
      Serial.println("Dim");
      updateDim(true);
      remote.brightDown();
      delay(10);
      updateDim(false);
    } else if (inside(x0, y0, butBrightX, butBrightY, butBrightW, butBrightH)) {
      Serial.println("Bright");
      updateBright(true);
      remote.brightUp();
      delay(10);
      updateBright(false);
    }
  }
};

Card card = Card(0, 0, 540, 960);

void TouchLoop() {
  static uint16_t touchpoint[2];

  if (M5.TP.available()) {
    if (!M5.TP.isFingerUp()) {
      M5.TP.update();
      bool is_update = false;

      tp_finger_t FingerItem = M5.TP.readFinger(0);
      if ((touchpoint[0] != FingerItem.x) ||
          (touchpoint[1] != FingerItem.y)) {

          is_update   = true;
          touchpoint[0] = FingerItem.x;
          touchpoint[1] = FingerItem.y;
      }
      if (is_update) {
        Serial.printf("Finger -->X: %d  Y: %d\r\n", touchpoint[0], touchpoint[1]);
        card.pressed(touchpoint[0], touchpoint[1]);
      }
    }
  }
}

// Main Program
void setup() {
  Serial.begin(115200);

  M5.begin();
  M5.TP.SetRotation(90);
  M5.EPD.SetRotation(90);
  M5.EPD.Clear(true);

  remote.setup();
  card.draw();
}

void loop() {
  M5.update();
  TouchLoop();
}
