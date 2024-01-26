#include "secrt.h"
#include "pin.h"
String clientId;
char macAddressStr[18];
int  off_time_real;

int init_o_t;

DynamicJsonDocument cert(4000);

String chats="";
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);


void readWiFiCredentials() {
  // Read SSID from EEPROM
  char c;
  eepromAddress=0;
  while ((c = char(EEPROM.read(eepromAddress++))) != '\0') {
    storedSSID += c;
  }

  while ((c = char(EEPROM.read(eepromAddress++))) != '\0') {
    storedPassword += c;
  }
    Serial.print("u: "+storedSSID);
  Serial.print("p: "+storedPassword);

  // Connect to WiFi using stored credentials
 
}

void saveWiFiCredentials(String ssid,String password) {
  // Clear existing WiFi settings
  WiFi.disconnect(true);
  eepromAddress=0;

  // Write SSID to EEPROM
  for (int i = 0; i < ssid.length(); ++i) {
    EEPROM.write(eepromAddress++, ssid[i]);
    delay(10);
  }

  // Add a delimiter between SSID and password
  EEPROM.write(eepromAddress++, '\0');

  // Write password to EEPROM
  for (int i = 0; i < password.length(); ++i) {
    EEPROM.write(eepromAddress++, password[i]);
    delay(10);
  }
   EEPROM.write(eepromAddress++, '\0');

  // Commit the changes
  EEPROM.commit();
  EEPROM.commit();
  
}

void read_button()
{
   if(digitalRead(Push_button)==LOW)
  {
    int c=0;
    while(digitalRead(Push_button)==LOW)
    {
      c++;
      esp_task_wdt_reset();
      delay(1000);
      if(c>5)
      {
        buz_tone(5);
        saveWiFiCredentials("w","123456789");
        ESP.restart();
      }
    }
  }

}
 
void reconnectToWiFi(const String &ssid, const String &password) {
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial. print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    read_button();
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

void handleProvisioning(AsyncWebServerRequest *request) {
  // Extract SSID and password parameters from the URL
  String newSSID = request->arg("ssid");
  String newPassword = request->arg("password");

  // Process provisioning and save WiFi credentials to EEPROM
  saveWiFiCredentials(newSSID, newPassword);

  // Respond to the client
  request->send(200, "text/plain", macAddressStr);

  // Reconnect to WiFi with new credentials
  reconnectToWiFi(newSSID, newPassword);
  ESP.restart();
}

void run_provisioning()
{
  delay(1000);
  EEPROM.begin(256);
  delay(1000);
  readWiFiCredentials();

   if (storedSSID.equals("w")||storedPassword.length()>20) {
         
 // Connect to WiFi
  WiFi.mode(WIFI_AP);
  WiFi.softAP("oflow","123456789");

  // Print ESP32 IP address
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Route for handling provisioning requests
  server.on("/provision", HTTP_GET, handleProvisioning);
  server.begin();
    while(true)
  {
    delay(1000);
    Serial.println("..");
    read_button();
  }
  }
  else
  {
    Serial.println("Stored WiFi credentials:");
    Serial.println("SSID: " + storedSSID);
    Serial.println("Password: " + storedPassword);
    reconnectToWiFi(storedSSID, storedPassword);
  }

}

void updateFirmware() {
   //wm.resetSettings();
  // Start pulling down the firmware binary.
  http.begin(FIRMWARE_URL);
  int httpCode = http.GET();
  if (httpCode <= 0) {
    Serial.printf("HTTP failed, error: %s\n", 
       http.errorToString(httpCode).c_str());
    return;
  }
  // Check that we have enough space for the new binary.
  int contentLen = http.getSize();
  Serial.printf("Content-Length: %d\n", contentLen);
  bool canBegin = Update.begin(contentLen);
  if (!canBegin) {
    Serial.println("Not enough space to begin OTA");
    return;
  }
  // Write the HTTP stream to the Update library.
  WiFiClient* client = http.getStreamPtr();
  size_t written = Update.writeStream(*client);
  Serial.printf("OTA: %d/%d bytes written.\n", written, contentLen);
  if (written != contentLen) {
    Serial.println("Wrote partial binary. Giving up.");
    return;
  }
  if (!Update.end()) {
    Serial.println("Error from Update.end(): " + 
      String(Update.getError()));
    return;
  }
  if (Update.isFinished()) {

  } else {
    Serial.println("Error from Update.isFinished(): " + 
      String(Update.getError()));
    return;
  }
}

void saveCertificateToFS(DynamicJsonDocument doc)
{
  DynamicJsonDocument pem(4000);
  pem["certificatePem"] = doc["certificatePem"];
  pem["privateKey"] = doc["privateKey"];
  File file = SPIFFS.open("/aws3.json", "w");

  if (!file)
  {
    Serial.println("failed to open config file for writing");
  }
  serializeJson(pem, Serial);
  serializeJson(pem, file);
  file.close();
}

void registerThing(DynamicJsonDocument doc)
{
  const char *certificateOwnershipToken = doc["certificateOwnershipToken"];
  DynamicJsonDocument reqBody(4000);
  reqBody["certificateOwnershipToken"] = certificateOwnershipToken;
  reqBody["parameters"]["SerialNumber"] = macAddressStr;
  char jsonBuffer[4000];
  serializeJson(reqBody, jsonBuffer);
  Serial.println("Sending certificate..");
  client.publish("$aws/provisioning-templates/claim_cer2/provision/json", jsonBuffer);
}

void messageHandler(char* topic, byte* payload, unsigned int length) 
{
  int off_r_temp;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
 String receivedData;
 for (int i = 0; i < length; i++) {
      receivedData += (char)payload[i];
  }

  Serial.print("incoming: ");
  Serial.println(topic);
  DynamicJsonDocument doc(length);
  deserializeJson(doc, payload);
  if (strcmp(topic,"$aws/certificates/create/json/accepted")==0)
  {
    saveCertificateToFS(doc);
    registerThing(doc);
  }
  else if (strcmp(topic,"$aws/provisioning-templates/claim_cer2/provision/json/accepted")==0)
  {
    Serial.println("Register things successfully.");
    Serial.println("Restart in 5s.");
    sleep(5);
    ESP.restart();
  }

  Serial.println("Received data: " + receivedData);

   String PUBTopic = String(macAddressStr) + "/chats";
  if (strcmp(topic, PUBTopic.c_str()) == 0)
  {
    chats=receivedData;
  }
   PUBTopic = String(macAddressStr) + "/ota";
  if (strcmp(topic, PUBTopic.c_str()) == 0)
  {
        DynamicJsonDocument jsonDocument(256);
        DeserializationError error = deserializeJson(jsonDocument, receivedData);

        // Check for errors during parsing
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return;
        }
        // Extract values from the JSON object
        int val = jsonDocument["update"];
        if(val==1)
        {
          updateFirmware();
          client.publish(PUBTopic.c_str(),"{\"update\":\"0\"}",true);
          ESP.restart();
        }
  }
}

void connectToAWS(DynamicJsonDocument cert)
{
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(cert["certificatePem"]);
  net.setPrivateKey(cert["privateKey"]);
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  // Create a message handler
  client.setCallback(messageHandler);
  Serial.print("Connecting to AWS IOT.");
  client.connect(macAddressStr);
  if (!client.connected())
  {
    Serial.println(client.state());
    Serial.println("Timeout!");
    ESP.restart();
  }
  Serial.println("Connected");
 
  // String subscriptionTopic = String(AWS_IOT_SUB_TOPIC) + "_" + WiFi.macAddress();
  // char topic[50];
  // subscriptionTopic.toCharArray(topic, 50);
  // Serial.printf("Subscription topic: %s", topic);
  // Serial.println();
  // client.subscribe(topic);
 String subscriptionTopic = String(macAddressStr) + "/chats";
  client.subscribe(subscriptionTopic.c_str());
  subscriptionTopic = String(macAddressStr) + "/ota";
  client.subscribe(subscriptionTopic.c_str());
  subscriptionTopic = String(macAddressStr) + "/vals";
  client.subscribe(subscriptionTopic.c_str());
  delay(2000);
   subscriptionTopic = String(macAddressStr) + "/status";
  client.subscribe(subscriptionTopic.c_str());
  Serial.println("AWS IoT Connected!");
}

void createCertificate()
{
  Serial.println("No file content.");
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  // Create a message handler
  client.setCallback(messageHandler);
  // Set buffer size for receive a certificate.
  client.setBufferSize(4000);
  Serial.print("Connecting to AWS IOT.");
  client.connect(macAddressStr);
  if (!client.connected())
  {
    Serial.println("Timeout!");
    ESP.restart();
  }
  Serial.println("Connected");
  client.subscribe("$aws/certificates/create/json/accepted");
  client.subscribe("$aws/certificates/create/json/rejected");
  client.subscribe("$aws/provisioning-templates/esp32_fleet_prov_template/provision/json/accepted");
  client.subscribe("$aws/provisioning-templates/esp32_fleet_prov_template/provision/json/rejected");
  Serial.println("Create certificate..");
  client.publish("$aws/certificates/create/json", "");
}

void reconnect() {
  while (!client.connected()) {
     pump_control();
     delay(500);
  if(digitalRead(Push_button)==LOW)
  {
    int count=0;
    while(digitalRead(Push_button)==LOW)
    {
      count++;
      delay(1000);
      if(count>5)
      {
        buz_tone(5);
        saveWiFiCredentials("w","123456789");
        ESP.restart();
      }
    }
  }
   if(WiFi.status() == WL_CONNECTED) {
      connectToAWS(cert);
  }
    
    Serial.println("Attempting MQTT connection...");
    if (client.connected()) {
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retry in 5 seconds");
      delay(5000);
    }
  }
}


void setup()
{
  initialize_pin();
  esp_task_wdt_init(300, true);
  esp_task_wdt_add(NULL);
  EEPROM.begin(256);
  Serial.begin(115200);
  delay(1000);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.println(off_t_remains);

  
  snprintf(macAddressStr, sizeof(macAddressStr), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // Connect to AWS IoT using the MAC address as the client ID
  Serial.print("Connecting to AWS IOT with Client ID: ");
  Serial.println(macAddressStr);

  // Convert the MAC address to const char*
  clientId = String(macAddressStr);


  
  run_provisioning();
  Serial.println("Connected to Wi-Fi");


  timeClient.begin();
  timeClient.update();

   if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  // Read AWS config file.
  File file = SPIFFS.open("/aws3.json", "r");

  delay(1000);
  //SPIFFS.remove("/aws3.json");
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }
  auto deserializeError = deserializeJson(cert, file);
  if (!deserializeError)
  {
    if (cert["certificatePem"])
    {
      connectToAWS(cert);
    }
  }
  else
  {
    createCertificate();
  }
  file.close();
  for(int i=0;i<5;i++)
  {
    client.loop();
    delay(1000);
  }

}


void loop()
{
  esp_task_wdt_reset();
  if(!client.connected())
  {
    reconnect();
  }
  client.loop();
  delay(1000);
  read_button();
}
