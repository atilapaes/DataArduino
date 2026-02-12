// Clock module
#include <Wire.h>                   // for I2C communication
#include <RTClib.h>                 // for RTC
RTC_DS3231 rtc;                     // create rtc for the DS3231 RTC module, address is fixed at 0x68

//DHT11 sensor
#include <dht.h> 
dht DHT;
#define DHT11_PIN 4

// SD card module
#include <SD.h>
#include <SPI.h>
File log_File;
const int chipSelect = 10;
char csv_file_name[15]; // Array for csv filename
char dataStr[100];      // Array for Data string for the CSV file
char buffer[6];         // Buffer for float conversion
char timestamp[20];     // Buffer for the timestamp

// Modes for testing. serial_mode sends data to the computer, sd_mode writes data in the SD card.
// serial_mode is used for development and debug, and turned off for running the measurement system.
// sd_mode is used for writing the data in the SD card. Usefu for development and debug. Mandatory for measurement running log.
bool serial_mode = true;
bool sd_mode = true;

// Function for troubleshooting. This function helps to identify errors based in the number of blinks in the buildin LED
void errorBlinks(int n_blinks, int on_durantion, int off_duration) {
  for (int i=1; i<= n_blinks; i++) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(on_durantion);              // wait for a second (1000 milliseconds)
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(off_duration);     
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT); // Initialize the buildin LED for easy debug

  // Clock device presence check
  if (! rtc.begin()) {
    if (serial_mode){
      Serial.println(F("RTC Module not found"));
    }
    // If clock is not present, it will blink two, then a second-long pause
    while (1) {
      errorBlinks(2,200,600);
      delay(1000);

    } // Halt if no RTC is found  
  }
  
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //Sets the RTC to the date & time the sketch was compiled

  if (sd_mode) {
    if (SD.begin(chipSelect)) {
      if (serial_mode){
        Serial.println(F("SD card is present & ready"));
      }
    } 
    else {
      if (serial_mode){
        Serial.println(F("SD card missing or failure"));
      }
      // If SD not present, it will blink 3 times
      while(1) {
      errorBlinks(3,200,600);
      delay(1000);
      } //halt program
    }
  }
    // Future improvement involves dynamic file names in some way. Possible solution: use the timestamp as the filename, so it will not repeat.
    csv_file_name[0] = 0; // cleaning file name
    strcat(csv_file_name,"File3.txt"); //append the file extension
    
    //write csv headers to file:
    log_File = SD.open(csv_file_name, FILE_WRITE);  
    if (log_File) // it opened OK
    {
      //Serial.println(F("Writing headers to csv file"));
      log_File.println("dt,bat_v,dht_t,dht_h,ref_v,therm_v,out_v,in_v");
      log_File.close(); 
      
      // Blink once when writting on SD
      errorBlinks(1,200,500);
      delay(1000);
      //Serial.println("Headers written in SD"); // change for output status with led lights blink or display
    }
    else { 
      if (serial_mode){
      Serial.println("Error opening csv file");
      }
      // If SD not present, it will blink 3 times
      while(1) {
      errorBlinks(3,200,600);
      delay(1000);
      }
    }

  if (serial_mode){
    Serial.println(F("dt,bat_v,dht_h,dht_t,ref_v,therm_v,out_v,in_v"));  
  }
}

void loop() {
  
  // Reading sensors
  int chk = DHT.read11(DHT11_PIN); // DHT11 

  // Battery voltage divider. Used for checking the battery level
  int sensor_battery_vol_div = analogRead(A7);
  float battery_vol_div = sensor_battery_vol_div*(9.00/1023.00);

  // Indoors reference voltage divider  
  int sensor_ref_vol_div = analogRead(A3);
  float ref_vol_div = sensor_ref_vol_div*(5.00/1023.00);
  
  // Therm voltage divider  
  int sensor_therm_vol_div = analogRead(A2);
  float therm_vol_div = sensor_therm_vol_div*(2*ref_vol_div/1023.00);
  
  // Outdoors voltage divider  
  int sensor_out_vol_div = analogRead(A1);
  float out_vol_div = sensor_out_vol_div*(2*ref_vol_div/1023.00);

  // Indoors voltage divider  
  int sensor_in_vol_div = analogRead(A0);
  float in_vol_div = sensor_in_vol_div*(2*ref_vol_div/1023.00);

  //Generate the timestamp
  timestamp[0] = 0; // Cleaning the array
  DateTime now = rtc.now(); // Get the timestamp for the current datetime
  sprintf(timestamp, "%4d%02d%02dT%02d%02d%02d", now.year(),now.month(),now.day(),now.hour(), now.minute(), now.second()); // Timestamp with no divisor to save memory

  // Creating the array to append to the CSV file
  dataStr[0] = 0; // Clear dataStr and start with the timestamp

  // Timestamp
  strcat(dataStr,timestamp); //append the timestamp
  strcat(dataStr, ","); //append the delimeter

  // Battery voltage divider
  dtostrf(battery_vol_div, 5, 3, buffer);  //5 is mininum width, 3 is precision; float value is copied onto buff
  strcat( dataStr, buffer); //append the coverted float
  strcat( dataStr, ","); //append the delimeter
  
  // DHT reference indoors temperature
  dtostrf(DHT.temperature, 5, 1, buffer); 
  strcat( dataStr, buffer);
  strcat( dataStr, ",");

  // DHT reference indoors humidity
  dtostrf(DHT.humidity, 5, 1, buffer);
  strcat( dataStr, buffer);
  strcat( dataStr, ",");

  //  Reference indoors voltage divider
  dtostrf(ref_vol_div, 5, 3, buffer);
  strcat( dataStr, buffer);
  strcat( dataStr, ",");

  // Voltage divider with one resistor outdoors
  dtostrf(out_vol_div, 5, 3, buffer);
  strcat( dataStr, buffer);
  strcat( dataStr, ",");

  // Voltage divider with thermistor outdoors
  dtostrf(therm_vol_div, 5, 3, buffer);
  strcat( dataStr, buffer); //append the coverted float
  strcat( dataStr, ","); //append the delimeter

  // Voltage divider with one resistor indoors, close to the hot air vent
  dtostrf(in_vol_div, 5, 3, buffer);
  strcat( dataStr, buffer);
  strcat( dataStr, 0);
  Serial.println(dataStr);

  // Serial mode for debugging 
  if (serial_mode){
    Serial.println(dataStr);
  }

  if (sd_mode){
    // Open the file. Note that only one file can be open at a time,
    log_File = SD.open(csv_file_name, FILE_WRITE);     
    // if the file opened okay, write to it:
    if (log_File) 
    {
      if (serial_mode){
        Serial.println(F("Writing to SD"));
      }

      log_File.println(dataStr);  // write the new row in the CSV file
      log_File.close(); // Close the CSV file

      // Blink once when writting on SD
      errorBlinks(1,200,400);
      delay(400);

    } 
    else 
    {
      if (serial_mode){
        Serial.println(F("Cannot open csv"));
      }

      // LED goes on and the program stops if the wirting is unsuccessful
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(5000);
      // Blink 3 times in case of SD problems
      while (1){
        // If SD not present, it will blink 3 times
        errorBlinks(3,200,600);
        delay(1000);
      }
    }
  }
  //delay(1000);               // wait for a second to start the new measuring ccycle
}
