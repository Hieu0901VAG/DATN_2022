// #define ERA_DEBUG

#define DEFAULT_MQTT_HOST "mqtt1.eoh.io"

#define ERA_AUTH_TOKEN "5dcfd94d-abd8-49e5-a014-f49974fa0abf"

#include <MPU6050_tockn.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <SPI.h>
#include "RTClib.h"
#include "Common.h"
#include <Arduino.h>
#include <ERa.hpp>
#include <ERa/ERaTimer.hpp>
#include <SimpleKalmanFilter.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include <SD.h>


const int chipSelect = 5;
File myFile;


AsyncWebServer server2(8080);


MPU6050 mpu6050(Wire);
WiFiClass wifiServer;
RTC_DS3231 rtc;
Common:: Common() {}
Common common;
SimpleKalmanFilter kalmanX(0.001, 0.01, 0.1); // Tham số của Kalman Filter
SimpleKalmanFilter kalmanY(0.001, 0.01, 0.1);

bool isWifiConnected =false;

const char* fileName="/demo6.txt";
bool setuptiep = true;
const char* headerLine = "DateTime,KalmaPitch,Pitch,KalmaRoll,Roll";
String ssidI ;
String  passI ;
unsigned long startTime; // Biến lưu thời gian bắt đầu chương trình
const unsigned long interval = 3600000;
const char* ssidP = "ESP32-Access-Point";
const char* passwordP = "123456789";
const char* htmlPage = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Welcome to the Gyro App!</title>
  <style>
    body {
      font-family: Arial, sans-serif;
    }
    input {
      padding: 8px;
      font-size: 14px;
    }
    button {
      padding: 10px 15px;
      font-size: 16px;
    }
  </style>
</head>
<body>
  <h1>Welcome to the Gyro App!</h1>
  <form action="/saveWifi" method="POST">
    <label for="ssid">WiFi SSID:</label><br>
    <input type="text" id="ssid" name="ssid" required><br><br>
    <label for="password">WiFi Password:</label><br>
    <input type="password" id="password" name="password" required><br><br>
    <button type="submit">Submit</button>
  </form>
</body>
</html>
)";




ERaTimer timer;

void timerEvent() {
    ERa.virtualWrite(V5, common.getPitch()); 
    ERa.virtualWrite(V6, common.getRoll()); 
    
}

ERA_WRITE(V10){
  int value = param.getInt();
  if(value = 1){
    Serial.println("REtrieve data");
    ERa.virtualWrite(V2, 7); 
    ERa.virtualWrite(V2, 8); 
    int i =0;
    int maxSize = 250;
    int arrtemp[maxSize];
    bool isRun = true;
    while(isRun){
    mpu6050.update();
    timerEvent();
    float kalmanRoll = kalmanX.updateEstimate(common.getRoll());
    int numberAppend = (int)kalmanRoll;
    arrtemp[i] = numberAppend;
    i++;
    common.saveFile();
    if(i>200){
        isRun = false;
      }
    delay(300);
    }
      Serial.println("Array: ");
    for(int j=0; j<i;j++){
      Serial.print(arrtemp[j]);
      Serial.print(",");
    }
    Serial.println("Peak:");
    int peaks = common.findPeak(arrtemp, i-1);
    Serial.println(peaks); 
    ERa.virtualWrite(V1, peaks); 
    ERa.virtualWrite(V2, 2); 
    ERa.virtualWrite(V2, 3); 
    ERa.virtualWrite(V0, peaks);
    Serial.println("End REtrieve data");
  }
}

void setup() {
  Serial.begin(9600);
  wifiServer.softAP(ssidP, passwordP);
  IPAddress IP = wifiServer.softAPIP();
  Serial.print("AP IP address:");
  Serial.println(IP);

  
  // Xử lý request GET cho trang cấu hình WiFi
  server2.on("/home", HTTP_GET, [](AsyncWebServerRequest *request){
    
    request->send(200, "text/html", htmlPage);
  });

  // Xử lý request POST khi người dùng cấu hình WiFi
  // Xử lý request POST khi người dùng cấu hình WiFi
   server2.on("/saveWifi", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      String ssid = request->getParam("ssid", true)->value();
      String password = request->getParam("password", true)->value();
      Serial.println("ssid");
      Serial.println(ssid);
      Serial.println("password");
      Serial.println(password);
      // Process the received SSID and password, e.g., save to EEPROM or elsewhere
      // ...
      passI = password;
      ssidI = ssid;
      setuptiep = false;
      String response = "Kết nối Thành Công ";
      request->send(200, "text/plain", response);
    } else {
      request->send(400, "text/plain", "Kết nối thất bại");
    }
  });
  
  // Bắt đầu server
  server2.begin();
  while(setuptiep){
    Serial.println("...");
  }
  ERa.begin(ssidI.c_str(), passI.c_str());
    isWifiConnected = true;

  common.setUpModuleSD();
  common.setUpMPU();

  timer.setInterval(300,timerEvent);
  startTime = millis();
}

void loop() {
  mpu6050.update();
  if(isWifiConnected){
    ERa.run();
    timer.run();
  }
  delay(300);

}

void Common:: setUpMPU() {
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void Common:: setUpModuleSD(){
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("1. is a card inserted?");
    Serial.println("2. is your wiring correct?");
    Serial.println("3. did you change the chipSelect pin to match your shield or module?");
    Serial.println("Note: press reset button on the board and reopen this serial monitor after fixing your issue!");
    while (1);
  }
  Serial.println("initialization done.");

  if(!SD.exists(fileName)){
  Serial.println("file not found create header");
  myFile = SD.open(fileName,FILE_WRITE);
  if(myFile){
    myFile.println(headerLine);
  }
  myFile.close();
  }
}

float Common:: getPitch() {
  float accX = mpu6050.getAccX();
  float accY = mpu6050.getAccY();
  float accZ = mpu6050.getAccZ();

  return atan2(-accX, sqrt(accY * accY + accZ * accZ)) * 180.0 / PI;
}

float Common:: getRoll() {
  float accX = mpu6050.getAccX();
  float accY = mpu6050.getAccY();
  float accZ = mpu6050.getAccZ();
  return atan2(accY, sqrt(accX * accX + accZ * accZ)) * 180.0 / PI;
}

String Common:: getDateTimeString() {
  DateTime now = rtc.now();
  return now.timestamp();
}

void Common:: getDataFromPreviousMinute() {
  DateTime now = rtc.now();

  // Tính thời gian 1 giờ trước
  DateTime previous = common.getOneHourseAgo(now);

  // Mở file dữ liệu .txt
  File dataFile = SD.open(fileName, FILE_READ);
  if (dataFile) {
    int i =0;
    int maxSize = 500;
    int arrtemp[maxSize];
    int totalPeak = 0;
    while (dataFile.available()) {
      // Đọc mỗi dòng trong file
      String line = dataFile.readStringUntil('\n');
      try {
      int datatemp = common.processLine(line,previous);
      if(datatemp>=-1){
        if(datatemp<0){
          datatemp=0;
        }
        arrtemp[i]= datatemp;
        i++; 
      }
      if(i>=499){
          int peak = common.findPeak(arrtemp,i);
          totalPeak+=peak;
          i=0;
        }
      }
      catch(const char* exception){
          Serial.println("exceptions");
      }
    }
    dataFile.close();
    if(i>0) {
      int peak = common.findPeak(arrtemp, i);
      totalPeak+=peak;
    }
    ERa.virtualWrite(V0, totalPeak); 
  } else {
    Serial.println("Khong the mo file!");
  }
}

DateTime Common:: getOneMinuteAgo(DateTime currentTime) {
  int newSecond = currentTime.second() - 60;
  int newMinute = currentTime.minute();
  int newHour = currentTime.hour();

  if (newSecond < 0) {
    // Trường hợp đặc biệt: nếu giây hiện tại lớn hơn hoặc bằng 60, thực hiện điều chỉnh giờ và phút
    newSecond += 60;
    newMinute--;
    if (newMinute < 0) {
      // Giảm phút nếu như phút hiện tại là 0
      newMinute = 59;
      newHour--;
      if (newHour < 0) {
        // Giảm giờ nếu như giờ hiện tại là 0
        newHour = 23;
      }
    }
  }

  return DateTime(currentTime.year(), currentTime.month(), currentTime.day(), newHour, newMinute, newSecond);
}

DateTime Common:: getOneHourseAgo(DateTime currentTime) {
  return currentTime - TimeSpan(0, 1, 0, 0);
}

int Common::processLine(String line, DateTime previous) {
    String filterDate = previous.timestamp();
    String lineDate = line.substring(0,13);
    String chosenDate = filterDate.substring(0,13);
    if(lineDate == chosenDate){
      int dataPush = line.substring(20,25).toInt();
      return dataPush;
    }
    return -5;
}

void Common::appenData(const char* fileName,const char* data){
  myFile = SD.open(fileName,FILE_APPEND);
  if(myFile){
    myFile.println(data);
  }
  myFile.close();
}

void Common::saveFile() {
  float kalmanRoll = kalmanX.updateEstimate(common.getRoll());
  float kalmanPitch = kalmanY.updateEstimate(common.getPitch());
  String dataAppend = common.getDateTimeString()+","+kalmanPitch+","+common.getPitch()+","+kalmanRoll+","+common.getRoll();
  common.appenData(fileName,dataAppend.c_str());
}

String Common::getLine(const char* fileName){
  myFile = SD.open(fileName,FILE_READ);
  if(myFile){
    String temp ="";
    while (myFile.available()){
      temp+= myFile.readStringUntil('/n');
    }
    myFile.close();
    return temp;
  }
}

bool Common::setUpWifi(){
  File fuu = SD.open("/Wifi.txt",FILE_READ);
  if(!fuu){
    return false;
  }
  bool isconnecd;
  while (fuu.available()){
    String line = fuu.readStringUntil('/n');
    int commaIndex = line.indexOf(',');
    Serial.println("wifi");
    Serial.println(line);
  // Tách chuỗi thành hai mảng sau khi gặp dấu ','
    String ssid = line.substring(0, commaIndex);
    String password = line.substring(commaIndex + 2);
    WiFi.begin(ssid, password);
    int i =0;
    while (WiFi.status() != WL_CONNECTED) {
    i++;
    if(i==5){
      isconnecd=false;
      break;
    }
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  if(WiFi.status()== WL_CONNECTED){
    fuu.close();
    return true;
  }

  }
  fuu.close();

  return false;
  
}

int Common:: findPeak(int arr[], int size) {
  if (size <= 2) {
    // Mảng có 2 phần tử trở xuống không thể có đỉnh
    return 0;
  }

  int peakCount = 0;
  int k =0;

  // Xử lý phần tử đầu tiên
  if (arr[0] >= arr[1] && arr[0]>=30) {
    peakCount++;
  }

  // Duyệt qua các phần tử từ phần tử thứ 2 đến phần tử thứ (size - 2)
  for (int i = 2; i < size - 1; i++) {
    if (arr[i] >= arr[i - 1] && arr[i] >= arr[i + 1] 
    && arr[i]>=30
    &&arr[i] > arr[i-2] && arr[i] > arr[i+2]
    ) {
      if(k!=0&& i-k <=3){
        continue;
      }
      peakCount++; // Tìm thấy đỉnh tại vị trí i
       k=i;
    }
  }

  // Xử lý phần tử cuối cùng
  if (arr[size - 1] >= arr[size - 2] && arr[size -1 ]>=30) {
    peakCount++;
  }

  return peakCount;
}