#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <HTTPClient.h>

BLEScan* pBLEScan;

//Calibration Parameters!!!
const short near_thrsh = -55; //dB
const short far_thrsh = -82;  //dB
const short timeout = 5; //in seconds, 
                         //how long to wait after no device is found. 
                         //(Turn off in case the BLE device dies).


//Change the following with your information
const char* sit   = "https://maker.ifttt.com/trigger/Sit/with/key/XXXXXXX";
const char* stand = "https://maker.ifttt.com/trigger/Stand/with/key/xxxxxxx";
const char* ssid = "Wifi Name";
const char* pswd = "WiFi Password";

const char* myDevice = "XX:XX:XX:XX:XX";  //For single user applications

//Some helper variables
short prox = 0;
bool near = true;
long sat = 0;


void wifiTask() {
      WiFi.begin(ssid, pswd);
      Serial.print("Connecting to Wifi");
      while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(300);
      }
      Serial.println();
      Serial.print("Connected with IP: ");
      Serial.println(WiFi.localIP());
      

      //Future Failproof, in case of bad key or non responsive internet
      //Serial.print("Ping Host: ");
      //Serial.println(apiurl);
/*
      if(Ping.ping(remote_host)){
        Serial.println("Success!!");
      }else{
        Serial.println("ERROR!!");
      }
*/
}


void wifis(const char* action){
// wait for WiFi connection
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
  
    Serial.print("[HTTP] begin...\n");
    http.begin(action); //HTTP
    Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();

    if(httpCode > 0) {
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      if(httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  
  }else{
    wifiTask();
    wifis(action);
  }
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        //Comment Next Lines for a cleaner Console
        Serial.print(advertisedDevice.toString().c_str());
        Serial.println((int)advertisedDevice.getRSSI());
        //Serial.println(advertisedDevice.getManufacturerData().c_str());
        //Serial.println(advertisedDevice.getAddressType());
        
        //Comment following line and uncomment the next one or the next to that if you want to filter by MAC Address or Name
        if(true){   //Any BLE advertiser can trigger the sensor
        //if(strcmp(advertisedDevice.getAddress().toString().c_str(), myDevice)==0){    //Filter by MAC Address
        //if(strcmp(advertisedDevice.getName().c_str(), myDevice)==0){                  //Filter by Name
            
            
            Serial.print("Found The Device: ");
            prox = (int)advertisedDevice.getRSSI();
            Serial.println(prox);
            sat = millis();
            
            //Determine if it is close enough to turn on
            //Or if it is far enough to turn off
            if((prox > near_thrsh)&&(near)){
                //If the device is closer than the short threshold
                Serial.println("Device in Close Range");
                Serial.println("Sitting");
                wifis(sit);
                near = false;
            }else if((prox < far_thrsh) && (!near)){
                //If the device is farther than the long threshold
                Serial.println("Device went away");
                Serial.println("Standing");
                wifis(stand);
                near = true;
            }
            
        //If device found is not target (loophole if no bluetooth devices arround, unlikely!)
        }else{
            //If last known time of the device is more than 5s then turn off
            if((millis()-sat>(timeout*1000))&&(!near)){
                Serial.println("Standing");
                wifis(stand);
                near = true;
                //We don't reset timer to avoid making it persistent,
                //in case the light is turned on by another means/reason.
            }
        }
        //Probably not the best place
        pBLEScan->clearResults();
    }
};

void bleTask(){
  // Create the BLE Device
  BLEDevice::init("ESP32 THAT PROJECT");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
  //Serial.println("Waiting a client connection to notify...");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //Setup Wifi
  Serial.println("WIFI MODE");
  wifiTask();

  //Setup BLE
  Serial.println("BLE MODE");
  bleTask();
  //BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  BLEScanResults foundDevices = pBLEScan->start(0);
}



void loop() {
//////Following code needs testing!//////
//It is a turn off failsafe in case the unlikely event that
//No bluetooth device is near AND trigger device suddenly stopped broadcasting
/*
//If last known time of the device is more than 5s then turn off
if((millis()-sat>(timeout*1000))&&(!near)){
   Serial.println("Standing");
   wifis(stand);
   near = true;
   //We don't reset timer to avoid making it persistent,
   //in case the light is turned on by another means/reason.
}*/

}
