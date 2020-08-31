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

struct AQI_result {
  String quality;
  int aqi;
} AQI_result;

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
  // set up the LCD's number of columns and rows:
  lcd.begin(16,2);
  // Print a first string to the LCD.
  lcd.print("AQI in Los Altos");
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
    const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(29) + JSON_OBJECT_SIZE(40) + 1320;
    DynamicJsonDocument doc(capacity);

    const char* json = result;
    deserializeJson(doc, result);
    JsonObject results_0 = doc["results"][0];
    const char* results_0_PM2_5Value = results_0["PM2_5Value"];
    struct AQI_result aqi_res = pm_to_aqi(results_0_PM2_5Value);
    char buf[20];
    itoa(aqi_res.aqi, buf, 10);

    Particle.publish(results_0_PM2_5Value,buf,aqi_res.aqi,PRIVATE);

    // set the cursor to column 0, line 1
    // (note: line 1 is the second row, since counting begins with 0):
    lcd.setCursor(0, 1);
    // print the number of seconds since reset:
    lcd.print(aqi_res.aqi);
    lcd.print(" ");
    lcd.print(aqi_res.quality);
    delay(150000);
}
