void playAudio16(const uint8_t* buffer, uint32_t length) {
  int switchValue = digitalRead(PIN_SWITCH);
  if (!switchValue) {
    return;
  }
  
  delay(3);
  for (uint32_t i=0; i<length; i += 2) {
    // Can't seem to run I2S in 16-bit mode, so let's run it in 32-bit mode
    // and transform our 16-bit sample to 32-bit.
    int32_t sample = (buffer[i+1] << 24) | (buffer[i] << 16);
    i2s.write(sample, sample);
  }
}
