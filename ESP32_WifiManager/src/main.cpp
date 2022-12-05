/*******************************************************************
    Example project using wifimanager to config parts of your project For the ESP32
    As well as the regular WiFiManager stuff
    It shows example config for a:
    - Text Box (String)
    - Text Box (number)
    - Checkbox (bool)
    - Dropdown (number, but could be string)
    It will enter config mode if
    - Cant connect to the Wifi
    - config doesnt exist on the FS
    - User has "double reset" (pressed the reset button twice)
    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/
    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// ----------------------------
// Library Defines - Need to be defined before library import
// ----------------------------

#define ESP_DRD_USE_SPIFFS true

// ----------------------------
// Standard Libraries - Already Installed if you have ESP32 set up
// ----------------------------

#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <WiFiManager.h>
// Captive portal for configuring the WiFi

// Can be installed from the library manager (Search for "WifiManager", install the Alhpa version)
// https://github.com/tzapu/WiFiManager

#include <ESP_DoubleResetDetector.h>
// A library for checking if the reset button has been pressed twice
// Can be used to enable config mode
// Can be installed from the library manager (Search for "ESP_DoubleResetDetector")
//https://github.com/khoih-prog/ESP_DoubleResetDetector

#include <ArduinoJson.h>
// ArduinoJson is used for parsing and creating the config file.
// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson

// -------------------------------------
// -------   Other Config   ------
// -------------------------------------

const int PIN_LED = BUILTIN_LED;

#define JSON_CONFIG_FILE "/sample_config.json"

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 5

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

// -----------------------------

// -----------------------------

DoubleResetDetector *drd;

//flag for saving data
bool shouldSaveConfig = false;

char testString[50] = "deafult value";
int testNumber = 1500;
bool testBool = true;
int day = 3; // 0 == Monday etc
char *daysOfWeek[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

void saveConfigFile()
{
  Serial.println(F("Saving config"));
  StaticJsonDocument<512> json;
  json["testString"] = testString;
  json["testNumber"] = testNumber;
  json["testBool"] = testBool;
  json["day"] = day;

  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
  }

  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0)
  {
    Serial.println(F("Failed to write to file"));
  }
  configFile.close();
}

bool loadConfigFile()
{
  //clean FS, for testing
  // SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  // May need to make it begin(true) first time you are using SPIFFS
  // NOTE: This might not be a good way to do this! begin(true) reformats the spiffs
  // it will only get called if it fails to mount, which probably means it needs to be
  // formatted, but maybe dont use this if you have something important saved on spiffs
  // that can't be replaced.
  if (SPIFFS.begin(false) || SPIFFS.begin(true))
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists(JSON_CONFIG_FILE))
    {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      if (configFile)
      {
        Serial.println("opened config file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error)
        {
          Serial.println("\nparsed json");

          strcpy(testString, json["testString"]);
          testNumber = json["testNumber"].as<int>();
          testBool = json["testBool"].as<bool>();
          day = json["day"].as<int>();

          return true;
        }
        else
        {
          Serial.println("failed to load json config");
        }
      }
    }
  }
  else
  {
    Serial.println("failed to mount FS");
  }
  //end read
  return false;
}

//callback notifying us of the need to save config
void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// This gets called when the config mode is launced, might
// be useful to update a display with this info.
void configModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("Entered Conf Mode");

  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());

  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}

// Custom HTML WiFiManagerParameter don't support getValue directly
String getCustomParamValue(WiFiManager *myWiFiManager, String name)
{
  String value;

  int numArgs = myWiFiManager->server->args();
  for (int i = 0; i < numArgs; i++) {
    Serial.println(myWiFiManager->server->arg(i));
  }
  if (myWiFiManager->server->hasArg(name))
  {
    value = myWiFiManager->server->arg(name);
  }
  return value;
}

void setup()
{
  pinMode(PIN_LED, OUTPUT);

  bool forceConfig = false;

  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  if (drd->detectDoubleReset())
  {
    Serial.println(F("Forcing config mode as there was a Double reset detected"));
    digitalWrite(PIN_LED,!digitalRead(PIN_LED));
    delay(50);
    digitalWrite(PIN_LED,!digitalRead(PIN_LED));
    delay(50);
    digitalWrite(PIN_LED,!digitalRead(PIN_LED));
    delay(50);
    digitalWrite(PIN_LED,!digitalRead(PIN_LED));
    delay(50);
    digitalWrite(PIN_LED,!digitalRead(PIN_LED));
    delay(50);
    digitalWrite(PIN_LED,!digitalRead(PIN_LED));
    delay(50);
    digitalWrite(PIN_LED,!digitalRead(PIN_LED));
    delay(50);
    digitalWrite(PIN_LED,!digitalRead(PIN_LED));
    delay(50);
    digitalWrite(PIN_LED,!digitalRead(PIN_LED));
    forceConfig = true;
  }

  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup)
  {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }

  //WiFi.disconnect();
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  Serial.begin(115200);
  delay(10);

  // wm.resetSettings(); // wipe settings

  WiFiManager wm;

  //wm.resetSettings(); // wipe settings
  //set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  //--- additional Configs params ---

  // Text box (String)
  WiFiManagerParameter custom_text_box("key_text", "Enter your string here", testString, 50); // 50 == max length

  // Text box (Number)
  char convertedValue[6];
  sprintf(convertedValue, "%d", testNumber); // Need to convert to string to display a default value.

  WiFiManagerParameter custom_text_box_num("key_num", "Enter your number here", convertedValue, 7); // 7 == max length

  //Check Box
  char *customHtml;
  if (testBool)
  {
    customHtml = "type=\"checkbox\" checked";
  }
  else
  {
    customHtml = "type=\"checkbox\"";
  }
  
  WiFiManagerParameter custom_checkbox("key_bool", "Checkbox", "T", 2, customHtml); // The "t" isn't really important, but if the
  // box is checked the value for this field will
  // be "t", so we can check for that.

  //Select menu (custom HTML)
  // The custom html options do not get handled the same way as other standard ones

  const char *day_select_str = R"(
  <br/><label for='day'>Custom Field Label</label>
  <select name="dayOfWeek" id="day" onchange="document.getElementById('key_custom').value = this.value">
    <option value="0">Monday</option>
    <option value="1">Tuesday</option>
    <option value="2">Wednesday</option>
    <option value="3">Thursday</option>
    <option value="4">Friday</option>
    <option value="5">Saturday</option>
    <option value="6">Sunday</option>
  </select>
  <script>
    document.getElementById('day').value = "%d";
    document.querySelector("[for='key_custom']").hidden = true;
    document.getElementById('key_custom').hidden = true;
  </script>
  )";

  char bufferStr[700];
  // The sprintf is so we can input the value of the current selected day
  // If you dont need to do that, then just pass the const char* straight in.
  sprintf(bufferStr, day_select_str, day);

  Serial.print(bufferStr);

  WiFiManagerParameter custom_field(bufferStr);


  // Hidden field to get the data
  sprintf(convertedValue, "%d", day); // Need to convert to string to display a default value.

  WiFiManagerParameter custom_hidden("key_custom", "Will be hidden", convertedValue, 2);

  //add all your parameters here
  wm.addParameter(&custom_text_box);
  wm.addParameter(&custom_text_box_num);
  wm.addParameter(&custom_checkbox);
  wm.addParameter(&custom_hidden); // Needs to be added before the javascript that hides it
  wm.addParameter(&custom_field);

  Serial.println("hello");

  digitalWrite(PIN_LED, LOW);
  if (forceConfig)
  {
    if (!wm.startConfigPortal("WifiTetris", "clock123"))
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
    if (!wm.autoConnect("WifiTetris", "clock123"))
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // if we still have not connected restart and try all over again
      ESP.restart();
      delay(5000);
    }
  }

  // If we get here, we are connected to the WiFi
  digitalWrite(PIN_LED, HIGH);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //save the custom parameters to FS
  if (shouldSaveConfig)
  {
    // Lets deal with the user config values

    // Copy the string value
    strncpy(testString, custom_text_box.getValue(), sizeof(testString));
    Serial.print("testString: ");
    Serial.println(testString);

    //Convert the number value
    testNumber = atoi(custom_text_box_num.getValue());
    Serial.print("testNumber: ");
    Serial.println(testNumber);

    //Handle the bool value
    testBool = (strncmp(custom_checkbox.getValue(), "T", 1) == 0);
    Serial.print("testBool: ");
    if (testBool)
    {
      Serial.println("true");
    }
    else
    {
      Serial.println("false");
    }

    //The custom one
    day = atoi(custom_hidden.getValue());
    Serial.print("Selected Day: ");
    Serial.println(daysOfWeek[day]);

    delay(50);
    digitalWrite(PIN_LED,HIGH);
    saveConfigFile();
  }
}

void loop()
{
  drd->loop();
}