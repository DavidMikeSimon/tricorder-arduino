void sendHassCommand(const char* url, const char* body) {
  Serial.print("hass command: ");
  Serial.println(url);

  int bodyLen = strlen(body);

  char reqHeaderBuffer[512];
  int reqHeaderLen = sprintf(
    reqHeaderBuffer,
    "POST %s HTTP/1.0\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %u\r\n"
    "Authorization: Bearer %s\r\n"
    "Connection: close\r\n"
    "\r\n",
    url,
    bodyLen,
    HOME_ASSISTANT_API_KEY
  );

  if (!client.connect(HOME_ASSISTANT_HOST, HOME_ASSISTANT_PORT)) {
    Serial.println("Connection failed");
    return;
  }

  client.write(reqHeaderBuffer, reqHeaderLen);
  client.write(body, bodyLen);
  client.stop();
}

void hassEntityService(const char* serviceType, const char* serviceName, const char* entityName) {
  char urlBuffer[256];
  sprintf(
    urlBuffer,
    "/api/services/%s/%s",
    serviceType,
    serviceName
  );
   
  char reqBodyBuffer[256];
  sprintf(
    reqBodyBuffer,
    "{\"entity_id\":\"%s\"}\r\n\r\n",
    entityName
  );

  sendHassCommand(urlBuffer, reqBodyBuffer);
}
