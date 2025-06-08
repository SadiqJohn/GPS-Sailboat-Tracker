/*
  GPS_to_SD.ino

  Every 5 minutes:
    • Read one valid GPS fix from a Goouuu Tech GT-U7
    • Convert UTC → Pacific Time (PST/PDT, UTC−7)
    • Append a CSV line to "gpslog.csv" on SD card:

      LocalDate,LocalTime,Latitude,Longitude,Altitude_m,Speed_mph,NumSatellites

  Uses:
    • TinyGPSPlus   (for NMEA parsing)
    • SD            (for file I/O)
    • SPI           (underlying SD library)

  Connections:
      GT-U7  <-->   MKR Zero (Serial1)
      VCC      ->   5V
      GND      ->   GND
      TXD      ->   pin 13 (Serial1 RX)
      RXD      ->   pin 14 (Serial1 TX)

    SD card:
      Insert microSD into MKR Zero’s slot. SD chip-select = pin 4.
      (SAMD core defines SDCARD_SS_PIN = 4 on MKR Zero).

  Install TinyGPSPlus via Library Manager before compiling.
*/

#include <TinyGPSPlus.h>
#include <SPI.h>
#include <SD.h>

// GPS parser instance
TinyGPSPlus gps;

// SD chip-select pin for MKR Zero (SAMD core defines SDCARD_SS_PIN = 4)
static const int chipSelect = SDCARD_SS_PIN;  // const int chipSelect = SDCARD_SS_PIN;

// How often to log (milliseconds)
// 5 minutes = 5 * 60 000 ms = 300 000 ms
static const unsigned long LOG_INTERVAL_MS = 300000UL;

// Pacific Time offset from UTC (in hours):
//   Use -7 for PDT (spring/summer), or -8 for PST (winter)
//   If you want strictly PST year-round, replace -7 → -8.
static const int TIMEZONE_OFFSET_HOURS = -7;

// Keep track of when we last logged
unsigned long previousLogMillis = 0;


//------------------------------------------------------------------------------
//  Setup: open Serial, Serial1 (GPS), and SD
void setup() {
  // 1) USB Serial for debug
  Serial.begin(115200);
  Serial.println(F("=== GT-U7 → SD Logger ==="));

  // 2) Serial1 ↔ GT-U7 @ 9600 baud
  Serial1.begin(9600);
  Serial.println(F("Serial1 @ 9600 (MKR <--> GT-U7)"));

  // 3) Initialize SD card (CS = pin 4)
  Serial.print(F("Initializing SD card... "));
  if (!SD.begin(chipSelect)) {
    Serial.println(F("FAIL. Check SD card or wiring (CS=4)."));
    while (true) { delay(1000); } // Halt Here
  }
  Serial.println(F("OK"));

  // 4) Create CSV file + header if it doesn’t exist
  if (!SD.exists("gpslog.csv")) {
    File headerFile = SD.open("gpslog.csv", FILE_WRITE);
    if (headerFile) {
      headerFile.println(F("Local_Date,Local_Time,"
                          "Latitude,Longitude,Altitude_m,Speed_mph,Num_Satellites"));
      headerFile.close();
      Serial.println(F("Created gpslog.csv with header."));
    } else {
      Serial.println(F("Error: Could not create gpslog.csv"));
    }
  } else {
    Serial.println(F("gpslog.csv exists. Will append new lines."));
  }

  // 5) Set first logging to happen immediately
  previousLogMillis = millis() - LOG_INTERVAL_MS;
}


//------------------------------------------------------------------------------
//  Main loop: continuously parse GPS, log every 5 minutes
void loop() {
  // 1) Keep feeding TinyGPSPlus with incoming bytes
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // 2) Check if it's time to log
  unsigned long currentMillis = millis();
  if (currentMillis - previousLogMillis >= LOG_INTERVAL_MS) {
    previousLogMillis = currentMillis;
    logOneGPSFix();
  }

}


//------------------------------------------------------------------------------
//  Attempt to read one valid fix, convert to Pacific Time & mph, and append CSV
void logOneGPSFix() {
  Serial.println(F("\n--- Logging GPS Fix ---"));

  // 1) Wait up to 10 seconds for a valid location + valid date/time
  const unsigned long timeout = 10000UL; // 10 s
  unsigned long startWait = millis();
  while (millis() - startWait < timeout) {
    while (Serial1.available() > 0) {
      gps.encode(Serial1.read());
    }
    if (gps.location.isUpdated()) {
      break;
    }
  }

  // 2) If no valid fix/time arrived, skip logging
  if (!(gps.location.isValid())) {
    Serial.println(F("No valid GPS fix within 10 s! Skipping log."));
    return;
  }

  // 3) Grab UTC fields
  int localDay   = gps.date.day();
  int localMonth = gps.date.month();
  int localYear  = gps.date.year();

  int localHour   = gps.time.hour() + TIMEZONE_OFFSET_HOURS;
  int localMinute = gps.time.minute();
  int localSecond = gps.time.second();

  // If subtracting 7 (or 8) hours dropped below 0, wrap around to previous day
  if (localHour < 0) {
    localHour += 24;
    localDay -= 1;
    if (localDay < 1) {
      localMonth -= 1;
      if (localMonth < 1) {
        localMonth = 12;
        localYear -= 1;
      }
    }
  }

  // 4) Extract location, altitude, and speed from TinyGPSPlus
  double latitude       = gps.location.lat();
  double longitude      = gps.location.lng();
  double altitudeM      = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
  double speedMph       = gps.speed.isValid()    ? gps.speed.mph()      : 0.0;
  int    numSatellites  = gps.satellites.isValid() ? gps.satellites.value() : 0;

  static char latBuf[16];
  static char lngBuf[16];
  static char altBuf[16];
  static char spdBuf[16];

  // Convert each double to a String with respective decimals, then to char[]
  String temp;

  temp = String(latitude, 6);     // 6 decimals for latitude
  temp.toCharArray(latBuf, sizeof(latBuf));

  temp = String(longitude, 6);    // 6 decimals for longitude
  temp.toCharArray(lngBuf, sizeof(lngBuf));

  temp = String(altitudeM, 2);    // 2 decimals for altitude
  temp.toCharArray(altBuf, sizeof(altBuf));

  temp = String(speedMph, 2);     // 2 decimals for speed
  temp.toCharArray(spdBuf, sizeof(spdBuf));

  // 5) Build a CSV line:
  //    MM/DD/YYYY,HH:MM:SS,lat,lng,alt_m,speed_mph,numSat
  char lineBuf[128];
  snprintf(lineBuf, sizeof(lineBuf),
           "%02d/%02d/%04d,%02d:%02d:%02d,"
           "%s,%s,%s,%s,%d",
           localMonth, localDay, localYear,
           localHour, localMinute, localSecond,
           latBuf,    // full latitude text
           lngBuf,    // full longitude text
           altBuf,    // full altitude text
           spdBuf,    // full speed text
           numSatellites);

  Serial.print(F("CSV → "));
  Serial.println(lineBuf);

  // 7) Append that line to gpslog.csv on SD
  File dataFile = SD.open("gpslog.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(lineBuf);
    dataFile.close();
    Serial.println(F("Appended to gpslog.csv"));
  } else {
    Serial.println(F("Error: Could not open gpslog.csv for writing"));
  }
}
