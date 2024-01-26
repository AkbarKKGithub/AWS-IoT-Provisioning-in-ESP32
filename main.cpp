#include "secrt.h"
#include "pin.h"
String clientId;
char macAddressStr[18];
int  off_time_real;

int init_o_t;
 
float h ;
float t;
DynamicJsonDocument cert(4000);

String chats="";
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);


void flash(int x)
{
  if(x==1)
  {
    digitalWrite(Top_Led,HIGH);
    delay(100);
    digitalWrite(Top_Led,LOW);
    delay(100);
  }
  else if(x==2)
  {
    digitalWrite(Fault_Led,HIGH);
    delay(100);
    digitalWrite(Fault_Led,LOW);
    delay(100);
  }
   else if(x==3)
  {
    digitalWrite(Mid_Led,HIGH);
    delay(100);
    digitalWrite(Mid_Led,LOW);
    delay(100);
  }
   else if(x==4)
  {
    digitalWrite(Dry_Led,HIGH);
    delay(100);
    digitalWrite(Dry_Led,LOW);
    delay(100);
  }
  else
  {
  digitalWrite(Mid_Led, HIGH);
  delay(100);
  digitalWrite(Mid_Led, LOW);
  delay(100);
  }
}

inline void buz_tone(uint8_t x) {
  for (int i; i < x; i++) {
    digitalWrite(Buzzer, HIGH);
    delay(30);
    digitalWrite(Buzzer, LOW);
    delay(100);
  }
}

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
    flash(3);
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
    flash(3);
    flash(1);
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

void updateStatus(String p_s)
{
  StaticJsonDocument<200> doc;
  doc["p"] = p_s;
  doc["o"] = "1";
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  String PUBTopic = String(macAddressStr) + "/status";
  client.publish(PUBTopic.c_str(),jsonBuffer,true);
}

void chat(String msg)
{
     timeClient.update();
     chats.concat(timeClient.getEpochTime()+msg+",");
     int length = chats.length();
     String limitedData;

// Extract the last 59 characters
if(length>70)
    limitedData = chats.substring(length - 59, length);
  else
  limitedData=chats;
  String PUBTopic = String(macAddressStr) + "/chats";
     client.publish(PUBTopic.c_str(), limitedData.c_str(), true);
}

void pump_off()
{
  if(pump_flag==1)
  {
  digitalWrite(Pump,LOW);
  buz_tone(2);
  pump_flag=0;
  chat("0");
   off_time=off_time_real;
   off_t_remains=0;
   off_time =(off_time*60000)-(off_t_remains*60000);
   
  }
}

void pump_on()
{
  if(pump_flag==0)
  {
  Serial.print("ooon");
  digitalWrite(Pump,HIGH);
  off_time_millis=millis();
  buz_tone(3);
  pump_flag=1;
 // updateStatus("1");
  chat("1");
  off_t_remains=EEPROM.read(60);
  Serial.println(off_t_remains);
  off_time=off_time-(off_t_remains*60000);
  }
}

void initialize_pin() {
  pinMode(Top_Led, OUTPUT);
  pinMode(Mid_Led, OUTPUT);
  pinMode(Dry_Led, OUTPUT);
  pinMode(Fault_Led, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(Pump, OUTPUT);
  digitalWrite(Pump,LOW);

  pinMode(33, INPUT);
  pinMode(Top_sens, INPUT);
  pinMode(Voltge_sens, INPUT);
  pinMode(Push_button, INPUT);
  pinMode(Dry_sens,INPUT);

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

void auotOff()
{
     if (pump_flag==1)
     {
        Serial.print("off t=");
        Serial.println((millis() - off_time_millis)/1000);
        int o_t=(millis() - off_time_millis)/1000;
        Serial.print("off timer=");
        Serial.println(off_time/1000 );
        if(o_t%60==0&&o_t>50)
        {
          EEPROM.write(60,off_t_remains+(o_t/60));
          EEPROM.commit();
          Serial.println(EEPROM.read(60));
          //update time
        }

     }

   if (pump_flag==1&&(millis() - off_time_millis > off_time || off_time_millis == 0))
    {
      EEPROM.write(60,0);
      EEPROM.commit();
      pump_off();
      updateStatus("0");
      Serial.println("Timer Exeeded");
      chat("3");
    }

}

int check_voltage()
{
  if(pump_flag==1)
  {
    return ((analogRead(34)+290)/10)-70;
  }
  else
  {
    return ((analogRead(34))/10)-70;
  }
}

float check_current()
{
  float vl=0;
   for(int i=0;i<20;i++)
  {
    vl=vl+analogRead(33);
    delay(10);
  }
  vl=vl/20;
    return vl/219;
  
}

void volt_check() 
{ 
  //check volt h/l
 if(pump_flag==1||v_f==1)
  {
  if((check_voltage())<min_voltage||(check_voltage())>max_voltage)
  {
    flash(2);
    v_error=v_error+2;
     if(v_error>20)
    v_error=20;
  }
  else
  {
    v_error--;
    if(v_error<2)
    v_error=0;
  }
  Serial.print("v_error=");
     Serial.println(v_error);
if(v_error>10&&v_f==0)
{
  pump_off();
  delay(200);
  updateStatus("0");
  delay(200);
  chat("5");
  Serial.print("v_errour=");
  Serial.println(v_error);

  v_f=1;

  Serial.println(off_time_min);
}
if(v_error<3&&v_f==1)
{

  pump_on();
  delay(200);
  updateStatus("1");
  v_f=0;
}
  }
}

void dry_check() 
{
  //check volt h/l
 if(pump_flag==1)
  {
  if((check_current()<min_current||check_current()>max_current)&&check_current()>0)
  {
    c_error=c_error+2;
     if(c_error>80)
    c_error=80;
    flash(4);
  }
  else
  {
    c_error--;
    if(c_error<2)
    c_error=0;
  }
  Serial.print("c_error=");
     Serial.println(c_error);
if(c_error>20)
{
  if(pump_flag==1)
  {
  EEPROM.write(60,0);
  EEPROM.commit();
  }
  pump_off();
  delay(200);
  updateStatus("0");
  delay(500);
  chat("4");
}
  }
}

void pump_control() {
  auotOff();
  volt_check();
  dry_check();

  if (fault_flag==0) {
    if (top_lvl_flag) {
     // pump_off();  //disabled for sensor removal
      sensor_e_c_flag = 1;
    } else if (!top_lvl_flag && !mid_lvl_flag && !sensor_e_c_flag) {
      // pump_on();
    }
    if (mid_lvl_flag)
      sensor_e_c_flag = 0;
  }
  else
  {

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
 PUBTopic = String(macAddressStr) + "/vals";
  if (strcmp(topic,PUBTopic.c_str()) == 0)
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
      off_time_real = jsonDocument["off_time"];
      min_voltage = jsonDocument["min_volt"];
      min_current = jsonDocument["min_current"];
      max_current = jsonDocument["max_current"];
      //off_time_real = jsonDocument["off_time"];
      off_time=off_time_real;  
   off_time =(off_time*60000);
}
 PUBTopic = String(macAddressStr) + "/status";
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
    int pump = jsonDocument["p"];
    int onl = jsonDocument["o"];
    if(pump==1)
    {
      pump_on();
    }
    else if(pump==0)
    {
      if(pump_flag==1)
      {
        EEPROM.write(60,0);
        EEPROM.commit();
      }
      pump_off();
    }
    if(onl==0)
    {
    StaticJsonDocument<200> doc;
    pump_flag ? doc["p"] = "1" :doc["p"] = "0";
    doc["o"] = "1";
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer); // print to client
    PUBTopic = String(macAddressStr) + "/status";
    client.publish(PUBTopic.c_str(),jsonBuffer,true);
    StaticJsonDocument<200> doc1;
    doc1["v"] = check_voltage();
    doc1["a"] =check_current();
    char jsonBuffer2[512];
    serializeJson(doc1, jsonBuffer2); // print to client
    PUBTopic = String(macAddressStr) + "/pow";
    client.publish(PUBTopic.c_str(),jsonBuffer2,true);
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

void connectAWS()
{
// {
//   // Configure WiFiClientSecure to use the AWS IoT device credentials
//   net.setCACert(AWS_CERT_CA);
//   net.setCertificate(AWS_CERT_CRT);
//   net.setPrivateKey(AWS_CERT_PRIVATE);

  
 
//   // Connect to the MQTT broker on the AWS endpoint we defined earlier
//   client.setServer(AWS_IOT_ENDPOINT, 8883);
 
//   // Create a message handler
//   client.setCallback(messageHandler);
 
//   Serial.println("Connecting to AWS IOT");
 
//   while (!client.connect(THINGNAME))
//   {
//     Serial.print(".");
//     delay(100);
//   }
 
//   if (!client.connected())
//   {
//     Serial.println("AWS IoT Timeout!");
//     return;
//   }
 
//   // Subscribe to a topic
//   client.subscribe("div/status");
//   client.subscribe("div/chats");
//   client.subscribe("div/ota"); 
//   client.subscribe("div/vals"); 
//   Serial.println("AWS IoT Connected!");
} 

void reconnect() {
  while (!client.connected()) {
    flash(3);
    flash(3);
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
  digitalWrite(Buzzer,1);
  delay(200);
  digitalWrite(Buzzer,0);
  Serial.begin(115200);
  delay(1000);
  off_t_remains=EEPROM.read(60);
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
    //    Serial.println("Certificate PEM:");
    // Serial.println(cert["certificatePem"].as<String>());

    // // You can also print the private key if needed
    // Serial.println("Private Key:");
    // Serial.println(cert["privateKey"].as<String>());

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
  else
  {
    flash(1);
  }
  client.loop();
  pump_control();
  delay(1000);
  
  read_button();
}