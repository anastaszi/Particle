/////////////////////////////////////////////
#include "application.h"

// This #include statement was automatically added by the Particle IDE.
#include <LiquidCrystal.h> // Screen control
#include <ArduinoJson.h> // JSON parse
#include <HttpClient.h> // Http request to get json from Purple Air

// Global variables for Http request and Screen control
HttpClient http;
http_header_t headers[] = {
    //  { "Content-Type", "application/json" },
    //  { "Accept" , "application/json" },
    { "Accept" , "*/*"},
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};
http_request_t request;
http_response_t response;
int previous_aqi;

struct AQI_result {
  String quality;
  int aqi;
  int temp;
} AQI_result;


byte arrow_up[] = {
  0x04,
  0x0E,
  0x15,
  0x04,
  0x04,
  0x04,
  0x04,
  0x04
};

byte arrow_down[] = {
  0x04,
  0x04,
  0x04,
  0x04,
  0x04,
  0x15,
  0x0E,
  0x04
};

byte same[] = {
  0x00,
  0x00,
  0x00,
  0x00,
  0x0E,
  0x00,
  0x00,
  0x00
};


int converter(double value, double lower_bound, double upper_bound, int floor, int ceiling) {
  int result = (int)((value - lower_bound) * (ceiling - floor)/(upper_bound - lower_bound)) + floor;
  return result;
}

struct AQI_result pm_to_aqi(const char *str) {
  int i = 0, array_len = 8;
  struct AQI_result result;
  double pm_value = atof(str);

  // array of pm and aqi boundaries
  double pm_b[8][2]  = {{0, 12.0}, {12.1, 35.4}, {35.5, 55.4}, {55.5, 150.4}, {150.5, 250.4
}, {250.5, 350.4}, {350.5, 500.4}, {500.5, 99999.9}};
  int aqi_b[8][2] = {{0, 50}, {51, 100}, {101, 150}, {151, 200}, {201, 300}, {301, 400}, {401, 500}, {501, 999}};
  String quality[8] = {"GOOD", "MODERATE", "UNHEALTHY FOR SENSITIVE", "UNHEALTHY", "VERY UNHEALTHY", "HAZARDOUS", "HAZARDOUS", "HAZARDOUS"};
  for (i = 0; i < array_len; i++) {
    if (pm_value >= pm_b[i][0] && pm_value <= pm_b[i][1]) {
      result.quality = quality[i];
      result.aqi = converter(pm_value, pm_b[i][0], pm_b[i][1], aqi_b[i][0], aqi_b[i][1]);
      break;
    }
  }

  return result;
}

// Make sure to update these to match how you've wired your pins.
// pinout on LCD [RS, EN, D4, D5, D6, D7];
// pin nums LCD  [ 4,  6, 11, 12, 13, 14];
// Shield Shield [RS, EN, D4, D5, D6, D7];
// Spark Core    [D3, D5, D2, D4, D7, D8];
LiquidCrystal lcd(D0, D1, D2, D3, D4, D5);

void setup() {
    previous_aqi = 0;
}

void loop() {
    // Request path and body can be set at runtime or at setup.
    request.hostname = "www.purpleair.com";
    request.port = 80;
    request.path = "/json?show=36991";
    // Get request
    http.get(request, response, headers);

    // Convert String to Char *
    size_t size = response.body.length();
    char result[size + 1];
    strcpy(result, response.body.c_str());

    // Parse response json and get value of PM2_5
    size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(28) + JSON_OBJECT_SIZE(39);
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, result);
    JsonObject results_0 = doc["results"][0];
    const char* results_0_PM2_5Value = results_0["PM2_5Value"];
    struct AQI_result aqi_res = pm_to_aqi(results_0_PM2_5Value);

    Particle.publish("PM_AQI",results_0_PM2_5Value,aqi_res.aqi,PRIVATE);








    // second request
    request.hostname = "api.openweathermap.org";
    request.port = 80;
    request.path =  "/data/2.5/weather?id=5368335&units=imperial&APPID=ADD_YOUR_ID";
    // Get request
    http.get(request, response, headers);

    capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(13) + 420;

    DynamicJsonDocument doc2(capacity);

    size_t size2 = response.body.length();
    char result2[size2 + 1];
    strcpy(result2, response.body.c_str());

    deserializeJson(doc2, result2);

     //result.temp = (temp_f − 32)*5.0/9;
    float main_temp =doc2["main"]["temp"];
    aqi_res.temp = (main_temp - 32) * 5.0 / 9;
    lcd.clear();
    // set up the LCD's number of columns and rows:
    lcd.begin(16,2);
    // Print a first string to the LCD.
    lcd.print("Temp: ");
    lcd.print(aqi_res.temp);
    lcd.print((char)223);
    lcd.print("C");

    // set the cursor to column 0, line 1
    // (note: line 1 is the second row, since counting begins with 0):

    // print the number of seconds since reset:
    if (previous_aqi < aqi_res.aqi)
        lcd.createChar(0, arrow_up);
    else if (previous_aqi > aqi_res.aqi)
        lcd.createChar(0, arrow_down);
    else
        lcd.createChar(0, same);
    lcd.setCursor(0, 1);
    lcd.print("Aqi: ");
    lcd.print(aqi_res.aqi);
    lcd.write(0);
    lcd.print(" ");
    lcd.print(aqi_res.quality);
    previous_aqi = aqi_res.aqi;
    delay(5000);
    delay(150000);
}
