#define AUDIO_BUFSIZE 1024
// TODO: Use I2S in mono mode so we don't need to do this
#define EFFECTIVE_AUDIO_BUFSIZE AUDIO_BUFSIZE/2

uint32_t soundBuf1[AUDIO_BUFSIZE];
uint32_t soundBuf2[AUDIO_BUFSIZE];

Adafruit_ZeroI2S i2s = Adafruit_ZeroI2S();
Adafruit_ZeroDMA soundDMA;
DmacDescriptor* soundDMADesc1;
DmacDescriptor* soundDMADesc2;

void writeAudioBuffer(const uint8_t* source, uint32_t* dest, uint32_t len) {
  if (len < EFFECTIVE_AUDIO_BUFSIZE) {
    memset(dest, 0, AUDIO_BUFSIZE);
  }

  uint32_t* destPtr = dest;
  const uint8_t* endPtr = source + len;
  for (const uint8_t* sourcePtr = source; sourcePtr < endPtr; sourcePtr += 2) {
    uint32_t sample = ((*(sourcePtr + 1) << 24) | (*sourcePtr << 16))/3;
    // TODO: Use I2S in mono mode so we don't need to write the sample once for each channel
    // TODO: Get 22050Hz mode working, so I don't have to double the sample *again*
    *destPtr++ = sample;
    *destPtr++ = sample;
    *destPtr++ = sample;
    *destPtr++ = sample;
  }
}

const uint8_t* audioSourceBuffer = NULL;
uint32_t audioSourceLength;
uint32_t buffersPlayed;
uint32_t buffersRefilled;
bool audioLooping;

void soundBufferSwapCallback(Adafruit_ZeroDMA *dma) {
  buffersPlayed += 1;
}

void playAudio(const uint8_t* buffer, uint32_t length, bool looping = false) {
  int switchValue = digitalRead(PIN_SWITCH);
  if (!switchValue) {
    return;
  }

  audioSourceBuffer = buffer;
  audioSourceLength = length;
  audioLooping = looping;
  buffersPlayed = 0;
  buffersRefilled = 0;

  if (length < EFFECTIVE_AUDIO_BUFSIZE*2) {
    if (length < EFFECTIVE_AUDIO_BUFSIZE) {
      writeAudioBuffer(buffer, soundBuf1, length);
      writeAudioBuffer(buffer, soundBuf2, 0);
    } else {
      writeAudioBuffer(buffer, soundBuf1, EFFECTIVE_AUDIO_BUFSIZE);
      writeAudioBuffer(buffer + EFFECTIVE_AUDIO_BUFSIZE, soundBuf2, length - EFFECTIVE_AUDIO_BUFSIZE);
    }
  } else {
    writeAudioBuffer(buffer, soundBuf1, EFFECTIVE_AUDIO_BUFSIZE);
    writeAudioBuffer(buffer + EFFECTIVE_AUDIO_BUFSIZE, soundBuf2, EFFECTIVE_AUDIO_BUFSIZE);
  }
  
  soundDMA.startJob();
}

void stopAudio() {
  soundDMA.abort();
}

void audioPoll() {
  if (buffersPlayed <= buffersRefilled) {
    return;
  }

  uint32_t* targetBuf = buffersPlayed % 2 == 1 ? soundBuf1 : soundBuf2;
  const uint8_t* sourcePtr = audioSourceBuffer + EFFECTIVE_AUDIO_BUFSIZE*(buffersPlayed+1);
  uint32_t writeLength = EFFECTIVE_AUDIO_BUFSIZE;
  if (sourcePtr + EFFECTIVE_AUDIO_BUFSIZE >= audioSourceBuffer + audioSourceLength) {
    writeLength = audioSourceLength % EFFECTIVE_AUDIO_BUFSIZE;
    if (sourcePtr >= audioSourceBuffer + audioSourceLength) {
      if (audioLooping) {
        buffersPlayed = buffersPlayed % 2;
      } else {
        stopAudio();
      }
    }
  }
  writeAudioBuffer(sourcePtr, targetBuf, writeLength);
  
  buffersRefilled = buffersPlayed;
}



DmacDescriptor* soundDMADescSetup(uint32_t* buf) {
   DmacDescriptor* desc = soundDMA.addDescriptor(
    (void*)buf,
#if defined(__SAMD51__)
    (void*)(&I2S->TXDATA.reg),
#else
    (void*)(&I2S->DATA[0].reg),
#endif
    AUDIO_BUFSIZE,
    DMA_BEAT_SIZE_WORD,
    true,
    false
  );
  if (desc == NULL) {
    Serial.println("Unable to set up sound DMA descriptor");
  }

  // Cause callback to fire after each descriptor is finished, instead of
  // only when reaching the end of the list of descriptors (which will
  // never happen because they are linked in a loop).
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;
  return desc;
}

void soundSetup() {
  soundDMA.setTrigger(I2S_DMAC_ID_TX_0);
  soundDMA.setAction(DMA_TRIGGER_ACTON_BEAT);
  ZeroDMAstatus allocStatus = soundDMA.allocate();
  if (allocStatus != DMA_STATUS_OK) {
    Serial.println("Unable to allocate sound DMA");
    soundDMA.printStatus(allocStatus);
    return;
  }
  soundDMA.setCallback(soundBufferSwapCallback);

  soundDMADesc1 = soundDMADescSetup(soundBuf1);
  soundDMADesc2 = soundDMADescSetup(soundBuf2);

  soundDMA.loop(true);

  if (!i2s.begin(I2S_32_BIT, 44100)) {
    Serial.println("Unable to initialize I2S");
    return;
  }
  
  i2s.enableTx();
}
