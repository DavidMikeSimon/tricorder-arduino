void dnaScanSetup() {
  tft.fillScreen(ST77XX_BLACK);

  addWidget(LCARS_WIDGET_DECO, LCARS_ORANGE);
  addWidgetBox({
    0, 0,
    24, 80,
    20, 0,
    LCARS_TOP_LEFT_BEVEL_IN,
    NULL,
    NULL
  });
  addWidgetBox({
    24, 0,
    160, 30,
    20, 0,
    LCARS_BOTTOM_LEFT_BEVEL_OUT,
    "ENV SCAN",
    NULL
  });

  addWidget(LCARS_WIDGET_DECO, LCARS_RED);
  addWidgetBox({
    164, 0,
    240, 30,
    0, 5,
    LCARS_TOP_RIGHT_BEVEL_IN | LCARS_BOTTOM_RIGHT_BEVEL_IN,
    "SNSR",
    NULL
  });

  addWidget(LCARS_WIDGET_DECO, LCARS_ORANGE);
  addWidgetBox({
    0, 84,
    24, 180,
    20, 0,
    LCARS_BOTTOM_LEFT_BEVEL_IN,
    NULL,
    NULL
  });

  drawDirtyWidgets();
  
  playAudio(tng_tricorder_scan_wav, TNG_TRICORDER_SCAN_WAV_LEN, true);
}

void dnaScanInput(int button) {
  stopAudio();
  setMode(MODE_MAIN_MENU);
}

void dnaScanPoll() {
  uint16_t color = ST77XX_BLACK;
  if (millis()/1000 % 2 == 0) {
    color = random(10000) + ((millis()/1000) % 5)*10000;
  }
  int16_t lineX = random(190) + 40;
  int16_t lineY = random(100) + 180;
  int16_t lineLen = random(35);
  tft.startWrite();
  tft.writeFastVLine(lineX, lineY, lineLen, color);
  tft.endWrite();
}
