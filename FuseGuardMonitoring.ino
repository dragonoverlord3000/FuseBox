#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// The esp8266 will create a wifi you can connect to with the password
const char* ssid = "BaldInnovators - FuseGuard"; 
const char* password = "GoBaldForEfficiency"; 

const int therometerPin = D2;
const int LEDGreenPin = D7;
const int LEDYellowPin = D6;
const int LEDRedPin = D5;
const int LEDWhitePin = D1; // This represents all house electronics - i have a relay if we want it more accurate
const int buzzerPin = D8;    


//const int buzzerFrequencyWarning = 262; // C4
//const int buzzerFrequencyCutoff = 262; // C4

const int buzzerFrequencyWarning = 0; // mute
const int buzzerFrequencyCutoff = 0; // mute

const int buzzerDurationWarning = 500;
const int buzzerDurationCutoff = 200;

const float warningThreshold = 30.0;
const float cutoffThreshold = 32.5;

unsigned long buzzerTimestamp = 0;

// 0 - normal, 1 - warning, 2 - cutoff 
int state = 0;

float tempC;

// IP Address details: you connect by writing 192.168.1.1 in a web browser
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);

OneWire oneWire(therometerPin);

DallasTemperature sensors(&oneWire);


void setup() {
  Serial.begin(115200);
  
  // Start the DS18B20 sensor
  sensors.begin();

  // IO mode for LEDs
  pinMode(LEDGreenPin, OUTPUT);
  pinMode(LEDYellowPin, OUTPUT);
  pinMode(LEDRedPin, OUTPUT);
  pinMode(LEDWhitePin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(LEDWhitePin, 1);

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  server.on("/", handle_OnConnect);
  server.on("/warningYes", handle_warningYes);
  server.on("/warningNo", handle_warningNo);
  server.onNotFound(handle_NotFound);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  // Get temperature
  sensors.requestTemperatures(); 
  tempC = sensors.getTempCByIndex(0);
  Serial.print(tempC);
  Serial.println("ºC");

  if (tempC >= cutoffThreshold){ // Cutoof
    state = 2;
    digitalWrite(LEDGreenPin, 0);
    digitalWrite(LEDYellowPin, 1);
    digitalWrite(LEDRedPin, 1);
    if (buzzerTimestamp + buzzerDurationCutoff * 2 < millis() ){
      tone(buzzerPin, buzzerFrequencyCutoff, buzzerDurationCutoff);
      buzzerTimestamp = millis();
    }
    digitalWrite(LEDWhitePin, 0);
  }
  else if (tempC >= warningThreshold ){ // Warning
    state = 1;
    digitalWrite(LEDGreenPin, 0);
    digitalWrite(LEDYellowPin, 1);
    digitalWrite(LEDRedPin, 0);
    if (buzzerTimestamp + buzzerDurationWarning * 2 < millis() ){
      tone(buzzerPin, buzzerFrequencyWarning, buzzerDurationWarning);
      buzzerTimestamp = millis();
    }
  }
  else{ // Normal
    state = 0;
    digitalWrite(LEDGreenPin, 1);
    digitalWrite(LEDYellowPin, 0);
    digitalWrite(LEDRedPin, 0);
  }
}
void handle_OnConnect() {
  server.send(200, "text/html", SendHTML(state, -1, tempC));
  digitalWrite(LEDWhitePin, tempC < cutoffThreshold);
}
void handle_warningYes() {
  digitalWrite(LEDWhitePin, 0);
  server.send(200, "text/html", SendHTML(state, 1, tempC)); 
}
void handle_warningNo() {
  server.send(200, "text/html", SendHTML(state, 0, tempC)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(int state, int warningAnswer, float tempC){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +=" <meta http-equiv=\"refresh\" content=\"1\">\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 30px auto 30px;} h3 {color: #444444;margin: 20px;}\n";
  ptr +=".warning {border: 5px solid DarkOrange; background-color: Khaki; text-align: center;} .alert {border:5px solid DarkRed; background-color: FireBrick; text-align: center;}\n";
  ptr +=".button {display: inline;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 45px;text-decoration: none;font-size: 25px;margin: 20px auto 30px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-yes {background-color: Green;}\n";
  ptr +=".button-no {background-color: Red;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 30px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Connected to FuseGurard</h1>\n";

  ptr +="<h3>Your fuse-box is currently: " + String(tempC, 1) + "&deg;C </h3>\n";


  if (warningAnswer == 0){ // no
    ptr +="<div class=\"warning\"><h1>WARNING!</h1>\n<h3>The temperature in you fuse-box is abnormally high</h3><p>You have chosen not to trip the circuit breaker</p><p><a class=\"button\" href=\"/\">Reset</a></div>\n";
  }
  else if (warningAnswer == 1){ // yes
    ptr +="<div class=\"warning\"><h1>WARNING!</h1>\n<h3>The temperature in you fuse-box is abnormally high</h3><p>The circuit breaker has been flipped</p><p><a class=\"button\" href=\"/\">Reset</a></div>\n";
  }
  else {
    switch(state){
    case 0: // normal
      ptr +="<h3>The temperature of your fuse-box is normal</h3>\n";
      break;
    case 1: // Warning
      ptr +="<div class=\"warning\"><h1>WARNING!</h1>\n<h3>The temperature in you fusebox is abnormally high</h3><p>Do you want to trip the circuit breaker?</p><p><a class=\"button button-yes\" href=\"/warningYes\">Yes</a><a class=\"button button-no\" href=\"/warningNo\">No</a></p></div>\n";
      break;
    case 2: // Cutoff
      ptr +="<div class=\"alert\"><h1>ALERT!</h1>\n<h3>The temperature in your fuse-box has reached an extreme level</h3>\n<h3>Your circuit breaker has been tripped to prevent a fire</h3></div>\n";
      break;
    }
  }
  
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}