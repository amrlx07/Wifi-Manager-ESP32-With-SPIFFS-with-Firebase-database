// WiFi Library
#include <WiFi.h>
// File System Library
#include <FS.h>
// SPI Flash Syetem Library
#include <SPIFFS.h>
// WiFiManager Library
#include <WiFiManager.h>
// Arduino JSON library
#include <ArduinoJson.h>
#include "FirebaseESP32.h"
#include "time.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
 
// JSON configuration file
#define JSON_CONFIG_FILE "/test_config.json"

// Insert Firebase project API Key
// ganti
#define API_KEY "AIzaSyCXCjDHUKVPnnuA89A6akryxY2P8xPqM10"
 
// Insert Authorized Email and Corresponding Password
// ganti
#define USER_EMAIL "wmktefaternakpolije@gmail.com"
#define USER_PASSWORD "wmktefaternak21"

// Insert RTDB URLefine the RTDB URL
// ganti
#define DATABASE_URL "https://contoh-database-dc882-default-rtdb.firebaseio.com/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)// sesuaikan folder database
String databasePath;
// Database child nodes
String tempPath = "/Suhu";
String humPath = "/Kelembapan";
String Datetime = "/DateTime";


// Parent Node (to be updated in every loop)
String parentPath;

char datetime[80];  // Buffer untuk menyimpan string datetime

int timestamp;
int timestamp1;


FirebaseJson json;

const char* ntpServer = "pool.ntp.org";

float temperature;
float humidity;
float pressure;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 10000;
unsigned long timerDelay2 = 1000;

float randomNumberHumidity;
float randomNumberTemperature;

int IntValueHeater;
int IntValueFan;
int IntValueMotor;

int intValue;
int floatValue;
// Flag for saving data
bool shouldSaveConfig = true;
 
// Variables to hold data from custom textboxes
char testString[50] = "test value";
int testNumber = 1234;
 
// Define WiFiManager Object
WiFiManager wm;

// Deklarasi fungsi convert UnixCode
void unixToDateTime(long int unixTime, char* buffer) {
  // Deklarasi variabel
  time_t rawTime = unixTime;
  tm* timeInfo;

  // Mengubah UNIX code ke struct tm
  timeInfo = localtime(&rawTime);

  // Memformat waktu ke string
  strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeInfo);
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}
 
void saveConfigFile()
// Save Config in JSON format
{
  Serial.println(F("Saving configuration..."));
  
  // Create a JSON document
  StaticJsonDocument<512> json;
  json["testString"] = testString;
  json["testNumber"] = testNumber;
 
  // Open config file
  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile)
  {
    // Error, file did not open
    Serial.println("failed to open config file for writing");
  }
 
  // Serialize JSON data to write to file
  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0)
  {
    // Error writing file
    Serial.println(F("Failed to write to file"));
  }
  // Close file
  configFile.close();
}
 
bool loadConfigFile()
// Load existing configuration file
{
  // Uncomment if we need to format filesystem
  // SPIFFS.format();
 
  // Read configuration from FS json
  Serial.println("Mounting File System...");
 
  // May need to make it begin(true) first time you are using SPIFFS
  if (SPIFFS.begin(false) || SPIFFS.begin(true))
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists(JSON_CONFIG_FILE))
    {
      // The file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      if (configFile)
      {
        Serial.println("Opened configuration file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error)
        {
          Serial.println("Parsing JSON");
 
          strcpy(testString, json["testString"]);
          testNumber = json["testNumber"].as<int>();
 
          return true;
        }
        else
        {
          // Error loading JSON data
          Serial.println("Failed to load json config");
        }
      }
    }
  }
  else
  {
    // Error mounting file system
    Serial.println("Failed to mount FS");
  }
 
  return false;
}
 
 
void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
 
void configModeCallback(WiFiManager *myWiFiManager)
// Called when config mode launched
{
  Serial.println("Entered Configuration Mode");
 
  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
 
  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}
 
void setup()
{
  // Change to true when testing to force configuration every time we run
  bool forceConfig = false;
 
  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup)
  {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }
 
  // Explicitly set WiFi mode
  WiFi.mode(WIFI_STA);
 
  // Setup Serial monitor
  Serial.begin(115200);
  delay(10);
 
  // Reset settings (only for development)
  //wm.resetSettings();
 
  // Set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);
 
  // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);
 
  // Custom elements
 
  // Text box (String) - 50 characters maximum
  WiFiManagerParameter custom_text_box("key_text", "Enter your string here", testString, 50);
  
  // Need to convert numerical input to string to display the default value.
  char convertedValue[6];
  sprintf(convertedValue, "%d", testNumber); 
  
  // Text box (Number) - 7 characters maximum
  WiFiManagerParameter custom_text_box_num("key_num", "Enter your number here", convertedValue, 7); 
 
  // Add all defined parameters
  wm.addParameter(&custom_text_box);
  wm.addParameter(&custom_text_box_num);
 
  if (forceConfig)
    // Run if we need a configuration
  {
    if (!wm.startConfigPortal("ARCI EGG_AP", "password"))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
  }
  else
  {
    if (!wm.autoConnect("ARCI EGG_AP", "password"))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // if we still have not connected restart and try all over again
      ESP.restart();
      delay(5000);
    }
  }
 
  // If we get here, we are connected to the WiFi
 
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  // Lets deal with the user config values
 
  // Copy the string value
  strncpy(testString, custom_text_box.getValue(), sizeof(testString));
  Serial.print("testString: ");
  Serial.println(testString);
 
  //Convert the number value
  testNumber = atoi(custom_text_box_num.getValue());
  Serial.print("testNumber: ");
  Serial.println(testNumber);
 
 
  // Save the custom parameters to FS
  if (shouldSaveConfig)
  {
    saveConfigFile();
  }
  //NTP Server time
  configTime(0, 0, ntpServer);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path (sesuaikan)
  databasePath = "/DemoDatabase/Monitoring/";  //"/UsersData/" + uid + "/readings";
}
 
 
 
 
void loop() {
   //ganti masukan nilai dari variabel sensor
  randomNumberHumidity = random(99.1278354) + 0.25;
  randomNumberTemperature = random(49.120983456) + 0.25;
  delay(6000);
  Serial.print("Kelembaban: ");
  Serial.println(randomNumberHumidity);
  Serial.print("Suhu: ");
  Serial.println(randomNumberTemperature);

  //read data from firebase
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay2 || sendDataPrevMillis == 0)) {
    //read data dari firebase
    if (Firebase.RTDB.getInt(&fbdo, "/Controlling/SetPointUp")) {
      if (fbdo.dataType() == "int") {
        intValue = fbdo.intData();
        Serial.print("Set Poin Atas :");
        Serial.println(intValue);
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.getFloat(&fbdo, "/Controlling/SetPointDown")) {
      if (fbdo.dataType() == "int") {
        floatValue = fbdo.floatData();
        Serial.print("Set Poin Bawah :");
        Serial.println(floatValue);
      }
    } else {
      Serial.println(fbdo.errorReason());
    }

    // Send new readings to database(mengirim data ke firebase)
    if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
      sendDataPrevMillis = millis();


      //Get current timestamp// timestamp gak usah
      timestamp = getTime();
      timestamp1 = timestamp + 25200;
      Serial.print("time: ");
      Serial.println(timestamp1);

      // Memanggil fungsi untuk mengubah UNIX code ke format datetime
      unixToDateTime(timestamp1, datetime);
      Serial.print("Datetime: ");
      Serial.println(datetime);

      //path monitoring
      parentPath = databasePath;
      Firebase.RTDB.setFloat(&fbdo,parentPath + tempPath,randomNumberTemperature);
      Firebase.RTDB.setFloat(&fbdo,parentPath + humPath,randomNumberHumidity);

      //sesuaikan
      parentPath = databasePath + "data/" + String(timestamp1);
      //gunakan path data list yang sudah di inisialisasi 
      json.set(tempPath.c_str(), (randomNumberTemperature));
      json.set(humPath.c_str(), (randomNumberHumidity));
      json.set(Datetime.c_str(), (datetime));
      Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
    }
  }
}