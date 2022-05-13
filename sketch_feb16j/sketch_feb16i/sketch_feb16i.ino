/*********
AUTOPRN V2 FOR ESP32 
*********/
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
//OTA UPDATE HEADER FILE
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Preferences.h>
String MAC_ID="Device_1234";
unsigned long time_countdown = 0;
long idle_time=0;
long cycle_time=0;
int prod_start_flag,prod_stop_flag,data_update_flag;
long counter,ok,ng,by_pass;
//WIFI CREDENTIAL
const char* ssid = "BK_SHOPFLOOR";
const char* password = "koki@best#";
//const char* ssid = "WIN";
//const char* password = "00001111";
// Set your Static IP address
IPAddress local_IP(192, 168, 2, 5);
// Set your Gateway IP address
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 3, 7);   //optional
//IPAddress secondaryDNS(8, 8, 4, 4); //optional

String jsonBuffer;
// Your Domain name with URL path or IP address with path
//String openWeatherMapApiKey = "REPLACE_WITH_YOUR_OPEN_WEATHER_MAP_API_KEY";
// Example:
//String openWeatherMapApiKey = "8ed009df5b75e8b0c5db3a12af4f4be6";
// Replace with your country code and city
//String city = "Gurgaon";
//String countryCode = "IN";

// THE DEFAULT TIMER IS SET TO 10 SECONDS FOR TESTING PURPOSES
// For a final application, check the API call limits per hour/minute to avoid getting blocked/banned
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 10 seconds (10000)
unsigned long timerDelay = 10000;
TaskHandle_t Task1;
TaskHandle_t Task2;

// LED pins
const int led1 = 2;
const int led2 = 4;

//Async Server Object
AsyncWebServer server(80);
Preferences preferences;

int ip1,ip2,ip3,ip4;
int op1,op2;
void setup() {
  Serial.begin(115200); 
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(5, INPUT);
  pinMode(18, INPUT);
  pinMode(19, INPUT);
  pinMode(21, INPUT);
  Serial.begin(115200);




  
if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  
//OTA SERVER Init
 server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "AutoPRN Firmware Version 2.1");
  });
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");

//CORE 1 TASK 1
  Serial.println("Timer set to 10 seconds (timerDelay variable), it will take 10 seconds before publishing the first reading.");
  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

//CORE 2 TASK 2
  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 
}

//Task1code: blinks an LED every 1000 ms
void Task1code( void * pvParameters ){
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    digitalWrite(led1, HIGH);
    delay(1000);
    digitalWrite(led1, LOW);
    delay(1000);


  } 
}

//Task2code: blinks an LED every 700 ms
void Task2code( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
   // Send an HTTP GET request
  if( data_update_flag==1){
    // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      String serverPath = "http://192.168.3.19/eol/module/autoprn/API/api/data.php?ok=" + (String)ok +"&ng=" + (String)ng+ "&by_pass=" + (String)by_pass + "&mac_id=" + (String)MAC_ID;
      Serial.println(serverPath);
      jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.println(jsonBuffer);
    }
     else {
      Serial.println("WiFi Disconnected");
    }
     if(WiFi.status()== WL_CONNECTED){
      String serverPath2 = "http://192.168.3.19/eol/module/autoprn/API/api/time_data.php?cycle_time=" + (String)cycle_time +"&idle_time=" + (String)idle_time+ "&mac_id=" + (String)MAC_ID;
      Serial.println(serverPath2);
      jsonBuffer = httpGETRequest(serverPath2.c_str());
      Serial.println(jsonBuffer);
    }
    else {
      Serial.println("WiFi Disconnected");
    }
  data_update_flag=0;
  }
  else{
     Serial.println("Update Flag is Off");
  }
   }
}

void loop() {
      preferences.begin("my-app", false);
      //  /unsigned int counter = preferences.getUInt("counter", 0);
      counter = preferences.getLong("counter", 0);
      ok = preferences.getLong("ok", 0);
      ng = preferences.getLong("ng", 0);  
      by_pass=preferences.getLong("by_pass", 0);  
      cycle_time = preferences.getLong("cycle_time", 0);
      idle_time = preferences.getLong("idle_time", 0);
      ip1=digitalRead(5);
      ip2=digitalRead(18);
      ip3=digitalRead(19);
      ip4=digitalRead(21);
      if(ip1==HIGH)
      {
        if(prod_start_flag==0){
            idle_time=millis()-time_countdown;
            idle_time=idle_time/1000;
            time_countdown=0;
            time_countdown=millis();
            prod_start_flag=1;
        }
      
      }
      else
      {
        if(prod_start_flag==1)
        {
        prod_start_flag=0;
        prod_stop_flag=1; 
        }
      }
 if(prod_stop_flag==1)
 { 
  cycle_time=millis()-time_countdown;
  cycle_time=cycle_time/1000;
  prod_stop_flag=0;
  time_countdown=0;
  
 }

    if(ip2==1 && ip4==0)
    {
      ok++;
      data_update_flag=1;
      delay(2000);
      ip2=0;
    }
   else if(ip2==0 && ip4==1)
    {
      ng++;
      data_update_flag=1;
      delay(2000);
      ip4=0;
    }
    else if(ip4==1 && ip2==1)
    {
      by_pass++;
      delay(2000);
      data_update_flag=1;
    }
    else{
      Serial.print("Machine is in IDLE Condtion!");
    }
if(prod_stop_flag==0 && prod_start_flag==0 && time_countdown<=0){
//time_countdown=0;
time_countdown=millis();  
}
if(ip3==1)
{
ok=0;
ng=0;
by_pass=0;
preferences.putLong("by_pass", by_pass);
preferences.putLong("ok", ok);
preferences.putLong("ng", ng);
}
Serial.print("Start_Flag=");
Serial.println(prod_start_flag);
Serial.print("Stop_Flag=");
Serial.println(prod_stop_flag);
Serial.print("Time Count=");
Serial.println(time_countdown);
Serial.print("Cycle Time=");
Serial.println(cycle_time);
Serial.print("Idle Time=");
Serial.println(idle_time);
Serial.print("IP1=");
Serial.println(ip1);
Serial.print("IP2=");
Serial.println(ip2);
Serial.print("IP3=");
Serial.println(ip3);
Serial.print("IP4=");
Serial.println(ip4);
Serial.print("OK=");
Serial.println(ok);
Serial.print("NG=");
Serial.println(ng);
Serial.print("TOC=");
Serial.println(by_pass);
preferences.putLong("ok", ok);
preferences.putLong("ng", ng);
preferences.putLong("by_pass", by_pass);
preferences.putLong("cycle_time",cycle_time);
preferences.putLong("idle_time",idle_time);
preferences.end();
//Serial.printf("Current counter value: %ld\n", counter);
 delay(100);
}
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
