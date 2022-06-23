/*********
AUTOPRN V2 FOR ESP32 
*********/
/* -----------------------------Pre-Processor Directives---------------------------------------*/
//WIFI, Arduino JSON & HTML Client HEADER FILE
#include <string.h>
#include<stdlib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
//OTA UPDATE HEADER FILE
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
//MEMORY CARD & SQLite  PRE-PROCESSOR DIRECTIVE
#include <sqlite3.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

/* -----------------------------NTP Library---------------------------------------*/
#include "time.h"
/* -----------------------------RTC Library---------------------------------------*/
#include <DS323x.h>
DS323x rtc;
time_t start_time, end_time;
double diff_time;
//SPI PORTS FOR MEMORY CARD READER
#define SD_CS         13
#define SPI_MOSI      15
#define SPI_MISO      2
#define SPI_SCK       14
#define MAC_ID "Device_12345"
/* -----------------------------File Operations---------------------------------------*/
/* -----------------------------Logic Variables---------------------------------------*/
//Global Variable for IO Logic
unsigned long time_countdown = 0;
long idle_time=0;
long cycle_time=0;
int prod_start_flag,prod_stop_flag,data_update_flag,up_ins_flag;
long counter,ok,ng,by_pass;
String c_date,c_time,c_timestamp;
/* -----------------------------IO Variables--------------------------------------*/
int ip1,ip2,ip3,ip4;
int op1,op2;
/* -----------------------------WiFi Credentials---------------------------------------*/
//WIFI CREDENTIAL & IP Configuartions
//const char* ssid = "BK_SHOPFLOOR";
const char* ssid = "BK ADMIN";
//const char* password = "koki@best#";
const char* password = "Best@#123";
//const char* ssid = "WIN";
//const char* password = "00001111";
// Set your Static IP address
IPAddress local_IP(192, 168, 2, 6);
// Set your Gateway IP address
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 3, 7);   //optional
//IPAddress secondaryDNS(8, 8, 4, 4);  //optional

/* -----------------------------Home WiFi Credentials---------------------------------------
//WIFI CREDENTIAL & IP Configuartions
const char* ssid = "Rakesh 5G";
const char* password = "Rakesh@1";
//const char* ssid = "WIN";
//const char* password = "00001111";
// Set your Static IP address
IPAddress local_IP(192, 168, 1, 150);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 1);   //optional
//IPAddress secondaryDNS(8, 8, 4, 4);  //optional
/* -----------------------------JSON Variable---------------------------------------*/
// JSON 
String jsonBuffer;
int httpResponseCode;
/* -----------------------------Timer Delays for API---------------------------------------*/
// THE DEFAULT TIMER IS SET TO 10 SECONDS FOR TESTING PURPOSES
// For a final application, check the API call limits per hour/minute to avoid getting blocked/banned
unsigned long lastTime = 0;
// Set timer to 10 seconds (10000)
unsigned long timerDelay = 10000;

/* -----------------------------Dual Core Task Handler Variabale---------------------------------------*/
TaskHandle_t Task1;
TaskHandle_t Task2;

/* -----------------------------START Web Server for ELEGANT OTA ---------------------------------------*/
//Async Server Object
AsyncWebServer server(80);
/* -----------------------------CREATE INSTANCE OF DS3231 RTC Module & UPDATE FLAGS---------------------------------------*/

/* -----------------------------Set NTP Server Paramters & Define Local Time Function---------------------------------------*/
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec =19800;
const int   daylightOffset_sec = 0;
struct tm timeinfo;
int rtc_update_flag;
/* -----------------------------START SQLITE  ---------------------------------------*/
/*SQLITE DATABASE OPERATION FUNCTION DEFINITION CODE STARTS FROM HERE*/
const char* data = "Callback function called";
sqlite3_stmt *res;
const char *tail;
sqlite3 *db1;
char *zErrMsg = 0;
int rc;
int db_request_num;

static int callback(void *data, int argc, char **argv, char **azColName) {
   int i;
   Serial.printf("%s: ", (const char*)data);
   for (i = 0; i<argc; i++){
       Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   Serial.printf("\n");
   return 0;
}

static int callback1(void *data, int argc, char **argv, char **azColName){
  int i,count;
   for (i = 0; i<argc; i++){
       Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
       if(strcmp(azColName[i],"OK")==0)
       {
        ok=strtol(argv[i],NULL,10);
       }
        else if(strcmp(azColName[i],"NG")==0)
       {
        ng=strtol(argv[i],NULL,10);
       }
        else if(strcmp(azColName[i],"BY_PASS")==0)
       {
        by_pass=strtol(argv[i],NULL,10);
       }
       else
       {
        Serial.println("Just for Fun with No reason!");
       }
   }
     return 0;
}

static int callback2(void *data, int argc, char **argv, char **azColName){
  int i,count;
  long ok1,ng1,by_pass1,cycle_time1;
  String sync_date,sync_time,sync_timestamp;
    for (i = 0; i<argc; i++){
       Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
       if(strcmp(azColName[i],"date")==0)
       {
        sync_date=argv[i];
       }
       else if(strcmp(azColName[i],"time")==0)
       {
        sync_time=argv[i];
       }
       else if(strcmp(azColName[i],"OK")==0) 
       {
        ok1=strtol(argv[i],NULL,10);
       }
        else if(strcmp(azColName[i],"NG")==0)
       {
        ng1=strtol(argv[i],NULL,10);
       }
        else if(strcmp(azColName[i],"BY_PASS")==0)
       {
        by_pass1=strtol(argv[i],NULL,10);
       }
       else if(strcmp(azColName[i],"cycle_time")==0)
       {
        cycle_time1=strtol(argv[i],NULL,10);
       }
        else if(strcmp(azColName  [i],"time_stamp")==0)
       {
        sync_timestamp=argv[i];
       }
       else
       {
        Serial.println("No Needed Columns!");
       }
// Call function to update Sync;
edge_data_sync_req(ok1,ng1,by_pass1);
if(httpResponseCode==200)
{
 update_sync(sync_timestamp); 
}

   }
     return 0;
}

int openDb(const char *filename, sqlite3 **db) {
   int rc = sqlite3_open(filename, db);
   if (rc) {
       Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
       return rc;
   } else {
       Serial.printf("Opened database successfully\n");
   }
   return rc;
}


void update_data_from_db(){
   // Open database 1
   if (openDb("/sd/data.db", &db1))
       return;
   String sql_1 = "SELECT *FROM historian ORDER BY time_stamp DESC LIMIT 1";
   char * sql=&sql_1[0];
   Serial.println(sql);
   long start = micros();
   int rc = sqlite3_exec(db1, sql, callback1, (void*)data, &zErrMsg);
   if (rc != SQLITE_OK) {
       Serial.printf("SQL error: %s\n", zErrMsg);
       sqlite3_free(zErrMsg);
   } else {
       Serial.printf("Operation done successfully\n");
   }
   if (rc != SQLITE_OK) {
     Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(db1));
     sqlite3_close(db1);  
     return;
   }  
   sqlite3_close(db1);
   Serial.print(F("Time taken:"));
   Serial.println(micros()-start);
}

void sync_edge_server()
{
  String sql_1;
  int rc;
  char * sql;
  int start;
   // SYNC DATA
   if (openDb("/sd/data.db", &db1))
       return;
//SYNC HISTORIAN
   sql_1 = "SELECT *FROM historian WHERE sync=0";
   sql=&sql_1[0];
   Serial.println(sql);
   start = micros();
   rc = sqlite3_exec(db1, sql, callback2, (void*)data, &zErrMsg);
   if (rc != SQLITE_OK) {
       Serial.printf("SQL error: %s\n", zErrMsg);
       sqlite3_free(zErrMsg);
   } else {
       Serial.printf("Operation done successfully\n");
   }

   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
   Serial.print(F("Time taken:"));
   Serial.println(micros()-start);
   sqlite3_close(db1); 
}
void update_sync(String db_timestamp)
{
   String sql_1;
   char * sql;
   long start;
   int rc;
   //sql_1 = "UPDATE historian SET sync=1 WHERE time_stamp='"+db_timestamp+"'";
   sql_1 = "DELETE FROM historian WHERE time_stamp='"+db_timestamp+"'";
   sql=&sql_1[0];
   Serial.println(sql);
   start = micros();
   rc = sqlite3_exec(db1, sql, callback, (void*)data, &zErrMsg);
   if (rc != SQLITE_OK) {
       Serial.printf("SQL error: %s\n", zErrMsg);
       sqlite3_free(zErrMsg);
   } else {
       Serial.printf("Operation done successfully\n");
   }

   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
   Serial.print(F("Time taken:"));
   Serial.println(micros()-start);
 //usync_code=0;
}

void insert_historian(long ok, long ng,long by_pass,int cycle_time,int idle_time, int sync)
{
   Serial.println("Insert Historian Operation");
   long ok1=ok;
   long ng1=ng;
   long by_pass1=by_pass;
   int cycle_time1=cycle_time;
   int idle_time1=idle_time;
   int sync1=sync;
   char * date1=&c_date[0];
   char * time1=&c_time[0];
   char * time_stamp=&c_timestamp[0];
   char *sql;  
   int rec_count;
   if(httpResponseCode==200){sync1=1;} 
   else{sync1=0;}
   // Open database 1
   if (openDb("/sd/data.db", &db1))
       return;  
 sql = "INSERT INTO historian VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
   rc = sqlite3_prepare_v2(db1, sql, strlen(sql), &res, &tail);
   if (rc != SQLITE_OK) {
     Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(db1));
     sqlite3_close(db1);  
     return;
   }
   sqlite3_bind_text(res, 1, date1, strlen(date1), SQLITE_STATIC);
   sqlite3_bind_text(res, 2, time1, strlen(time1), SQLITE_STATIC);
   sqlite3_bind_int(res, 3,ok1);
   sqlite3_bind_int(res, 4,ng1);
   sqlite3_bind_int(res, 5,by_pass1);
   sqlite3_bind_int(res, 6,cycle_time1);
   sqlite3_bind_text(res, 7, time_stamp, strlen(time_stamp), SQLITE_STATIC);
   sqlite3_bind_int(res, 8,idle_time1);  
   sqlite3_bind_int(res, 9,sync1);  
   if (sqlite3_step(res) != SQLITE_DONE) {
        Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(db1));
        sqlite3_close(db1);
        return;
      }
      sqlite3_clear_bindings(res);
      rc = sqlite3_reset(res);
      if (rc != SQLITE_OK) {
        sqlite3_close(db1);
        return;
      }
      sqlite3_finalize(res);
      sqlite3_close(db1);
}


void db_sync(long ok, long ng,long by_pass,int cycle_time, int sync){
   Serial.println("Call Insert Data + Historian Operation");
   get_date_time_timestamp();
   long ok1=ok;
   long ng1=ng;
   long by_pass1=by_pass;
   int cycle_time1=cycle_time;
   int idle_time;
   int sync1=sync;
   char * date1=&c_date[0];
   char * time1=&c_time[0];
   char * time_stamp=&c_timestamp[0];
   insert_historian(ok, ng,by_pass,diff_time,idle_time,sync1);   
   sync_edge_server();
}
/* -----------------------------SQLite Functions Ends Here---------------------------------------*/
/* -----------------------------FUCNTION TO GET DATA from NTP Server---------------------------------------*/
void printLocalTime()
{
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    rtc_update_flag=0;

  }
  else
  {
    rtc_update_flag=1;
    Serial.print("TIME NOW:");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  }
return;
}
//GET Date time timestamp
void get_date_time_timestamp()
{
c_date="20"+(String)rtc.year()+"-"+(String)rtc.month()+"-"+(String)rtc.day();
c_time=(String)rtc.hour()+":"+(String)rtc.minute()+":"+(String)rtc.second();
c_timestamp=c_date+" "+c_time;
Serial.println(c_date);
Serial.println(c_time);
Serial.println(c_timestamp);
delay(100);
}
/* -----------------------------HTTP Request---------------------------------------*/
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  // Send HTTP POST request
  httpResponseCode = http.GET();
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
  if(httpResponseCode!=200)
  {
  }
  http.end();
  return payload;
}
void edge_data_sync_req(long ok,long ng,long by_pass)
{
   time(&end_time);
   int sync1;
   long ok1,ng1,by_pass1;
   diff_time = difftime(end_time, start_time);
  if( data_update_flag==1){
    // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      String serverPath = "http://192.168.3.19/eol/module/autoprn/API/api/data.php?ok=" + (String)ok1 +"&ng=" + (String)ng1+ "&by_pass=" + (String)by_pass1+ "&mac_id=" + (String)MAC_ID;
      Serial.println(serverPath);
      jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.println(jsonBuffer);
    }
     else {
      Serial.println("WiFi Disconnected");
    }
     if(WiFi.status()== WL_CONNECTED){
      String serverPath2 = "http://192.168.3.19/eol/module/autoprn/API/api/time_data.php?cycle_time=" + (String)diff_time +"&idle_time=" + (String)idle_time+ "&mac_id=" + (String)MAC_ID;
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
  if(httpResponseCode==200){sync1=1;} 
   else{sync1=0;}
  db_sync(ok, ng,by_pass,diff_time,sync1);
   }

 void machine_operation()
 {
  if(WiFi.status()== WL_CONNECTED){
      String serverPath = "http://192.168.3.19/eol/module/autoprn/API/api/machine.php?mac_id=" + (String)MAC_ID;
      Serial.println(serverPath);
      jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.println(jsonBuffer);
    }
     else {
      Serial.println("WiFi Disconnected");
    }
if(strcmp(&jsonBuffer[0],"1111")==0)
{
  digitalWrite(0,LOW);
  digitalWrite(4,LOW);
  Serial.println("MACHINE OFF COMMAND!");
}
else if(strcmp(&jsonBuffer[0],"0000")==0)
{
  digitalWrite(0,HIGH);
  digitalWrite(4,HIGH);
  Serial.println("MACHINE ON COMMAND!");
}
else
{
  Serial.println("NO COMMANDS YET!");
}
 }
/* -----------------------------SET UP FUCNTION FOR ESP32---------------------------------------*/
void setup() {
  Serial.begin(115200); //START SERIAL PORT WITH BAUD RATE 115200 bps
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);  //START SPI for Memory Card Reader
  delay(2000); // Request the time correction on the Serial
  Wire.begin();  //START I2C FOR TRC Module
  delay(2000); // Request the time correction on the Serial
  if(!SD.begin(SD_CS, SPI)){ //SD Card open and Mounting Check
        Serial.println("Card Mount Failed");
        //return;
    }
  uint8_t cardType = SD.cardType(); //Get SD card type 
  if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        //return;
    }
 delay(2000);
/* -----------------------------IO Declaration---------------------------------------*/
  pinMode(5, INPUT);
  pinMode(18, INPUT);
  pinMode(19, INPUT);
  pinMode(17, INPUT);
  pinMode(0, OUTPUT);//For Relay 1
  pinMode(4, OUTPUT);//For Relay 2
/* -----------------------------Read from Database and Update Data---------------------------------------*/
   sqlite3_initialize();
   

/* -----------------------------Connect to WiFi---------------------------------------*/ 
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
/* -----------------------------SET & UPDATE RTC Date Time from NTP Server---------------------------------------*/
printLocalTime();   
rtc.attach(Wire);
if(rtc_update_flag==1)//UPDATE RTC IF NTP Server Returns OK Flag
{
 //rtc.now(DateTime(2022, 5, 25, 12, 0, 0));
int year1=timeinfo.tm_year+1900;
int month1=timeinfo.tm_mon+1;
rtc.now(DateTime(year1, month1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec)); 
rtc_update_flag=0;
}
get_date_time_timestamp();
update_data_from_db();
//Serial.println(diff_time);
delay(10000);
/* -----------------------------OTA SERVER START---------------------------------------*/  
//OTA SERVER Init
 server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "AutoPRN Firmware Version 2.1");
  });
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
/* -----------------------------DEFINE TASK FOR CORE 1---------------------------------------*/
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
/* -----------------------------DEFINE TASK FOR CORE 2---------------------------------------*/
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
/* -----------------------------Task 1 Definition---------------------------------------*/
//Task1code: blinks an LED every 1000 ms
void Task1code( void * pvParameters ){
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  for(;;){
//HERE WRITE CODE
    Serial.println("OK,NG,BY_PASS");
    Serial.println(ok);
    Serial.println(ng);
    Serial.println(by_pass);
    while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    counter +=1;
    if(counter>=100)
    {
      counter=0;
      break;
    }
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
    machine_operation();
    delay(10000);
  } 
}
/* -----------------------------Task 2 Definition---------------------------------------*/
//Task2code: blinks an LED every 700 ms
void Task2code( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    //HERE WRITE CODE
   // Send an HTTP GET request
   edge_data_sync_req(ok,ng,by_pass);
  delay(10000);
}
}

/* -----------------------------Main Loop---------------------------------------*/
void loop() {
      time(&start_time);
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
      //Serial.print("Machine is in IDLE Condtion!");
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
//Serial.printf("Current counter value: %ld\n", counter);
 delay(100);
}
