#ifndef Common_h
#define Common_h



class Common {
public:
  Common();


  void setUpMPU();

  void setUpModuleSD();

  float getPitch();
  float getRoll();
  String getDateTimeString();
  void getDataFromPreviousMinute();
  DateTime getOneMinuteAgo(DateTime currentTime);
  DateTime getOneHourseAgo(DateTime currentTime);
  void smoothDataUsingMovingAverage(int *inputData, int *outputData, int dataSize, int windowSize);
  void getDataFromPreviousHour();
  int processLine(String line, DateTime previousHour);
  void appenData(const char *fileName,const char *data);
  void saveFile();
  String getLine(const char *fileName);
  bool setUpWifi();
  int findPeak(int arr[], int size);
  void medianFilter(int *data, int size, int *output);
  void timerEvent();
  void EraSetup();
  char* getHtml() {
     char* htmlPage = R"(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 WiFi Config</title>
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
  <h1>ESP32 WiFi Configuration</h1>
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
return htmlPage;
  }
};

#endif