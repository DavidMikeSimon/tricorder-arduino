#include "fonts/swiss_911_ultra_compressed_12pt.h"
#include "fonts/swiss_911_ultra_compressed_16pt.h"

// Based on Adafruit GFX fillCircleHelper
void lcarsBoxSideHelper(int16_t x0, int16_t y0, int16_t r,
                      int16_t h, bool left,
                      int8_t topMode, int8_t bottomMode,
                      uint16_t color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t px = x;
  int16_t py = y;
  int16_t delta =  h - 2 * r - 1;

  delta++; // Avoid some +1's in the loop

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (x < (y + 1)) {
      int16_t lineX = left ? x0 - x : x0 + x;
      int16_t lineY0 = topMode == 0 ? y0 - r : (topMode < 0 ? y0 - r*2 + y : y0 - y);
      int16_t lineY1 = bottomMode == 0 ? y0 - r + h : (bottomMode < 0 ? y0 + h - y : y0 + y + delta);
      tft.writeFastVLine(lineX, lineY0, lineY1-lineY0, color);
    }
    if (y != py) {
      int16_t lineX = left ? x0 - py : x0 + py;
      int16_t lineY0 = topMode == 0 ? y0 - r : (topMode < 0 ? y0 - r*2 + px : y0 - px);
      int16_t lineY1 = bottomMode == 0? y0 - r + h : (bottomMode < 0 ? y0 + h - px : y0 + px + delta);
      tft.writeFastVLine(lineX, lineY0, lineY1-lineY0, color);
      py = y;
    }
    px = x;
  }
}

void lcarsBox(
  uint16_t x0, uint16_t y0,
  uint16_t x1, uint16_t y1,
  int16_t r0, int16_t r1,
  int8_t topLeftMode, int8_t topRightMode,
  int8_t bottomLeftMode, int8_t bottomRightMode,
  uint16_t color
) {
  tft.startWrite();
  int w = x1-x0;
  int h = y1-y0;
  lcarsBoxSideHelper(x0 + r0, y0 + r0, r0, h, true, topLeftMode, bottomLeftMode, color);
  tft.writeFillRect(x0 + r0, y0, w - (r0 + r1), h, color);
  lcarsBoxSideHelper(x0 + w - r1 - 1, y0 + r1, r1, h, false, topRightMode, bottomRightMode, color);
  tft.endWrite();
}

#define LCARS_ORANGE    0xFCA6
#define LCARS_BLUE      0x333F
#define LCARS_YELLOW    0xFFF2
#define LCARS_RED       0xD328

enum LcarsWidgetType {
  LCARS_WIDGET_DECO,
  LCARS_WIDGET_BUTTON,
};

#define LCARS_TOP_LEFT_BEVEL_IN 1
#define LCARS_TOP_LEFT_BEVEL_OUT 2
#define LCARS_TOP_RIGHT_BEVEL_IN 4
#define LCARS_TOP_RIGHT_BEVEL_OUT 8
#define LCARS_BOTTOM_LEFT_BEVEL_IN 16
#define LCARS_BOTTOM_LEFT_BEVEL_OUT 32
#define LCARS_BOTTOM_RIGHT_BEVEL_IN 64
#define LCARS_BOTTOM_RIGHT_BEVEL_OUT 128

#define LCARS_ALL_BEVEL_IN (LCARS_TOP_LEFT_BEVEL_IN | LCARS_TOP_RIGHT_BEVEL_IN | LCARS_BOTTOM_LEFT_BEVEL_IN | LCARS_BOTTOM_RIGHT_BEVEL_IN)

struct LcarsWidgetBox {
  uint8_t x0;
  uint8_t y0;
  uint8_t x1;
  uint8_t y1;
  int8_t r0;
  int8_t r1;
  uint8_t bevelFlags;
  const char* label;
  LcarsWidgetBox* nextBox;
};

struct LcarsWidget {
  LcarsWidgetType type;
  uint16_t color;
  LcarsWidgetBox* firstBox;
  bool dirty;
  void (*callback)();
};

void drawWidget(const LcarsWidget* widget) {
  LcarsWidgetBox* box = widget->firstBox;
  while (box != NULL) {
    // Drawable points from 0, 124 to 240, 320
    lcarsBox(
      (uint16_t)box->x0,
      (uint16_t)box->y0 + 124,
      (uint16_t)box->x1,
      (uint16_t)box->y1 + 124,
      (int16_t)box->r0,
      (int16_t)box->r1,
      box->bevelFlags & LCARS_TOP_LEFT_BEVEL_IN ? 1 : (box->bevelFlags & LCARS_TOP_LEFT_BEVEL_OUT ? -1 : 0),
      box->bevelFlags & LCARS_TOP_RIGHT_BEVEL_IN ? 1 : (box->bevelFlags & LCARS_TOP_RIGHT_BEVEL_OUT ? -1 : 0),
      box->bevelFlags & LCARS_BOTTOM_LEFT_BEVEL_IN ? 1 : (box->bevelFlags & LCARS_BOTTOM_LEFT_BEVEL_OUT ? -1 : 0),
      box->bevelFlags & LCARS_BOTTOM_RIGHT_BEVEL_IN ? 1 : (box->bevelFlags & LCARS_BOTTOM_RIGHT_BEVEL_OUT ? -1 : 0),
      widget->color
    );

    if (box->label != NULL) {
      bool big = box->y1 - box->y0 > 24;
      if (big) {
        tft.setFont(&swiss_911_ultra_compressed_bt16pt7b);
      } else {
        tft.setFont(&swiss_911_ultra_compressed_bt12pt7b);
      }
      tft.setTextColor(0);
      tft.setCursor(box->x0 + (box->bevelFlags & LCARS_BOTTOM_LEFT_BEVEL_IN ? 12 : 2), box->y1 + 124 - (big ? 4 : 2));
      tft.print(box->label);
    }
    
    box = box->nextBox;
  }
}

#define MAX_WIDGETS 32

int numWidgets = 0;
LcarsWidget widgets[MAX_WIDGETS];

int getNumWidgetsOfType(LcarsWidgetType type) {
  int num = 0;
  for (size_t i = 0; i < numWidgets; ++i) {
    if (widgets[i].type == type) {
      num++;
    }
  }
  return num;
}

size_t getNthWidgetIdxOfType(LcarsWidgetType type, int n) {
  size_t idx = 0;
  size_t found = 0;
  while (true) {
    if (widgets[idx].type == type) {
      found++;
      if (found > n) {
        return idx;
      }
    }
    idx++;
  }
}

void drawDirtyWidgets() {
  for (size_t i = 0; i < numWidgets; ++i) {
    if (widgets[i].dirty) {
      drawWidget(&widgets[i]);
      widgets[i].dirty = false;
    }
  }
}

int nextColorChangeTime = 0;

void changeRandomColor() {
  if (millis() < nextColorChangeTime) {
    return;
  }

  uint16_t numDecoWidgets = getNumWidgetsOfType(LCARS_WIDGET_DECO);
  if (numDecoWidgets == 0) {
    return;
  }

  uint16_t newColor;
  switch (random(4)) {
    case 0: newColor = LCARS_ORANGE; break;
    case 1: newColor = LCARS_RED; break;
    case 2: newColor = LCARS_BLUE; break;
    default: newColor = LCARS_YELLOW; break;
  }

  uint16_t widgetIdx = getNthWidgetIdxOfType(LCARS_WIDGET_DECO, random(numDecoWidgets));
  widgets[widgetIdx].color = newColor;
  widgets[widgetIdx].dirty = true;

  drawDirtyWidgets();
  nextColorChangeTime = millis() + random(800, 2000);
}

void resetWidgets() {
  // TODO: Reset button position
  // TODO: Free lcars boxes
  numWidgets = 0;
}

void addWidget(LcarsWidgetType type, uint16_t color, void (*callback)() = NULL) {
  widgets[numWidgets] = { type, color, NULL, true, callback };
  numWidgets++;
}

void addWidgetBox(LcarsWidgetBox newBox) {
  LcarsWidgetBox* heapBox = (LcarsWidgetBox*)malloc(sizeof(LcarsWidgetBox));
  memcpy(heapBox, &newBox, sizeof(LcarsWidgetBox));
  if (widgets[numWidgets-1].firstBox == NULL) {
    widgets[numWidgets-1].firstBox = heapBox;
  } else {
    LcarsWidgetBox* lastBox = widgets[numWidgets-1].firstBox;
    while (lastBox->nextBox != NULL) {
      lastBox = lastBox->nextBox;
    }
    lastBox->nextBox = heapBox;
  }
}

void addButton(const char* label, void (*callback)()) {
  int i = getNumWidgetsOfType(LCARS_WIDGET_BUTTON);
  addWidget(LCARS_WIDGET_BUTTON, i == 0 ? LCARS_ORANGE : LCARS_BLUE, callback);
  addWidgetBox({
    50 + (i/5)*64, 16 + (i%5)*25,
    110 + (i/5)*64, 16 + (i%5)*25 + 20,
    10, 10,
    LCARS_ALL_BEVEL_IN,
    label,
    NULL
  });
}

void cornerLampOff() {
  hassEntityService("switch", "turn_off", "switch.corner_lamp");
}

void cornerLampOn() {
  hassEntityService("switch", "turn_on", "switch.corner_lamp");
}

void tvOff() {
  hassEntityService("media_player", "turn_off", "media_player.sony_bravia_tv");
}

void tvOn() {
  hassEntityService("media_player", "turn_on", "media_player.sony_bravia_tv");
}

void tvPlayPause() {
  hassEntityService("script", "turn_on", "script.contextual_media_play_pause");
}

void tvVolumeUp() {
  hassEntityService("media_player", "volume_up", "media_player.sony_bravia_tv");
}

void tvVolumeDown() {
  hassEntityService("media_player", "volume_down", "media_player.sony_bravia_tv");
}

void scan() {
  playAudio16(tng_tricorder_scan_wav, TNG_TRICORDER_SCAN_WAV_LEN);
}

void setupMainMenu() {
  resetWidgets();

  addButton("L ON", &cornerLampOn);
  addButton("L OFF", &cornerLampOff);
  addButton("SC ON", &tvOn);
  addButton("SC OFF", &tvOff);
  addButton("PL/PS", &tvPlayPause);
  addButton("VOL+", &tvVolumeUp);
  addButton("VOL-", &tvVolumeDown);
  addButton("SCAN", &scan);

  addWidget(LCARS_WIDGET_DECO, LCARS_ORANGE);
  addWidgetBox({
    0, 0,
    40, 60,
    20, 0,
    LCARS_TOP_LEFT_BEVEL_IN,
    NULL,
    NULL
  });
  addWidgetBox({
    40, 0,
    140, 11,
    20, 0,
    LCARS_BOTTOM_LEFT_BEVEL_OUT,
    NULL,
    NULL
  });

  addWidget(LCARS_WIDGET_DECO, LCARS_RED);
  addWidgetBox({
    144, 0,
    240, 11,
    0, 5,
    LCARS_TOP_RIGHT_BEVEL_IN | LCARS_BOTTOM_RIGHT_BEVEL_IN,
    NULL,
    NULL
  });

  addWidget(LCARS_WIDGET_DECO, LCARS_ORANGE);
  addWidgetBox({
    0, 64,
    40, 80,
    0, 0,
    0,
    NULL,
    NULL
  });

  addWidget(LCARS_WIDGET_DECO, LCARS_YELLOW);
  addWidgetBox({
    0, 84,
    40, 136,
    20, 0,
    LCARS_BOTTOM_LEFT_BEVEL_IN,
    NULL,
    NULL
  });

  addWidget(LCARS_WIDGET_DECO, LCARS_ORANGE);
  addWidgetBox({
    0, 140,
    40, 160,
    20, 0,
    LCARS_TOP_LEFT_BEVEL_IN,
    NULL,
    NULL
  });
  addWidgetBox({
    40, 140,
    164, 148,
    10, 0,
    LCARS_BOTTOM_LEFT_BEVEL_OUT,
    NULL,
    NULL
  });

  addWidget(LCARS_WIDGET_DECO, LCARS_ORANGE);
  addWidgetBox({
    168, 140,
    240, 148,
    0, 0,
    0,
    NULL,
    NULL
  });
  
  addWidget(LCARS_WIDGET_DECO, LCARS_RED);
  addWidgetBox({
    0, 164,
    44, 196,
    0, 0,
    0,
    "BK",
    NULL,
  });

  addWidget(LCARS_WIDGET_DECO, LCARS_BLUE);
  addWidgetBox({
    48, 164,
    130, 196,
    0, 0,
    0,
    "SEL",
    NULL,
  });

  addWidget(LCARS_WIDGET_DECO, LCARS_YELLOW);
  addWidgetBox({
    134, 164,
    216, 196,
    0, 0,
    0,
    "NXT",
    NULL,
  });
}

int nextButtonActionTime = 0;
int buttonDown = 0;
int curButton = 0;

void checkInput() {
  if (millis() < nextButtonActionTime) {
    return;
  }

  int upValue = !digitalRead(PIN_JOY_UP);
  int downValue = !digitalRead(PIN_JOY_DOWN);
  int btnValue = !digitalRead(PIN_JOY_BTN);
  
  int geoValue = !digitalRead(PIN_BTN_GEO);
  int metValue = !digitalRead(PIN_BTN_MET);
  int bioValue = !digitalRead(PIN_BTN_BIO);

  int button = -100;

  if (upValue) {
    button = PIN_JOY_UP;
  } else if (downValue) {
    button = PIN_JOY_DOWN;
  } else if (btnValue) {
    button = PIN_JOY_BTN;
  }

  if (buttonDown == 1) {
    if (button == -100) {
      buttonDown = 0;
    }
  } else if (button != -100) {
    buttonDown = 1;
    nextButtonActionTime = millis() + 100;

    int numButtons = getNumWidgetsOfType(LCARS_WIDGET_BUTTON);

    if (numButtons == 0) {
      return;
    }

    int lastButton = curButton;

    if (button == PIN_JOY_UP) {
      curButton -= 1;
    } else if (button == PIN_JOY_BTN) {
      void (*callback)() = widgets[getNthWidgetIdxOfType(LCARS_WIDGET_BUTTON, curButton)].callback;
      if (callback != NULL) {
        callback();
      }
    } else if (button == PIN_JOY_DOWN) {
      curButton += 1;
    }

    if (curButton < 0) {
      curButton += numButtons;
    } else {
      curButton %= numButtons;
    }

    if (curButton != lastButton) {
      size_t lastButtonIdx = getNthWidgetIdxOfType(LCARS_WIDGET_BUTTON, lastButton);
      size_t curButtonIdx = getNthWidgetIdxOfType(LCARS_WIDGET_BUTTON, curButton);
      widgets[lastButtonIdx].color = LCARS_BLUE;
      widgets[lastButtonIdx].dirty = true;
      widgets[curButtonIdx].color = LCARS_ORANGE;
      widgets[curButtonIdx].dirty = true;
      drawDirtyWidgets();
    }
  }
}
