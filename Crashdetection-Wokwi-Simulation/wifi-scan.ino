#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>
#include <vector>
#include <WiFi.h>
#include "time.h"
#include "FS.h"
#include "SPIFFS.h"
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 0;
Adafruit_MPU6050 mpu;
const float ACCEL_THRESH    = 2.8;
const float ACCEL_AXIS_SAT  = 1.8;
const float GYRO_THRESH     = 300.0;
const float GYRO_AXIS_SAT   = 220.0;
const float TEMP_ALARM      = 70.0;
struct GPSNMEA {
  String gprmc;
  String gpgga;
};
std::vector<GPSNMEA> gpsNmeaData = {
  {"$GPRMC,083000.00,A,1235.896,N,07735.676,E,0.0,0.0,010123,,,A*6C", "$GPGGA,083000.00,1235.896,N,07735.676,E,1,08,1.0,45.0,M,0.0,M,,*56"},
  {"$GPRMC,091500.00,A,2839.246,N,07706.150,E,0.0,0.0,010123,,,A*7B", "$GPGGA,091500.00,2839.246,N,07706.150,E,1,08,1.0,45.0,M,0.0,M,,*65"},
  {"$GPRMC,104500.00,A,1904.560,N,07252.620,E,0.0,0.0,010123,,,A*79", "$GPGGA,104500.00,1904.560,N,07252.620,E,1,08,1.0,45.0,M,0.0,M,,*6F"},
  {"$GPRMC,121500.00,A,1304.962,N,08016.242,E,0.0,0.0,010123,,,A*71", "$GPGGA,121500.00,1304.962,N,08016.242,E,1,08,1.0,45.0,M,0.0,M,,*72"},
  {"$GPRMC,143000.00,A,2234.356,N,08821.834,E,0.0,0.0,010123,,,A*72", "$GPGGA,143000.00,2234.356,N,08821.834,E,1,08,1.0,45.0,M,0.0,M,,*75"},
  {"$GPRMC,155500.00,A,1723.100,N,07829.202,E,0.0,0.0,010123,,,A*72", "$GPGGA,155500.00,1723.100,N,07829.202,E,1,08,1.0,45.0,M,0.0,M,,*63"},
  {"$GPRMC,164000.00,A,2301.350,N,07234.284,E,0.0,0.0,010123,,,A*7F", "$GPGGA,164000.00,2301.350,N,07234.284,E,1,08,1.0,45.0,M,0.0,M,,*70"}
};
String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "TimeError";
  }
  char buffer[30];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
          timeinfo.tm_year + 1900,
          timeinfo.tm_mon + 1,
          timeinfo.tm_mday,
          timeinfo.tm_hour,
          timeinfo.tm_min,
          timeinfo.tm_sec);
  return String(buffer);
}
// Pick unique random indices
std::vector<int> pickUniqueRandomIndices(int num, int maxVal) {
  std::vector<int> indices;
  while (indices.size() < num) {
    int idx = random(0, maxVal);
    if (std::find(indices.begin(), indices.end(), idx) == indices.end()) {
      indices.push_back(idx);
    }
  }
  return indices;
}
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  randomSeed(analogRead(0));
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Syncing time with NTP...");
  delay(2000);
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  Serial.println("MPU6050 initialized");
}
void logCrash(const std::vector<GPSNMEA>& loggedPoints) {
  String timestamp = getTimestamp();
  Serial.println("---- CRASH DETECTED ----");
  Serial.print("Timestamp: "); Serial.println(timestamp);
  int pick = random(0, loggedPoints.size());
  const GPSNMEA& singlePoint = loggedPoints[pick];
  String gprmc = singlePoint.gprmc;
  int firstComma = gprmc.indexOf(',');                       // after $GPRMC
  int secondComma = gprmc.indexOf(',', firstComma + 1);      // after time
  int thirdComma = gprmc.indexOf(',', secondComma + 1);      // after status
  int fourthComma = gprmc.indexOf(',', thirdComma + 1);      // after latitude
  int fifthComma  = gprmc.indexOf(',', fourthComma + 1);     // after N/S
  int sixthComma  = gprmc.indexOf(',', fifthComma + 1);      // after longitude
  int seventhComma = gprmc.indexOf(',', sixthComma + 1);     // after E/W
  String latStr = gprmc.substring(thirdComma + 1, fourthComma);
  String latDir = gprmc.substring(fourthComma + 1, fifthComma);
  String lonStr = gprmc.substring(fifthComma + 1, sixthComma);
  String lonDir = gprmc.substring(sixthComma + 1, seventhComma);
  float lat = latStr.toFloat();
  float lon = lonStr.toFloat();
  float latDegrees = int(lat / 100);
  float latMinutes = lat - (latDegrees * 100);
  float decLat = latDegrees + latMinutes / 60.0;
  if (latDir == "S") decLat = -decLat;
  float lonDegrees = int(lon / 100);
  float lonMinutes = lon - (lonDegrees * 100);
  float decLon = lonDegrees + lonMinutes / 60.0;
  if (lonDir == "W") decLon = -decLon;
  Serial.print("Latitude: "); Serial.println(decLat, 6);
  Serial.print("Longitude: "); Serial.println(decLon, 6);
  Serial.println("-------------------------");
}
void loop() {
  static bool crashed = false;
  if (crashed) return; 
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float ax = a.acceleration.x / 9.81;
  float ay = a.acceleration.y / 9.81;
  float az = a.acceleration.z / 9.81;
  float total_g = sqrt(ax * ax + ay * ay + az * az);
  
  String status;
  float gx = g.gyro.x;
  float gy = g.gyro.y;
  float gz = g.gyro.z;
  float total_gyro = sqrt(gx * gx + gy * gy + gz * gz);

  bool accel_axis_sat = (fabs(ax) >= ACCEL_AXIS_SAT || fabs(ay) >= ACCEL_AXIS_SAT || fabs(az) >= ACCEL_AXIS_SAT);
  bool gyro_axis_sat  = (fabs(gx) >= GYRO_AXIS_SAT || fabs(gy) >= GYRO_AXIS_SAT || fabs(gz) >= GYRO_AXIS_SAT);
  bool crash_now = false;
  
  if (accel_axis_sat && gyro_axis_sat) {
    status = "Accident Likely: Axis Saturation on Both!";
    crash_now = true;
  } else if (accel_axis_sat) {
    status = "Extreme Linear Accel (Axis Saturation)";
    crash_now = true;
  } else if (gyro_axis_sat) {
    status = "Extreme Rotation (Axis Saturation)";
    crash_now = true;
  } else if (total_g >= ACCEL_THRESH && total_gyro >= GYRO_THRESH) {
    status = "Accident Likely: High Accel & Rotation!";
    crash_now = true;
  } else if (total_g >= ACCEL_THRESH) {
    status = "Extreme Linear Accel";
    crash_now = true;
  } else if (total_gyro >= GYRO_THRESH) {
    status = "Extreme Rotation";
    crash_now = true;
  } else if (total_g < 1.0 && total_gyro < 50.0) {
    status = "Stationary/No Movement";
  } else if (total_g < 2.0 && total_gyro < 100.0) {
    status = "Normal Movement";
  } else {
    status = "Sudden Turn/Brake";
  }

  if (crash_now && !crashed) {
    std::vector<int> selected = pickUniqueRandomIndices(5 + random(0, 2), gpsNmeaData.size()); // Pick 5 or 6
    std::vector<GPSNMEA> chosen;
    for (int idx : selected) {
      chosen.push_back(gpsNmeaData[idx]);
    }
    logCrash(chosen);
    crashed = true;
    while (1);
  }
  Serial.print("Time: "); Serial.print(getTimestamp());
  Serial.print(" | Accel (g): X="); Serial.print(ax, 2);
  Serial.print(" Y="); Serial.print(ay, 2);
  Serial.print(" Z="); Serial.print(az, 2);
  Serial.print(" | Rotation (°/s): X="); Serial.print(gx, 2);
  Serial.print(" Y="); Serial.print(gy, 2);
  Serial.print(" Z="); Serial.print(gz, 2);
  Serial.print(" | Temp: "); Serial.print(temp.temperature, 1);
  Serial.print(" | Total Accel (g): "); Serial.print(total_g, 2);
  Serial.print(" | Total Gyro (°/s): "); Serial.print(total_gyro, 2);
  Serial.print(" | Condition: "); Serial.println(status);
  delay(1000);
}