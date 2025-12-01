/*

  Final - Gateway - v1.0.0
  
  Author: João Vitor Galdino Souto Alvares  
  Date: 16/06/2023
  
  Description: Firmware final do Gateway.
         
         -> ESP32
         
          -> With Debug
          
          This program is utilizing 836542 (63%) bytes of the memory FLASH
          The maximum is of 1310720 (1.3MB) bytes of memory FLASH
          
          This program is utilizing 43180 (13%) bytes of the memory RAM
          The maximum is of 294912 (294KB) bytes of memory RAM
          
          -> Without Debug
          
          This program is utilizing 833258 (63%) bytes of the memory FLASH
          The maximum is of 1310720 (1.3MB) bytes of memory FLASH
          
          This program is utilizing 43180 (13%) bytes of the memory RAM
          The maximum is of 294912 (294KB) bytes of memory RAM

*/

//*****************************************************************************************************************
// Library(ies)
//*****************************************************************************************************************

//#define FASTLED_ALLOW_INTERRUPTS 0
// #include "fastled.h"

// To Wifi
#include <WiFiUdp.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// To OTA
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

// To HTTP
#include <HTTPClient.h>

// To MQTT
#include <PubSubClient.h>

// To Arduino Json
#include <ArduinoJson.h>
#include <Arduino_JSON.h>

// To LoRa
#include <SPI.h>
#include <LoRa.h>

// To ESP32
#include <Wire.h>  
#include <Arduino.h> 
#include "EEPROM.h"

// To String
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// To Math
#include <math.h> 

// To I2C
#include <LiquidCrystal_I2C.h>

// To BLE
#include <NimBLEDevice.h>

// To serial
#include <SoftwareSerial.h>

// To GPS
#include <TinyGPS++.h>

// To RTC
#include "RTCDS1307.h"

// To SDCARD
#include "FS.h"
#include "SD.h"

// To buzzer
#include <Tone32.h>
#include <pitches.h>

//*****************************************************************************************************************
// Contant(s)
//*****************************************************************************************************************

// Hardware mapping
// To peripherals
#define pinToCS1            5
#define pinToSerialESP_AtmegaRX     16      
#define pinToSerialESP_AtmegaTX     17  
#define pinToBuzzer           4   
#define pinToLED            2   
#define pinToRST_GPS          15
#define pinToGATE_GPS         13
#define pinToRX_GPS           26
#define pinToTX_GPS           27
#define pinToPushButton         25
// To LoRa        
#define SCK                 18                      // GPIO5  -- SX127x's SCK
#define MISO                19                    // GPIO19 -- SX127x's MISO
#define MOSI                23                    // GPIO27 -- SX127x's MOSI
#define SS                  14                    // GPIO18 -- SX127x's CS
#define RST                 33                    // GPIO14 -- SX127x's RESET
#define DI00                32                    // GPIO26 -- SX127x's IRQ(Interrupt Request)

// Macro(s) generic(s)        
#define DEBUG       
#define DEBUG_NOW       
// #define DEBUG_TIME       
#define FIRMWARE_VERSION        "FIRMWARE VERSION 1.0.0"
#define DATA_UPDATE_FIRMWARE      "DATA 16-06-2023"
#define MESSAGE_FIRMWARE        "FIRMWARE GATEWAY!"
#define MESSAGE_SERIAL_NUMBER     "SN: "
#define SERIAL_NUMBER_PRINT       743587
        
// To Serial        
#define SERIAL                  115200
#define SERIAL_GPS            9600
#define SERIAL_ATMEGA         9600        
        
// To time        
#define timeToWait              1
#define timeToSendGS          60000
#define timeToBoot            30000
#define timeToTAGO_IO         5000
#define TIME_FINAL            60000
#define zeroSample            0
#define maxSample           255
#define minSendTime           1
#define maxSendTime           255
#define fixTimeToSend         1000

// To LoRa
#define BAND_END_DEVICE1        915E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE2        916E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE3        917E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE4        918E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE5        919E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE6        920E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE7        921E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE8        922E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE9        923E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE10       924E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE11       925E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE12       926E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE13       927E6                             // Select the band01 = 917MHz
#define BAND_END_DEVICE14       928E6                             // Select the band01 = 917MHz
#define PA_SET                true                                // If true = RFO / false = PABOOST
#define SF                9
// #define PA_Value           15
#define PA_Value            1
#define BW                125E6
#define CR                5
#define maxCounterReconnectSetupLoRa  240
#define minCounterReconnectSetupLoRa  0
#define maxSendLoRa           3

// To Wifi
#define maxCounterReconnectSetupWifi  240
#define minCounterReconnectSetupWifi  0
#define MSG_BUFFER_SIZE         100
#define MAX_SIZE_JSON         500
#define valueToResetDevice        500
#define messageSng            "sng"
#define messageSnd            "snd"
#define messageRd           "rd"
#define messageSnr            "snr"
#define messageRw           "rw"
#define messageTi             "ti"
#define messageTe             "te"
#define messageUe             "ue"
#define messagerfid           "rfid"
#define messageRs           "rs"
#define messageLm           "lm"
#define messageAtm            "atm"
#define messageAlt            "alt"
#define messageVz           "vz"
#define messageVt           "vt"
#define message4m1            "4m1"
#define message4m2            "4m2" 
#define message4m3            "4m3"
#define message4m4            "4m4"
#define messageAn1            "an1"
#define messageAn2            "an2"
#define messageTen            "ten"

// To BLE
#define SERVICE_UUID                "ab0828b1-198e-4351-b779-901fa0e0371e"  // UART service UUID.
#define CHARACTERISTIC_UUID_RX      "4ac8a682-9736-4e5d-932b-e9b31405049c"
#define CHARACTERISTIC_UUID_TX      "0972EF8C-7613-4075-AD52-756F33D4DA91"

// To memory EEPROM
#define flagSetupEEPROM         0
#define flagConfigBLE         1
#define flagUpdate            2
#define flagSDCard            3
#define flagExtra01           4
#define flagExtra02           5 
#define flagExtra03           6
#define flagExtra04           7
#define flagExtra05           8
#define flagExtra06           9
#define flagExtra07           10
#define flagExtra08           11
#define serialNumber          12
#define netSSID             22
#define netPassword           52
#define SNED01              82
#define SNED02              88
#define SNED03              94
#define SNED04              100
#define SNED05              106
#define SNED06              112
#define SNED07              118
#define SNED08              124
#define SNED09              130
#define SNED10              136
#define SNED11              142
#define SNED12              148
#define SNED13              154
#define SNED14              160
#define SNED15              166
#define SNED16              172
#define flagSendTime          178
#define EEPROM_SIZE           200
#define MAX_EEPROM            200
#define MAX_VALUE           30

// To setupBLE
#define startConfiguraiton        "GAT-OK"                // To mode standalone
#define serialNumberGateway       "GAT-SNGAT"               // To serial number Gateway
#define passwordConfig          "GAT-PS"                // To password to BLE
#define serialNumberED01        "GAT-ED01"                // To serial number End-Device 01
#define serialNumberED02        "GAT-ED02"                // To serial number End-Device 02
#define serialNumberED03        "GAT-ED03"                // To serial number End-Device 03
#define serialNumberED04        "GAT-ED04"                // To serial number End-Device 04
#define serialNumberED05        "GAT-ED05"                // To serial number End-Device 05
#define serialNumberED06        "GAT-ED06"                // To serial number End-Device 06
#define serialNumberED07        "GAT-ED07"                // To serial number End-Device 07
#define serialNumberED08        "GAT-ED08"                // To serial number End-Device 08
// Extra
#define serialNumberED09        "GAT-ED09"                // To serial number End-Device 09
#define serialNumberED10        "GAT-ED10"                // To serial number End-Device 10
#define serialNumberED11        "GAT-ED11"                // To serial number End-Device 11
#define serialNumberED12        "GAT-ED12"                // To serial number End-Device 12
#define serialNumberED13        "GAT-ED13"                // To serial number End-Device 13
#define serialNumberED14        "GAT-ED14"                // To serial number End-Device 14
#define serialNumberED15        "GAT-ED15"                // To serial number End-Device 15
#define serialNumberED16        "GAT-ED16"                // To serial number End-Device 16
#define modeOperateSTime        "GAT-TE:"               // TEMPO DE ENVIO (MÚLTIPLO DE 1000)
#define modeSendServer00        "GAT-SERVER01"              // GOOGLE SHEETS + TAGO
#define modeSendServer01        "GAT-SERVER02"              // GOOGLE
#define modeSendServer02        "GAT-SERVER03"              // TAGO
#define modeSendServer03        "GAT-SERVER04"              // EMPTY
#define modeSendServer04        "GAT-SERVER05"              // EMPTY
#define ssidBLE             "SD:"                 // To SSID
#define passwordBLE           "PS:"                 // To password
#define finishSetupBLE          "GAT-FINISH"              // To finish
#define restartDevice         "GAT-RESTART"             // To restart
#define updateDevice          "GAT-UPDATE"              // To update
#define enableSDCard          "GAT-SD-OK"               // To enable SDCARD
#define disableSDCard         "GAT-SD-NAO-OK"             // To disable SDCARD
#define consultSNGateway        "GAT-SNGAT???"              // To verifier serial number End-Device
#define modeOTA             "GAT-OTAJVTECH"             // To OTA

// To LCD
#define screenWidth               16      
#define screenHeight              2     
#define minScreenWidthPosition        0
#define minScreenHeightPosition       0
#define maxScreenWidthPosition        16
#define maxScreenHeightPosition       1
#define startGateway          "  Iniciando...  "
#define nameDevice            " GATEWAY JVTECH "  
#define serialNumberToLCD       " SERIAL NUMBER  "  
#define versionFirmwareName       " VERSAO FIRMWARE"
#define versionFirmware         "     1.0.0      "
#define dataUpdateName          "DATA DE ATUALIZ."
#define dataUpdate            "   16/06/2023   "
#define waitToLCD           "Aguarde...      "
#define waitToLCD2            "Aguardando...   "
#define messageData           " Data e hora ok!"
#define setupBLEMessage         "Configuracao BLE"
#define setupBLEMessage2        "Config. GATEWAY "
#define setupBLEMessage3        "BLE Configurado "
#define setupBLEMessage4        "BLE Disconectado"
#define finishBLEMessage        "      BLE OK   "
#define setupBLEMessage5        "   Finalizada   "
#define setupWifiMessage        "  Configurando  "
#define setupWifiMessage1       "     WIFI...    "
#define setupWifiMessage2       "Config. WIFI OK "
#define setupLoRaMessage        "  Configurando  "
#define setupLoRaMessage1       "     LoRa...    "
#define setupLoRaMessage2       "Config. LoRa OK "
#define operateMessage          "  GATEWAY LoRa  "
#define operateMessage1         "    Online"

//*****************************************************************************************************************
// Prototype of the function(s)
//*****************************************************************************************************************

// To setup
void setupSerial();
void setupPins();
void setupDisplay();
void setupRTC();
void setupSPI();
// void setupSDCard();
// void setupGPS();
void setupEEPROM();
void setupBLE();
// void setupOTA(); - NEXT VERSION
void setupWifi();
void setupLoRa();
void writeStringToEEPROM(char add, String data);
void readStringLikeChar(char data[], char add, int maxValue);
int readStringLikeInt(int add, int maxValue);
String readStringLikeString(int add, int maxValue);
void interfaceSetupBLE();
void interfaceBLE();

// To operate
void readRTC();
void functionPushButton();
// To LoRa
void onReceive(int packetSize);                         // Function that receive packet for interrupt
void sendMessageLoRa();
void dataBaseLoRa();
// To web
void toVerifierWifi();
String httpGETRequest(const char* serverName);
void errorHTTP();
void toVerifierResponseHTTP1();
void toVerifierResponseHTTP2();
void sendToGS();
void dataBaseWeb();
// void sendToTAGOTeste();
void sendToTAGO();
// To display
void showInTheDisplay();
// To SDCARD
void saveDataInSDCard();

//*****************************************************************************************************************
// Object(s)
//*****************************************************************************************************************
        
// To wifi          
// Object to Wifi e MQTTMQTT
WiFiClient espClient;
PubSubClient client(espClient);       
WiFiClientSecure client01;      
WiFiClient client02;      

// To OTA - NEXT VERSION
// WebServer server(80);

// To bluetooth       
//BLECharacteristic *characteristicTX;                      // Object to BLE.
NimBLECharacteristic* characteristicTX;         
static NimBLEAdvertisedDevice* advDevice;
static NimBLEServer* pServer;          
static NimBLERemoteService* pClient;  
        
// To serial
SoftwareSerial serialESP32(pinToSerialESP_AtmegaRX, pinToSerialESP_AtmegaTX);   

// To LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);          // Verificar
//LiquidCrystal_I2C lcd(0x27, 16, 2);               

// To GPS
// To Hardware serial          
HardwareSerial neogps(2);
// To GPS
TinyGPSPlus gps;        

// To RTC
RTCDS1307 rtc(0x68);      
        
//*****************************************************************************************************************
// Global variable(s)
//*****************************************************************************************************************               

// Struct to End-Device
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice;
endDevice dataED;

// Struct to End-Device wrong
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice0;
endDevice0  dataED0;

// Struct to End-Device 1
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice1;
endDevice1  dataED1;

// Struct to End-Device 2
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice2;
endDevice2  dataED2;
// Struct to End-Device 3
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice3;
endDevice3  dataED3;
// Struct to End-Device 4
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice4;
endDevice4  dataED4;
// Struct to End-Device 5
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice5;
endDevice5  dataED5;
// Struct to End-Device 6
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice6;
endDevice6  dataED6;
// Struct to End-Device 7
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice7;
endDevice7  dataED7;
// Struct to End-Device 8
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice8;
endDevice8  dataED8;
// Extra
// Struct to End-Device 9
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice9;
endDevice9  dataED9;
// Struct to End-Device 10
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice10;
endDevice10 dataED10;
// Struct to End-Device 11
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice11;
endDevice11 dataED11;
// Struct to End-Device 12
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice12;
endDevice12 dataED12;
// Struct to End-Device 13
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice13;
endDevice13 dataED13;
// Struct to End-Device 14
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice14;
endDevice14 dataED14;
// Struct to End-Device 15
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice15;
endDevice15 dataED15;
// Struct to End-Device 16
typedef struct
{
  
  // Header
  int snd_ed;
  
  // To intern sensor
  float data01;
  float data02;
  float data03;
  
  // To extern sensor
  float data04;
  float data05;
  float data06;
  float data07;
  float data08;
  float data09;
  float data10;
  float data11;
  float data12;
  float data13;
  float data14;
  
  // To relay
  int data15;
  int data16;
  
  // To verifier
  int counterLoRa   = 0;
  
} endDevice16;
endDevice16 dataED16;

// Struct to Gateway
typedef struct
{
  
  // Header
  int snd_ed;
  int sng_ed    = SERIAL_NUMBER_PRINT;
  
  // To RFID
  int data01;
  
  // To relay
  int data02;
  
} gatewayDevice;
gatewayDevice dataGAT;
    
// To boot
bool deviceConnected          = false;
bool waitSetupBLE           = true;                   // Controller the device connected.
bool flagSetupWifi            = false;
bool flagSetupBLE           = true;
String valueToBLE;    

// To push button
bool flagToPushButton         = false;
bool flagToSelectFunc         = false;
int counterPushButton         = 0;
int counterPB               = 0;
int valueToRestart            = 100;

// To time
unsigned long timeBefore        = 0;
unsigned long timeAfter         = 0;
unsigned long timeToSendWeb       = 0;
unsigned long timeToSendWebNow      = 0;

// To network   
char SERIAL_NUMBER[7]         = "743587";               // To test
char passwordDeviceED[10]       = "5enh@GAT";
const char* SSID            = "VIVOFIBRA-F548";
const char* PASSWORD          = "72232EF548";
//const char* SSID            = "JVTECH";
//const char* PASSWORD          = "lorajvtech";
int counterReconnectSetupWifi     = 0;
bool flagReceiveLoRa          = false;
bool flagReceiveLoRaSDCard        = false;
int valueRSSI             = 0;
int counterStatusWifi         = 0;
    
// To Other server
// const char* serverName1        = "";
// const char* serverName2        = "";     
int httpResponseCode1;      
int httpResponseCode2;    
String payload              = "";   
String dataReadingsArr[5];
    
// To Google Sheets
char *serverGS              = "script.google.com";                // Server URL
char *GScriptId             = "AKfycbyAdvrw2o4O8NwTFeMlWduYDDZX_zIP97vCXMUcTkWCopVBP27BlIHLRw0rkikPMMMSJQ";
const int portGS            = 443; 
// https://script.google.com/macros/s/AKfycbxcMmZ-cev8gdI6UsSJ1ouOcCops6COglnRGQV-P8oUJsGzaSiWU_iAUIQjUcn4Liz4/exec
  
// To TAGO-IO 
String apiKey               = "5944076c-a0cb-4709-a992-eceffdc8ffa9";         // O token vai aqui
const char* serverTago          = "api.tago.io";
const int portTago            = 80;
int counterTAGO             = 0;  
int counterToTAGO           = 0;  
  
// To OTA
/*
const char* hostOTA           = "GAT-JVTECH";
char IP_LOCAL[MSG_BUFFER_SIZE];
*/  
  
// To LoRa
int counterReconnectSetupLoRa     = 0;
bool flagEDCount            = false;    
float RSSIDevice01            = 0;
float SNRDevice01           = 0;    
bool toSendServer           = false;    
bool toReceiveLoRa            = false;  
bool flagToSendLoRa           = false;
bool flagToDadaBase01         = false;
bool flagToDadaBase02         = false;
bool flagToDadaBase03         = false;
bool flagToDadaBase04         = false;
bool flagToDadaBase05         = false;
bool flagToDadaBase06         = false;
bool flagToDadaBase07         = false;
bool flagToDadaBase08         = false;
// Extra
bool flagToDadaBase09         = false;
bool flagToDadaBase10         = false;
bool flagToDadaBase11         = false;
bool flagToDadaBase12         = false;
bool flagToDadaBase13         = false;
bool flagToDadaBase14         = false;
bool flagToDadaBase15         = false;
bool flagToDadaBase16         = false;
int serialNumberGATToSend;
int serialNumberEDToSend01; 
int serialNumberEDToSend02; 
int serialNumberEDToSend03; 
int serialNumberEDToSend04; 
int serialNumberEDToSend05; 
int serialNumberEDToSend06; 
int serialNumberEDToSend07;
int serialNumberEDToSend08;
// Extra
int serialNumberEDToSend09;
int serialNumberEDToSend10;
int serialNumberEDToSend11;
int serialNumberEDToSend12;
int serialNumberEDToSend13;
int serialNumberEDToSend14;
int serialNumberEDToSend15;
int serialNumberEDToSend16;
int valueRSSILoRa           = 0;
int valueRSSILoRa1            = 0;
int valueRSSILoRa2            = 0;
int valueRSSILoRa3            = 0;
int valueRSSILoRa4            = 0;
int valueRSSILoRa5            = 0;
int valueRSSILoRa6            = 0;
int valueRSSILoRa7            = 0;
int valueRSSILoRa8            = 0;
// Extra
int valueRSSILoRa9            = 0;
int valueRSSILoRa10           = 0;
int valueRSSILoRa11           = 0;
int valueRSSILoRa12           = 0;
int valueRSSILoRa13           = 0;
int valueRSSILoRa14           = 0;
int valueRSSILoRa15           = 0;
int valueRSSILoRa16           = 0;
int valueSNRLoRa            = 0;
int valueSNRLoRa1           = 0;
int valueSNRLoRa2           = 0;
int valueSNRLoRa3           = 0;
int valueSNRLoRa4           = 0;
int valueSNRLoRa5           = 0;
int valueSNRLoRa6           = 0;
int valueSNRLoRa7           = 0;
int valueSNRLoRa8           = 0;
// Extra
int valueSNRLoRa9           = 0;
int valueSNRLoRa10            = 0;
int valueSNRLoRa11            = 0;
int valueSNRLoRa12            = 0;
int valueSNRLoRa13            = 0;
int valueSNRLoRa14            = 0;
int valueSNRLoRa15            = 0;
int valueSNRLoRa16            = 0;
int valueDataBaseLoRa         = 0;
int valueToSendLoRaDataBase       = 0;

// To time  
unsigned long timeToWaitGS        = 0;
unsigned long timeToWaitBoot      = 0;
int counterToReset            = 0;

// To values EEPROM
char levelEEPROM[3]           = "";
char serialNumberEEPROM[MAX_VALUE/5]  = "";
char ssidEEPROM[MAX_VALUE]        = "";
char passwordEEPROM[MAX_VALUE]      = "";
char serialNEDLikeCh01[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh02[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh03[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh04[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh05[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh06[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh07[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh08[MAX_VALUE/5]   = "";
// Extra  
char serialNEDLikeCh09[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh10[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh11[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh12[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh13[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh14[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh15[MAX_VALUE/5]   = ""; 
char serialNEDLikeCh16[MAX_VALUE/5]   = ""; 

// To BLE
bool flagToStartSetup         = false;
bool flagSerialNumber         = false;
bool flagPasswordConfig         = false;
bool flagOTA              = false;
bool flagFinishBLE            = false;

// To display
char valueToDisplay           = 0x00;             
int count               = 0;
char AddressDevice            = 0x27;
bool flagLCD_ENABLE           = false;
bool flagLOCKER_ENABLE          = false;
bool flagRFID_ENABLE          = false;
char flagLCDMessage           = 0x00;
bool flagEnableTesteLCD         = false;

// To RTC   
// char diasDaSemana[7][12]       = {"Domingo", "Segunda", "Terca", "Quarta", "Quinta", "Sexta", "Sabado"}; // Dias da semana
String m[12]              = {"Janeiro", "Fevereiro", "Março", "Abril", "Maio", "Junho", "Julho", "Agosto", "Setembro", "Outubro", "Novembro", "Dezembro"};
String w[7]               = {"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sábado"};
bool period               = 0;
uint8_t year, month, weekday, day, hour, minute, second;

// To caracte LCD
byte level01[8]             = {0b00001,  0b00101,  0b10101,  0b10101,  0b10101,  0b10101,  0b10101,  0b10101};  // FULL 15
byte level02[8]             = {0b00000,  0b00100,  0b10100,  0b10100,  0b10100,  0b10100,  0b10100,  0b10100};
byte level03[8]             = {0b00000,  0b00000,  0b10000,  0b10000,  0b10000,  0b10000,  0b10000,  0b10000};
byte level04[8]             = {0b00000,  0b00000,  0b00000,  0b00001,  0b00101,  0b10101,  0b10101,  0b10101};  // FULL 14
byte level05[8]             = {0b00000,  0b00000,  0b00000,  0b00000,  0b00100,  0b10100,  0b10100,  0b10100};  
byte level06[8]             = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b10000,  0b10000,  0b10000};  
byte level07[8]             = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};  // ZERADO
byte level08[8]             = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00001,  0b00101};  // FULL 13
byte level09[8]             = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00100};  

int valueSerialNow;
int counterFirst;
int counterSecond;
int frequencyNow;

// To webserver
/*
const char* loginIndex = 
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>JVTECH - identifique-se</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<td>Login:</td>"
        "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Senha:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Identificar'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='gat-jvtech' && form.pwd.value=='admin123')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Login ou senha inválidos')"
    "}"
    "}"
"</script>";
  
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>Progresso: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('Progresso: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('Sucesso!')"
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";
*/

//*****************************************************************************************************************
// Classes to BLE
//*****************************************************************************************************************

// Callback to receive the events
class ServerCallbacks: public NimBLEServerCallbacks 
{
  
    void onConnect(NimBLEServer* pServer) 
  {
     
    deviceConnected = true;
    
    #ifdef  DEBUG
    Serial.println("The device connect ok! ");
    Serial.println();
    delay(timeToWait);
    #endif  
    
  }; // end onConnect

    void onDisconnect(NimBLEServer* pServer) 
  {
     
    deviceConnected = false;
    
    #ifdef  DEBUG
    Serial.println("The device disconnect ok! ");
    Serial.println();
    delay(timeToWait);
    #endif 
  
  }; // end onDisconnect

}; // end class ServerCallbacks

// Callback to event of the characteristic
class CharacteristicCallbacks: public NimBLECharacteristicCallbacks 
{
    
  void onWrite(NimBLECharacteristic *characteristic) 
  {
    
    std::string rxValue = characteristic->getValue();             // Return the pointer to register the value current of the characteristic.
     
    if (rxValue.length() > 0)                         // Verifier if exist the data. The size of the value is better that zero.
    {
        
      valueToBLE  = " "; 
        
      for (int i = 0; i < rxValue.length(); i++) 
      {
              
        valueToBLE = valueToBLE + rxValue[i];
      
      } // end for
      
      //**********************************************
      // Start configuration
      //**********************************************
      if (rxValue.find(startConfiguraiton)      != -1) 
      {
        
        //int n = valueToBLE.length();
        //char receiveValue[n+1];
        //strcpy(receiveValue,valueToBLE.c_str());  
        //flagStartConfiguration = true;
        
        flagToStartSetup  = true;
        
        interfaceSetupBLE();
        interfaceBLE();
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("INICIO BLE OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(" "); 
        delay(2000*timeToWait);
        
        #ifdef  DEBUG
        Serial.print("GAT-OK! - Value of BLE is: ");
        Serial.print(valueToBLE);
        Serial.println();
        delay(timeToWait);
        #endif
        
      } // end if
      
      //**********************************************
      // Mode Serial number                       
      //**********************************************
      else if (rxValue.find(serialNumberGateway)    != -1 &&  (flagToStartSetup))
      {
        
        int find2Point    = valueToBLE.indexOf(':');
        String SNGATToBLE = valueToBLE.substring(find2Point+1);
        
        int valueIntSN    = SNGATToBLE.toInt();
                
        #ifdef DEBUG
        Serial.print("GAT-SNGAT! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER: ");
        Serial.print(SNGATToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif          
        
        delay(500*timeToWait);
        
        if(valueIntSN   ==  SERIAL_NUMBER_PRINT)  
        {
          
          flagSerialNumber  = true;
          interfaceSetupBLE();
          interfaceBLE();
          
          lcd.clear();
          lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
          lcd.print("SERIAL NUMBER OK"); 
          lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
          lcd.print(valueIntSN); 
          delay(2000*timeToWait);
          
        } // end if
        
        else
        {
          
          // Let's convert the value to a char array:
          char txString[16];                          // Make sure this is big enuffz.
          snprintf(txString, 20, "%s","ENTRADA-ERRADA");            // float_val, min_width, digits_after_decimal, char_buffer.
        
          characteristicTX->setValue(txString);                 // Set the value that send.       
          characteristicTX->notify();                     // Send the value of the smartphone.          
          
          interfaceSetupBLE();
          interfaceSetupBLE();
          interfaceSetupBLE();
          interfaceSetupBLE();
          interfaceSetupBLE();
          
          lcd.clear();
          lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
          lcd.print("ENTRADA ERRADA"); 
          lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
          lcd.print(valueIntSN); 
          delay(2000*timeToWait);
          
        } // end 
    
      } // end else if
      
      //**********************************************
      // Mode senha                                     
      //**********************************************  
      else if (rxValue.find(passwordConfig)       != -1 &&  (flagSerialNumber))
      {
        
        int find2Point      = valueToBLE.indexOf(':');
        String passWordToBLE  = valueToBLE.substring(find2Point+1);
                
        #ifdef DEBUG
        Serial.print("GAT-PS! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("PASSWORD GAT: ");
        Serial.print(passWordToBLE);          
        Serial.println();
        delay(timeToWait);
        #endif          
        
        int n = passWordToBLE.length();
        char receiveValue[n+1];
        strcpy(receiveValue,passWordToBLE.c_str()); 
        
        delay(500*timeToWait);
        
        if (strcmp(receiveValue, passwordDeviceED) == 0)
        {
          
          flagPasswordConfig = true;
          interfaceSetupBLE();
          interfaceBLE();
          
          lcd.clear();
          lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
          lcd.print("SENHA GATEWAY"); 
          lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
          lcd.print(receiveValue); 
          delay(2000*timeToWait);
          
        } // end if
        
        else
        {
          
          // Let's convert the value to a char array:
          char txString[16];                          // Make sure this is big enuffz.
          snprintf(txString, 20, "%s","ENTRADA-ERRADA");            // float_val, min_width, digits_after_decimal, char_buffer.
        
          characteristicTX->setValue(txString);                 // Set the value that send.       
          characteristicTX->notify();                     // Send the value of the smartphone.          
          
          interfaceSetupBLE();
          interfaceSetupBLE();
          interfaceSetupBLE();
          interfaceSetupBLE();
          interfaceSetupBLE();
          
          lcd.clear();
          lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
          lcd.print("ENTRADA ERRADA"); 
          lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
          lcd.print(receiveValue); 
          delay(2000*timeToWait);
          
        } // end 
        
      } // end else if
      
      //**********************************************
      // Mode Serial number EDs             
      //**********************************************    
      else if (rxValue.find(serialNumberED01)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED01; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice01  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED01, SNEndDevice01);
        
        serialNumberEDToSend01  = SNEndDevice01.toInt();
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice01); 
        delay(2000*timeToWait);       
        
        #ifdef DEBUG
        Serial.print("GAT-ED01! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED01: ");
        Serial.println(SNEndDevice01);
        Serial.print("SERIAL NUMBER ED01: ");
        Serial.println(serialNumberEDToSend01);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if  
      else if (rxValue.find(serialNumberED02)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED02; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice02  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED02, SNEndDevice02);
        
        serialNumberEDToSend02  = SNEndDevice02.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice02); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED02! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED02: ");
        Serial.println(SNEndDevice02);    
        Serial.print("SERIAL NUMBER ED02: ");
        Serial.println(serialNumberEDToSend02);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if
      else if (rxValue.find(serialNumberED03)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED03; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice03  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED03, SNEndDevice03);
        
        serialNumberEDToSend03  = SNEndDevice03.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice03); 
        delay(2000*timeToWait);         
        
        #ifdef DEBUG
        Serial.print("GAT-ED03! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED03: ");
        Serial.println(SNEndDevice03);  
        Serial.print("SERIAL NUMBER ED03: ");
        Serial.println(serialNumberEDToSend03);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if
      else if (rxValue.find(serialNumberED04)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED04; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice04  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED04, SNEndDevice04);
        
        serialNumberEDToSend04  = SNEndDevice04.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice04); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED04! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED04: ");
        Serial.println(SNEndDevice04);      
        Serial.print("SERIAL NUMBER ED04: ");
        Serial.println(serialNumberEDToSend04);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if
      else if (rxValue.find(serialNumberED05)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED05; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice05  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED05, SNEndDevice05);
        
        serialNumberEDToSend05  = SNEndDevice05.toInt();          
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice05); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED05! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED05: ");
        Serial.println(SNEndDevice05);          
        Serial.print("SERIAL NUMBER ED05: ");
        Serial.println(serialNumberEDToSend05);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if
      else if (rxValue.find(serialNumberED06)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01) 
        {
          
          for (int i = SNED06; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice06  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED06, SNEndDevice06);
        
        serialNumberEDToSend06  = SNEndDevice06.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice06); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED06! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED06: ");
        Serial.println(SNEndDevice06);  
        Serial.print("SERIAL NUMBER ED06: ");
        Serial.println(serialNumberEDToSend06);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if
      else if (rxValue.find(serialNumberED07)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED07; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice07  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED07, SNEndDevice07);
        
        serialNumberEDToSend07  = SNEndDevice07.toInt();    
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice07); 
        delay(2000*timeToWait);         
        
        #ifdef DEBUG
        Serial.print("GAT-ED07! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED07: ");
        Serial.println(SNEndDevice07);        
        Serial.print("SERIAL NUMBER ED07: ");
        Serial.println(serialNumberEDToSend07);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if
      else if (rxValue.find(serialNumberED08)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED08; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice08  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED08, SNEndDevice08);
        
        serialNumberEDToSend08  = SNEndDevice08.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice08); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED08! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED08: ");
        Serial.println(SNEndDevice08);    
        Serial.print("SERIAL NUMBER ED08: ");
        Serial.println(serialNumberEDToSend08);           
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if      
      // Extra
      else if (rxValue.find(serialNumberED09)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED09; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice09  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED09, SNEndDevice09);
        
        serialNumberEDToSend09  = SNEndDevice09.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice09); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED09! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED09: ");
        Serial.println(SNEndDevice09);    
        Serial.print("SERIAL NUMBER ED09: ");
        Serial.println(serialNumberEDToSend09);           
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if      
      else if (rxValue.find(serialNumberED10)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED10; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice10  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED10, SNEndDevice10);
        
        serialNumberEDToSend10  = SNEndDevice10.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice10); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED10! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED10: ");
        Serial.println(SNEndDevice10);    
        Serial.print("SERIAL NUMBER ED10: ");
        Serial.println(serialNumberEDToSend10);           
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if      
      else if (rxValue.find(serialNumberED11)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED11; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice11  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED11, SNEndDevice11);
        
        serialNumberEDToSend11  = SNEndDevice11.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice11); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED11! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED11: ");
        Serial.println(SNEndDevice11);    
        Serial.print("SERIAL NUMBER ED11: ");
        Serial.println(serialNumberEDToSend11);           
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if    
      else if (rxValue.find(serialNumberED12)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED12; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice12  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED12, SNEndDevice12);
        
        serialNumberEDToSend12  = SNEndDevice12.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice12); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED12! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED12: ");
        Serial.println(SNEndDevice12);    
        Serial.print("SERIAL NUMBER ED12: ");
        Serial.println(serialNumberEDToSend12);           
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if
      else if (rxValue.find(serialNumberED13)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED13; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice13  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED13, SNEndDevice13);
        
        serialNumberEDToSend13  = SNEndDevice13.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice13); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED13! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED13: ");
        Serial.println(SNEndDevice13);    
        Serial.print("SERIAL NUMBER ED13: ");
        Serial.println(serialNumberEDToSend13);           
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if
      else if (rxValue.find(serialNumberED14)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED14; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice14  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED14, SNEndDevice14);
        
        serialNumberEDToSend14  = SNEndDevice14.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice14); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED14! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED14: ");
        Serial.println(SNEndDevice14);    
        Serial.print("SERIAL NUMBER ED14: ");
        Serial.println(serialNumberEDToSend14);           
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if    
      else if (rxValue.find(serialNumberED15)     != -1 &&  (flagPasswordConfig))
      {
        
        if(EEPROM.read(flagUpdate) == 0x01) 
        {
          
          for (int i = SNED15; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice15  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED15, SNEndDevice15);
        
        serialNumberEDToSend15  = SNEndDevice15.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice15); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED15! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED15: ");
        Serial.println(SNEndDevice15);    
        Serial.print("SERIAL NUMBER ED15: ");
        Serial.println(serialNumberEDToSend15);           
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if  
      else if (rxValue.find(serialNumberED16)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = SNED16; i < 6; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String SNEndDevice16  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(SNED16, SNEndDevice16);
        
        serialNumberEDToSend16  = SNEndDevice16.toInt();  
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERIAL NUMBER OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SNEndDevice16); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-ED16! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER ED16: ");
        Serial.println(SNEndDevice16);    
        Serial.print("SERIAL NUMBER ED16: ");
        Serial.println(serialNumberEDToSend16);           
        Serial.println();
        delay(timeToWait);
        #endif  
  
      } // end else if  
  
      //**********************************************
      // Mode operate send time
      //**********************************************
      else if (rxValue.find(modeOperateSTime)     != -1 &&  (flagPasswordConfig))
      { 
      
        if(EEPROM.read(flagUpdate) == 0x01)
        {

          EEPROM.write(flagSendTime, 0x00); 
          EEPROM.commit();
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
      
        int find2Point      = valueToBLE.indexOf(':');
        String sendTimeToBLE  = valueToBLE.substring(find2Point+1);                 
        int valueSendTime   = sendTimeToBLE.toInt();
                
        if(valueSendTime    <=  zeroSample) valueSendTime = minSendTime;
        if(valueSendTime    > maxSample)  valueSendTime = maxSendTime;                
                
        EEPROM.write(flagSendTime,    valueSendTime); 
        EEPROM.commit();      
      
        interfaceSetupBLE();
        interfaceBLE();
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("TEMPO OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(valueSendTime); 
        delay(2000*timeToWait);       
      
        #ifdef DEBUG
        Serial.print("GAT-TE! - Value of BLE is: ");
        Serial.print(valueToBLE);       
        Serial.println();
        delay(timeToWait);
        #endif
    
        #ifdef DEBUG
        Serial.print("Value time is: ");
        Serial.print(EEPROM.read(flagSendTime));
        Serial.println();
        delay(timeToWait);
        #endif  
    
      } // end else if      
      
      //**********************************************
      // Mode server 01
      //**********************************************
      else if (rxValue.find(modeSendServer00)     != -1 &&  (flagPasswordConfig))
      {
        
        if(EEPROM.read(flagUpdate) == 0x01)
        {

          EEPROM.write(flagConfigBLE, 0x00);  
          EEPROM.commit();
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        char valueEEPROMConfigMode  = EEPROM.read(flagConfigBLE);
        valueEEPROMConfigMode   &=  (0x0F);
        valueEEPROMConfigMode   |=  (0xA0);
  
        EEPROM.write(flagConfigBLE,     valueEEPROMConfigMode); 
        EEPROM.commit();    
      
        interfaceSetupBLE();
        interfaceBLE();
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERVER 01 OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(modeSendServer00); 
        delay(2000*timeToWait);
      
        #ifdef DEBUG
        Serial.print("GAT-SERVER01! - Value of BLE is: ");
        Serial.print(valueToBLE);       
        Serial.println();
        delay(timeToWait);
        #endif
        
      } // end else if
  
      //**********************************************
      // Mode server 02 
      //**********************************************
      else if (rxValue.find(modeSendServer01)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {

          EEPROM.write(flagConfigBLE, 0x00);  
          EEPROM.commit();
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        char valueEEPROMConfigMode  = EEPROM.read(flagConfigBLE);
        valueEEPROMConfigMode   &=  (0x0F);
        valueEEPROMConfigMode   |=  (0xB0);
  
        EEPROM.write(flagConfigBLE,     valueEEPROMConfigMode); 
        EEPROM.commit();    
      
        interfaceSetupBLE();
        interfaceBLE();
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERVER 02 OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(modeSendServer01); 
        delay(2000*timeToWait);       
      
        #ifdef DEBUG
        Serial.print("GAT-SERVER02! - Value of BLE is: ");
        Serial.print(valueToBLE);       
        Serial.println();
        delay(timeToWait);
        #endif
        
      } // end else if
      
      //**********************************************
      // Mode server 03
      //**********************************************
      else if (rxValue.find(modeSendServer02)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01)
        {

          EEPROM.write(flagConfigBLE, 0x00);  
          EEPROM.commit();
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        char valueEEPROMConfigMode  = EEPROM.read(flagConfigBLE);
        valueEEPROMConfigMode   &=  (0xFF);
        valueEEPROMConfigMode   |=  (0xC0);
  
        EEPROM.write(flagConfigBLE,     valueEEPROMConfigMode); 
        EEPROM.commit();    
      
        interfaceSetupBLE();
        interfaceBLE();
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERVER 03 OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(modeSendServer02); 
        delay(2000*timeToWait);       
      
        #ifdef DEBUG
        Serial.print("GAT-SERVER03! - Value of BLE is: ");
        Serial.print(valueToBLE);       
        Serial.println();
        delay(timeToWait);
        #endif
        
      } // end else if      
  
      //**********************************************
      // Mode server 04
      //**********************************************
      else if (rxValue.find(modeSendServer03)     != -1 &&  (flagPasswordConfig))
      {
  
        if(EEPROM.read(flagUpdate) == 0x01) 
        {

          EEPROM.write(flagConfigBLE, 0x00);  
          EEPROM.commit();
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        char valueEEPROMConfigMode  = EEPROM.read(flagConfigBLE);
        valueEEPROMConfigMode   &=  (0xFF);
        valueEEPROMConfigMode   |=  (0xD0);
  
        EEPROM.write(flagConfigBLE,     valueEEPROMConfigMode); 
        EEPROM.commit();    
      
        interfaceSetupBLE();
        interfaceBLE();
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERVER 04 OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(modeSendServer03); 
        delay(2000*timeToWait);       
      
        #ifdef DEBUG
        Serial.print("GAT-SERVER04! - Value of BLE is: ");
        Serial.print(valueToBLE);       
        Serial.println();
        delay(timeToWait);
        #endif
        
      } // end else if        
      
      //**********************************************
      // Mode server 05
      //**********************************************
      else if (rxValue.find(modeSendServer04)     != -1 &&  (flagPasswordConfig))
      {
        
        if(EEPROM.read(flagUpdate) == 0x01)
        {

          EEPROM.write(flagConfigBLE, 0x00);  
          EEPROM.commit();
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        char valueEEPROMConfigMode  = EEPROM.read(flagConfigBLE);
        valueEEPROMConfigMode   &=  (0xFF);
        valueEEPROMConfigMode   |=  (0xE0);
  
        EEPROM.write(flagConfigBLE,     valueEEPROMConfigMode); 
        EEPROM.commit();    
      
        interfaceSetupBLE();
        interfaceBLE();
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SERVER 05 OK"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(modeSendServer03); 
        delay(2000*timeToWait);       
      
        #ifdef DEBUG
        Serial.print("GAT-SERVER05! - Value of BLE is: ");
        Serial.print(valueToBLE);       
        Serial.println();
        delay(timeToWait);
        #endif
        
      } // end else if                    
      
      //**********************************************
      // Mode SSID
      //**********************************************
      else if (rxValue.find(ssidBLE)          != -1 &&  (flagPasswordConfig))
      {
        
        if(EEPROM.read(flagUpdate) == 0x01)
        {
          
          for (int i = netSSID; i < 30; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
        
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point    = valueToBLE.indexOf(':');
        String SSIDNetwork  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(netSSID,  SSIDNetwork);
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("SSID:"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SSIDNetwork); 
        delay(2000*timeToWait);       
        
        #ifdef DEBUG
        Serial.print("GAT-SSID! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif  
  
        #ifdef DEBUG
        Serial.print("SSID: ");
        Serial.print(SSIDNetwork);          
        Serial.println();
        delay(timeToWait);
        #endif          
                  
      } // end else if
        
      //**********************************************  
      // Mode PASSWORD
      //**********************************************
      else if (rxValue.find(passwordBLE)        != -1 &&  (flagPasswordConfig))
      {
        
        if(EEPROM.read(flagUpdate) == 0x01) 
        {
          
          for (int i = netSSID; i < 30; i++)  
          {
              
            delay(10*timeToWait);
            
            EEPROM.write(i, 0x00);  
            EEPROM.commit();
            
          } // end for
          
          #ifdef  DEBUG
          Serial.println("Empty memory!");
          Serial.println();
          #endif
          
        } // end if
  
        interfaceSetupBLE();
        interfaceBLE();
        
        int find2Point      = valueToBLE.indexOf(':');
        String PasswordNetwork  = valueToBLE.substring(find2Point+1);
        
        writeStringToEEPROM(netPassword,  PasswordNetwork);
        
        flagOTA         = true;
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("PASSWORD:"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(PasswordNetwork); 
        delay(2000*timeToWait);         
  
        #ifdef DEBUG
        Serial.print("GAT-PASSWORD! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif      
  
        #ifdef DEBUG
        Serial.print("PASSWORD: ");
        Serial.print(PasswordNetwork);          
        Serial.println();
        delay(timeToWait);
        #endif              
                              
      } // end else if
      
      //**********************************************  
      // FINISH SETUP BLE
      //**********************************************      
      else if (rxValue.find(finishSetupBLE)       != -1 &&  (flagPasswordConfig))
      {
        
        tone(pinToBuzzer, NOTE_C4, 5000, 0);
        delay(5000*timeToWait);
        
        interfaceBLE();
        
        EEPROM.write(flagSetupEEPROM, 0x01);
        EEPROM.write(flagUpdate,    0x00);
        EEPROM.commit();
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("  CONFIGURACAO "); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print("   FINALIZADA  "); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-FINISH! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif      
      
        ESP.restart();
      
      } // end else if      

      //**********************************************
      // Enable SDCARD 
      //**********************************************  
      else if (rxValue.find(enableSDCard)       != -1)        
      {
        
        interfaceSetupBLE();
        interfaceBLE();       
        
        EEPROM.write(flagSDCard,  0x01);
        EEPROM.commit();
        
        #ifdef DEBUG
        Serial.print("GAT-SD-OK! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif          
        
      } // end if
      
      //**********************************************
      // Disable SDCARD 
      //**********************************************  
      else if (rxValue.find(disableSDCard)      != -1)        
      {
        
        interfaceSetupBLE();
        interfaceBLE();   

        EEPROM.write(flagSDCard,  0x00);
        EEPROM.commit();        
        
        #ifdef DEBUG
        Serial.print("GAT-SD-NAO-OK! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif          
              
      } // end if     
        
      //**********************************************
      // Update 
      //**********************************************  
      else if (rxValue.find(updateDevice)       != -1)        
      {
        
        #ifdef DEBUG
        Serial.print("GAT-UPDATE! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif          
        
        tone(pinToBuzzer, NOTE_C4, 5000, 0);
        delay(5000*timeToWait);
        
        interfaceBLE();       
        
        EEPROM.write(flagSetupEEPROM, 0x00);
        EEPROM.write(flagUpdate,    0x01);
        EEPROM.commit();
              
        ESP.restart();

      } // end if
        
      //**********************************************  
      // RESTART DEVICE
      //**********************************************          
      else if (rxValue.find(restartDevice)      != -1)
      {
        
        //for (int i = 0; i < EEPROM.length(); i++) 
        for (int i = 0; i < MAX_EEPROM; i++)  
        {
            
          delay(10*timeToWait);
          
          EEPROM.write(i, 0x00);  
          EEPROM.commit();
          
        } // end for  
        
        tone(pinToBuzzer, NOTE_C4, 3000, 0);
        
        interfaceBLE();
        
        EEPROM.write(flagSetupEEPROM, 0x00);
        EEPROM.commit();
        
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("     RESTART  "); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print("     GATEWAY  "); 
        delay(2000*timeToWait); 
        
        #ifdef DEBUG
        Serial.print("GAT-RESTART! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif      
      
        ESP.restart();
      
      } // end else if          
            
      //**********************************************  
      // CONSULT SERIAL NUMBER - VERIFICAR EM OUTRO MOMENTO                 
      //**********************************************  
      else if (rxValue.find(consultSNGateway)     != -1)
      {
        
        interfaceSetupBLE();
        interfaceBLE();
        
        #ifdef DEBUG
        Serial.print("SERIAL NUMBER E: - Value of BLE is: ");
        Serial.print(SERIAL_NUMBER_PRINT);          
        Serial.println();
        delay(timeToWait);
        #endif
  
        // Let's convert the value to a char array:
        char txString[16];                          // Make sure this is big enuffz.
        snprintf(txString, 7, "%d",SERIAL_NUMBER_PRINT);          // float_val, min_width, digits_after_decimal, char_buffer.
      
        characteristicTX->setValue(txString);                 // Set the value that send.       
        characteristicTX->notify();                     // Send the value of the smartphone.
      
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("CONSULTA SN:  "); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print(SERIAL_NUMBER_PRINT); 
        delay(2000*timeToWait);     
      
      } // end else if          
      
      //**********************************************
      // Mode OTA                           
      //**********************************************  
      /*
      else if (rxValue.find(modeOTA)        != -1 && (flagOTA))
      {
  
        digitalWrite(pinToBuzzer, HIGH);
        delay(3000*timeToWait);
        digitalWrite(pinToBuzzer, LOW);
        
        interfaceBLE();
        
        setupWifi();
          
        // Let's convert the value to a char array:
        char txString[16];                          // Make sure this is big enuffz.
        snprintf(txString, 20, "%s",IP_LOCAL);                // float_val, min_width, digits_after_decimal, char_buffer.
        
        characteristicTX->setValue(txString);                 // Set the value that send.       
        characteristicTX->notify();                     // Send the value of the smartphone.    
        
        EEPROM.write(flagConfigBLE, 0x04);
        EEPROM.commit();
        
        digitalWrite(pinToBuzzer, HIGH);
        delay(3000*timeToWait);
        digitalWrite(pinToBuzzer, LOW);
        
        ESP.restart();
        
        #ifdef DEBUG
        Serial.print("GAT-OTAJVTECH! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif        
  
      } // end else if        
      */
      
      else
      {
  
        interfaceSetupBLE();
        interfaceSetupBLE();
        interfaceSetupBLE();
        interfaceSetupBLE();
        interfaceSetupBLE();
  
        #ifdef DEBUG
        Serial.print("ENTRADA-ERRADA! - Value of BLE is: ");
        Serial.print(valueToBLE);         
        Serial.println();
        delay(timeToWait);
        #endif
  
        // Let's convert the value to a char array:
        char txString[16];                          // Make sure this is big enuffz.
        snprintf(txString, 20, "%s","ENTRADA-ERRADA");            // float_val, min_width, digits_after_decimal, char_buffer.
      
        characteristicTX->setValue(txString);                 // Set the value that send.       
        characteristicTX->notify();                     // Send the value of the smartphone.
  
        lcd.clear();
        lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
        lcd.print("ENTRADA ERRADA"); 
        lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
        lcd.print("ENTRADA ERRADA"); 
        delay(2000*timeToWait);
  
      } // end else       
        
      #ifdef  DEBUG
        Serial.print("*************************************************");
        Serial.println();
        delay(timeToWait);
        #endif
          
      //receiveValue = 0;
        
    } // end if
    
  }; // end void onWrite
  
}; // end class CharacteristicCallbacks

//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
// Initial settings
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************

void setup()
{
  
  setupSerial();

  setupPins();
  
  setupDisplay();
  
  setupRTC();
  
  setupSPI(); 
  
  // setupGPS();

  // setupEEPROM(); 
  
  // if(EEPROM.read(flagSDCard) ==  0x01) setupSDCard();
  
  setupLoRa();  
  
  // setupWifi(); 

  #ifdef  DEBUG
  Serial.println("Setup main ok! ");
  Serial.println();
  delay(timeToWait);  
  #endif  
  
  Serial.println(FIRMWARE_VERSION);
  Serial.println(DATA_UPDATE_FIRMWARE);
  Serial.println(MESSAGE_FIRMWARE);   
  Serial.print(MESSAGE_SERIAL_NUMBER);    
  Serial.println(SERIAL_NUMBER_PRINT);  
  
  lcd.clear();
  valueToDisplay = 0x09;
  showInTheDisplay();
    
} // end setup

//*****************************************************************************************************************
// Setup serial
//*****************************************************************************************************************

void setupSerial()
{
  
  Serial.begin(SERIAL);
  serialESP32.begin(SERIAL_ATMEGA);
  
  #ifdef  DEBUG
  Serial.println("Setup serial ok! ");
  Serial.println();
  delay(timeToWait);  
  #endif
  
  Serial.println(FIRMWARE_VERSION);
  Serial.println(DATA_UPDATE_FIRMWARE);
  Serial.println(MESSAGE_FIRMWARE);   
  Serial.print(MESSAGE_SERIAL_NUMBER);    
  Serial.println(SERIAL_NUMBER_PRINT);    
  
} // end setupSerial

//*****************************************************************************************************************
// Setup pins
//*****************************************************************************************************************

void setupPins()
{
  
  tone(pinToBuzzer, NOTE_C4, 100, 0);
  
  pinMode(pinToPushButton,      INPUT);
  pinMode(pinToCS1,         OUTPUT);
  pinMode(pinToBuzzer,        OUTPUT);
  pinMode(pinToLED,         OUTPUT);
  pinMode(pinToRST_GPS,       OUTPUT);
  pinMode(pinToGATE_GPS,        OUTPUT);

  digitalWrite(pinToCS1,          LOW);
  digitalWrite(pinToBuzzer,         LOW);
  digitalWrite(pinToLED,          LOW);
  digitalWrite(pinToRST_GPS,        LOW);
  digitalWrite(pinToGATE_GPS,       LOW);

  #ifdef  DEBUG
  Serial.println("Setup pins ok! ");
  Serial.println();
  delay(timeToWait);  
  #endif

} // end setupPins 

//*****************************************************************************************************************
// setup display
//*****************************************************************************************************************

void setupDisplay()
{
  
  // To scannig I2C - To LCD
  while(0)
  {
    
    #ifdef  DEBUG
    Serial.println ("Scanning I2C device...");
    #endif
    
    Wire.begin();
    
    for (byte i = 0; i <50; i++)
    {
    
    Wire.beginTransmission(i);
    
    if (Wire.endTransmission () == 0)
    {
      
      #ifdef  DEBUG
      Serial.print ("Address found->");
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")");
      #endif
      count++;
      
    } // end if
    
    #ifdef  DEBUG
    Serial.print ("Found ");
    Serial.print (count, DEC);
    Serial.println (" device");
    #endif
    
    } // end for
    
  } // end while  
  
  lcd.begin(screenWidth,  screenHeight);
  lcd.setBacklight(HIGH); 
  
  lcd.createChar(0, level01);
  lcd.createChar(1, level02);
  lcd.createChar(2, level03);
  lcd.createChar(3, level04);
  lcd.createChar(4, level05);
  lcd.createChar(5, level06);
  lcd.createChar(6, level07);
  lcd.createChar(7, level08);
  lcd.createChar(8, level09); 
  
  delay(1000*timeToWait);
  lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
  lcd.print(startGateway); 
  lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
  lcd.print(nameDevice); 
  delay(2000*timeToWait);
  lcd.clear();
  lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
  lcd.print(serialNumberToLCD); 
  lcd.setCursor(minScreenWidthPosition+4,maxScreenHeightPosition);
  lcd.print(SERIAL_NUMBER_PRINT); 
  delay(2000*timeToWait);
  lcd.clear();
  lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
  lcd.print(versionFirmwareName); 
  lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
  lcd.print(versionFirmware);   
  delay(2000*timeToWait); 
  lcd.clear();
  lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
  lcd.print(dataUpdateName); 
  lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
  lcd.print(dataUpdate);  
  delay(2000*timeToWait); 
  lcd.clear();
  lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
  lcd.print(waitToLCD); 
  lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
  lcd.print("  "); 
  delay(2000*timeToWait); 
  
  #ifdef  DEBUG
  Serial.print("Setup display ok! ");
  Serial.println();
  delay(timeToWait);
  #endif  
  
} // end setupDisplay

//*****************************************************************************************************************
// Setup RTC
//*****************************************************************************************************************

void setupRTC()
{
  
  rtc.begin();
  // rtc.setDate(23, 03, 14);
  // rtc.setTime(9, 23, 00);  
  
  readRTC();  
  
  lcd.clear();
  lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
  lcd.print(messageData);   
  
  // To hour
  if (hour < 10)
  {
    
    lcd.setCursor(minScreenWidthPosition+4,maxScreenHeightPosition);
    lcd.print("0");
    lcd.setCursor(minScreenWidthPosition+5,maxScreenHeightPosition);
    lcd.print(hour);
    
  } // end if 
  else
  {
  
    lcd.setCursor(minScreenWidthPosition+4,maxScreenHeightPosition);
    //lcd.print(hora);
    lcd.print(hour);
    
  } // end else
  lcd.setCursor(minScreenWidthPosition+6,maxScreenHeightPosition);
  lcd.print(":");
  
  // To minute
  if (minute < 10)
  {
    
    lcd.setCursor(minScreenWidthPosition+7,maxScreenHeightPosition);
    lcd.print("0");
    lcd.setCursor(minScreenWidthPosition+8,maxScreenHeightPosition);
    lcd.print(minute);
    
  } // end if
  else
  {
  
    lcd.setCursor(minScreenWidthPosition+7,maxScreenHeightPosition);
    lcd.print(minute);
    
  } // end else
  lcd.setCursor(minScreenWidthPosition+9,maxScreenHeightPosition);
  lcd.print(":");
  
  // To seconds
  if (second < 10)
  {
    
    lcd.setCursor(minScreenWidthPosition+10,maxScreenHeightPosition);
    lcd.print("0");
    lcd.setCursor(minScreenWidthPosition+11,maxScreenHeightPosition);
    lcd.print(second);
    
  } // end if
  else
  {
    
    lcd.setCursor(minScreenWidthPosition+10,maxScreenHeightPosition);
    lcd.print(second);
    
  } // end else     

  delay(2000*timeToWait);   
  lcd.clear();
  
  #ifdef  DEBUG
  Serial.print("Setup RTC ok! ");
  Serial.println();
  delay(timeToWait);
  #endif
  
} //  end setupRTC

//*****************************************************************************************************************
// Setup SPI
//*****************************************************************************************************************

void setupSPI()
{

  SPI.begin(SCK,MISO,MOSI,SS);                                    // Setup SPI  
  
  #ifdef  DEBUG
  Serial.print("Setup SPI ok! ");
  Serial.println();
  delay(timeToWait);
  #endif  
  
} // end setupSPI

//*****************************************************************************************************************
// Setup SDCARD
//*****************************************************************************************************************

void setupSDCard()
{
  
  if (!SD.begin(pinToCS1)) 
  {
  
    #ifdef  DEBUG
    Serial.println("Card Mount Failed");
    Serial.println();
    #endif
    return;
  
  } // end if
  
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE)
  {
  
    #ifdef  DEBUG
    Serial.println("No SD card attached");
    Serial.println();
    #endif
    return;
  
  } // end if
  
  #ifdef  DEBUG
  Serial.print("SD Card Type: ");
  Serial.println();
  #endif  
  
  if(cardType == CARD_MMC)
  {
  
    #ifdef  DEBUG
    Serial.println("MMC");
    Serial.println();
    #endif
  
  } // end else if
  
  else if(cardType == CARD_SD)
  {
  
    #ifdef  DEBUG
    Serial.println("SDSC");
    Serial.println();
    #endif
  
  } // end else if
  
  else if(cardType == CARD_SDHC)
  {
  
    #ifdef  DEBUG
    Serial.println("SDHC");
    Serial.println();
    #endif
  
  } // end else if
  
  else 
  {
      
    #ifdef  DEBUG
    Serial.println("UNKNOWN");
    Serial.println();
    #endif
    
  } // end else
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  
  #ifdef  DEBUG
  Serial.printf("SD Card Size: %lluMB\n", cardSize);  
  Serial.println();
  #endif
    
  File file = SD.open("/jvtech.txt");
  
  if(!file) 
  {
  
    #ifdef  DEBUG
    Serial.println("File doesn't exist");
    Serial.println("Creating file...");
    Serial.println();
    #endif
    
    writeFile(SD, "/jvtech.txt", "Data, Hour, sned, counter, data01, data02, data03, data04, data05, data06, data07, data08, data09, data10, data11, data12, data13, data14, data15, data16 \r\n");
  
  } // end if
  
  else 
  {
  
    #ifdef  DEBUG
    Serial.println("File already exists"); 
    Serial.println();
    #endif    
  
  } // end else
  
  file.close(); 
  
  #ifdef  DEBUG
  Serial.print("Setup SDCard ok! ");
  Serial.println();
  delay(timeToWait);
  #endif  
  
} //  end setupSDCard

//*****************************************************************************************************************
// Setup gps
//*****************************************************************************************************************

/*
void setupGPS()
{
  
  neogps.begin(SERIAL_GPS, SERIAL_8N1, pinToTX_GPS, pinToRX_GPS);
  
  #ifdef  DEBUG
  Serial.print("Setup GPS ok! ");
  Serial.println();
  delay(timeToWait);
  #endif
  
} // end setupGPS
*/

//*****************************************************************************************************************
// Function to write file
//*****************************************************************************************************************

void writeFile(fs::FS &fs, const char * path, const char * message) 
{
  
  #ifdef  DEBUG
  Serial.printf("Writing file: %s\n", path);
  Serial.println();
  #endif
  
  File file = fs.open(path, FILE_WRITE);
  
  if(!file) 
  {
  
    #ifdef  DEBUG
    Serial.println("Failed to open file for writing");
    Serial.println();
    #endif
    
    return;
  
  } // end if
  
  if(file.print(message)) 
  {
  
    #ifdef  DEBUG
    Serial.println("File written");
    Serial.println();
    #endif    
  
  } // end if
  
  else 
  {
  
    #ifdef  DEBUG
    Serial.println("Write failed");
    Serial.println();
    #endif
  
  } // end else
  
  file.close();

} // end writeFile

//*****************************************************************************************************************
// Function to append data
//*****************************************************************************************************************

void appendFile(fs::FS &fs, const char * path, const char * message) 
{

  #ifdef  DEBUG
  Serial.printf("Appending to file: %s\n", path);
  Serial.println();
  #endif

  File file = fs.open(path, FILE_APPEND);
  
  if(!file) 
  {
  
    #ifdef  DEBUG
    Serial.println("Failed to open file for appending");
    Serial.println();
    #endif
    
    return;
  
  } // end if
  
  if(file.print(message)) 
  {
  
    #ifdef  DEBUG
    Serial.println("Message appended");
    Serial.println();
    #endif
  
  } // end if
  
  else 
  {
  
    #ifdef  DEBUG
    Serial.println("Append failed");
    Serial.println();
    #endif
  
  } // end else
  
  file.close();

}  // end appendFile

//*****************************************************************************************************************
// Function to EEPROM
//*****************************************************************************************************************

void setupEEPROM()
{
  
  EEPROM.begin(EEPROM_SIZE);  
  
  /*
  if(EEPROM.read(flagConfigBLE) ==  0x04) setupOTA();
  else                    delay(timeToWait);
  */
  
  if(EEPROM.read(flagSetupEEPROM) ==  0x01)
  {
    
    #ifdef  DEBUG
    Serial.println("BLE OK! ");
    Serial.println();
    delay(1000*timeToWait);
    #endif  
    
    flagSetupBLE = true;
    setupBLE();
    lcd.clear();
    
    timeToWaitBoot = millis();
    
    while(millis() - timeToWaitBoot < timeToBoot)
    {
      
      digitalWrite(pinToLED,    HIGH);
      delay(1000*timeToWait);
      digitalWrite(pinToLED,    LOW);
      delay(1000*timeToWait);
      
      valueToDisplay = 0x00;
      showInTheDisplay();
      
    } // end while
    
    lcd.clear();
    flagSetupBLE = false;
    setupBLE();
    
    timeToSendWebNow    = EEPROM.read(flagSendTime)*fixTimeToSend;
    serialNumberGATToSend   =   readStringLikeInt(serialNumber, MAX_VALUE/5);
    serialNumberEDToSend01  = readStringLikeInt(SNED01,     MAX_VALUE/5);
    serialNumberEDToSend02  = readStringLikeInt(SNED02,     MAX_VALUE/5);
    serialNumberEDToSend03  = readStringLikeInt(SNED03,     MAX_VALUE/5);
    serialNumberEDToSend04  = readStringLikeInt(SNED04,     MAX_VALUE/5);
    serialNumberEDToSend05  = readStringLikeInt(SNED05,     MAX_VALUE/5);
    serialNumberEDToSend06  = readStringLikeInt(SNED06,     MAX_VALUE/5);
    serialNumberEDToSend07  = readStringLikeInt(SNED07,     MAX_VALUE/5);
    serialNumberEDToSend08  = readStringLikeInt(SNED08,     MAX_VALUE/5);
    // Extra
    serialNumberEDToSend09  = readStringLikeInt(SNED09,     MAX_VALUE/5);
    serialNumberEDToSend10  = readStringLikeInt(SNED10,     MAX_VALUE/5);
    serialNumberEDToSend11  = readStringLikeInt(SNED11,     MAX_VALUE/5);
    serialNumberEDToSend12  = readStringLikeInt(SNED12,     MAX_VALUE/5);
    serialNumberEDToSend13  = readStringLikeInt(SNED13,     MAX_VALUE/5);
    serialNumberEDToSend14  = readStringLikeInt(SNED14,     MAX_VALUE/5);
    serialNumberEDToSend15  = readStringLikeInt(SNED15,     MAX_VALUE/5);
    serialNumberEDToSend16  = readStringLikeInt(SNED16,     MAX_VALUE/5);
    
    readStringLikeChar(ssidEEPROM,        netSSID,    MAX_VALUE);
    readStringLikeChar(passwordEEPROM,      netPassword,  MAX_VALUE);   

    #ifdef  DEBUG
    Serial.print("Flag setup EEPROM: ");
    Serial.println(EEPROM.read(flagSetupEEPROM));
    Serial.print("Flag setup BLE: ");
    Serial.println(EEPROM.read(flagConfigBLE)); 
    Serial.print("Flag update BLE: ");
    Serial.println(EEPROM.read(flagUpdate));  
    Serial.print("Flag SDCARD BLE: ");
    Serial.println(EEPROM.read(flagSDCard));  
    Serial.print("Flag Extra 01 BLE: ");
    Serial.println(EEPROM.read(flagExtra01));     
    Serial.print("Flag Extra 02 BLE: ");
    Serial.println(EEPROM.read(flagExtra02));
    Serial.print("Flag Extra 03 BLE: ");
    Serial.println(EEPROM.read(flagExtra03));
    Serial.print("Flag Extra 04 BLE: ");
    Serial.println(EEPROM.read(flagExtra04));
    Serial.print("Flag Extra 05 BLE: ");
    Serial.println(EEPROM.read(flagExtra05));
    Serial.print("Flag Extra 06 BLE: ");
    Serial.println(EEPROM.read(flagExtra06));
    Serial.print("Flag Extra 07 BLE: ");
    Serial.println(EEPROM.read(flagExtra07));   
    Serial.print("Flag Extra 08 BLE: ");
    Serial.println(EEPROM.read(flagExtra08));   
    Serial.print("Send time 01: ");
    Serial.println(EEPROM.read(flagSendTime));    
    Serial.print("Send time 02: ");
    Serial.println(timeToSendWebNow);     
    Serial.print("SSID: ");
    Serial.println(ssidEEPROM); 
    Serial.print("PASSWORD: ");
    Serial.println(passwordEEPROM);       
    Serial.print("SERIAL NUMBER GATEWAY: ");
    Serial.println(serialNumberGATToSend);  
    Serial.print("SERIAL_NUMBER END-DEVICE01: ");
    Serial.println(serialNumberEDToSend01);
    Serial.print("SERIAL_NUMBER END-DEVICE02: ");
    Serial.println(serialNumberEDToSend02);
    Serial.print("SERIAL_NUMBER END-DEVICE03: ");
    Serial.println(serialNumberEDToSend03);
    Serial.print("SERIAL_NUMBER END-DEVICE04: ");
    Serial.println(serialNumberEDToSend04);
    Serial.print("SERIAL_NUMBER END-DEVICE05: ");
    Serial.println(serialNumberEDToSend05);
    Serial.print("SERIAL_NUMBER END-DEVICE06: ");
    Serial.println(serialNumberEDToSend06);
    Serial.print("SERIAL_NUMBER END-DEVICE07: ");
    Serial.println(serialNumberEDToSend07);
    Serial.print("SERIAL_NUMBER END-DEVICE08: ");
    Serial.println(serialNumberEDToSend08);
    // Extra
    Serial.print("SERIAL_NUMBER END-DEVICE09: ");
    Serial.println(serialNumberEDToSend09);
    Serial.print("SERIAL_NUMBER END-DEVICE10: ");
    Serial.println(serialNumberEDToSend10);
    Serial.print("SERIAL_NUMBER END-DEVICE11: ");
    Serial.println(serialNumberEDToSend11);
    Serial.print("SERIAL_NUMBER END-DEVICE12: ");
    Serial.println(serialNumberEDToSend12);
    Serial.print("SERIAL_NUMBER END-DEVICE13: ");
    Serial.println(serialNumberEDToSend13);
    Serial.print("SERIAL_NUMBER END-DEVICE14: ");
    Serial.println(serialNumberEDToSend14);
    Serial.print("SERIAL_NUMBER END-DEVICE15: ");
    Serial.println(serialNumberEDToSend15);
    Serial.print("SERIAL_NUMBER END-DEVICE16: ");
    Serial.println(serialNumberEDToSend16);   
    
    #endif      
    
    valueToDisplay = 0x01;
    showInTheDisplay();
    delay(2000*timeToWait);
    
  } // end if
  
  else
  {
            
    writeStringToEEPROM(serialNumber, SERIAL_NUMBER);
    
    flagSetupBLE = true;
    setupBLE();
    
    lcd.clear();
    
    #ifdef  DEBUG
    Serial.print("Value flag Update: ");
    Serial.println(EEPROM.read(flagUpdate));
    Serial.println();
    #endif
    
    while(waitSetupBLE)
    {
      
      digitalWrite(pinToLED,    HIGH);
      delay(1000*timeToWait);
      digitalWrite(pinToLED,    LOW);
      delay(1000*timeToWait);
      
      valueToDisplay = 0x02;
      showInTheDisplay();
      
    } // end if
    
    // Show the message that finish the connection in the BLE
    
  } // end else
  
  #ifdef  DEBUG
  Serial.println("Setup EEPROM ok! ");
  Serial.println();
  delay(timeToWait);
  #endif    
  
} // end setupEEPROM

//*****************************************************************************************************************
// Function to BLE
//*****************************************************************************************************************

void setupBLE()
{
  
  if(flagSetupBLE)
  { 
  
    // Criar rede BLE
    NimBLEDevice::init("JVTECH-GATEWAY");                       // Name of the device Bluetooth
  
    //WI_Address = macAddressNumber(NimBLEDevice::toString());
    //NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    //NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
  
    // Criar o servidor BLE
    pServer = NimBLEDevice::createServer();                   // Create a BLE server 
    pServer->setCallbacks(new ServerCallbacks());                 // Set the callback of the server
    
    // Criar a instância do BLE
    //NimBLEService* pWifiService = pServer->createService(WIFI_SERVICE_UUID);  
    NimBLEService* pWifiService = pServer->createService(SERVICE_UUID); 
    
    // Para enviar dados
    characteristicTX = pWifiService->createCharacteristic(CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY); // Create a BLE Characteristic to send the data
    
    // Para receber dados pelas classes de callback
    NimBLECharacteristic* pWifiSSIDCharacteristic = pWifiService->createCharacteristic(
                                                CHARACTERISTIC_UUID_RX,
                                                NIMBLE_PROPERTY::READ     |
                                                NIMBLE_PROPERTY::WRITE    |
                                                NIMBLE_PROPERTY::NOTIFY   |
                                                NIMBLE_PROPERTY::WRITE_NR   );  
  
    pWifiSSIDCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pWifiService->start();                            // Start the service
    
    // Para o advertiging do módulo BLE
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pWifiService->getUUID());
    pAdvertising->setScanResponse(true);
    pAdvertising->start();  
      
    #ifdef  DEBUG
    Serial.println("Waiting a client connection to notify...");
    Serial.println();
    delay(timeToWait);
    #endif    
  
    valueToDisplay = 0x03;
    showInTheDisplay();
    delay(2000*timeToWait);
  
  } // end if
  
  else
  {
    
    digitalWrite(pinToLED,  LOW);
    
    pServer->stopAdvertising();
    
    #ifdef  DEBUG
    Serial.println("Disconnect BLE ok! ");
    Serial.println();
    delay(timeToWait);
    #endif      
    
    valueToDisplay = 0x04;
    showInTheDisplay();
    delay(2000*timeToWait);
    
  } // end else
  
  #ifdef  DEBUG
  Serial.println("The setup BLE ok! ");
  Serial.println();
  delay(timeToWait);
  #endif  
  
} // end setupBLE

//*****************************************************************************************************************
// Setup OTA                                  // MUDAR A IMPLEMENTAÇÃO
//*****************************************************************************************************************

/*
void setupOTA()
{
  
  setupWifi();
  
  #ifdef  DEBUG 
    Serial.println("");
    Serial.print("Conectado a rede wi-fi ");
    Serial.println(ssidEEPROM);
    Serial.print("IP obtido: ");
    Serial.println(WiFi.localIP());
  delay(timeToWait);
  #endif
  
    // Usa MDNS para resolver o DNS
    if (!MDNS.begin(hostOTA)) 
    { 

        //http://esp32.local
    #ifdef  DEBUG 
        Serial.println("Erro ao configurar mDNS. O ESP32 vai reiniciar em 1s...");
        #endif
    
    delay(1000*timeToWait);
        ESP.restart();   
    
    } // end if
   
  #ifdef  DEBUG
    Serial.println("mDNS configurado e inicializado;");
  #endif
   
    // Configfura as páginas de login e upload de firmware OTA
    server.on("/", HTTP_GET, []() 
    {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", loginIndex);
    });
     
    server.on("/serverIndex", HTTP_GET, []() 
    {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
    });
   
    // Define tratamentos do update de firmware OTA 
    server.on("/update", HTTP_POST, []() 
    {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    }, []() {
        HTTPUpload& upload = server.upload();
         
        if (upload.status == UPLOAD_FILE_START) 
        {
            // Inicio do upload de firmware OTA
            Serial.printf("Update: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) 
                Update.printError(Serial);
        } 
        else if (upload.status == UPLOAD_FILE_WRITE) 
        {
            // Escrevendo firmware enviado na flash do ESP32
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) 
                Update.printError(Serial);      
        } 
        else if (upload.status == UPLOAD_FILE_END) 
        {
            // Final de upload 
            if (Update.end(true))             
                Serial.printf("Sucesso no update de firmware: %u\nReiniciando ESP32...\n", upload.totalSize);
            else
                Update.printError(Serial);
      
      EEPROM.write(flagConfigBLE, 0x00);
      EEPROM.commit();
      delay(40*timeToWait);
      
        }   
    });
    server.begin();   
  
  while(1)
  {
    
    server.handleClient();
    Serial.println("Wait OTA...");
    delay(timeToWait);    
  
  } // end while  
  
} // end setupOTA
*/

//***************************************************************************************************************** 
// Setup LoRa
//*****************************************************************************************************************

void setupLoRa()
{
        
  LoRa.setPins(SS,RST,DI00);                                      // Setup pins LoRa
  
  lcd.clear();
  
  if (!LoRa.begin(BAND_END_DEVICE1))
  {
    
    digitalWrite(pinToLED,  HIGH);
        
    counterReconnectSetupLoRa++;
    
    #ifdef  DEBUG
    Serial.print("Counter for reboot LoRa setup: ");            // Counter reboot force 
    Serial.println(counterReconnectSetupLoRa);                // Counter reboot force 
    Serial.println();
    delay(timeToWait);
    #endif  
        
    if(counterReconnectSetupLoRa == maxCounterReconnectSetupLoRa)
    {
      
      counterReconnectSetupLoRa = minCounterReconnectSetupLoRa;
      
      #ifdef  DEBUG
      Serial.println("Reboot force! ");                 // Reboot force 
      Serial.println();                   
      delay(timeToWait);
      #endif
      
      ESP.restart();                        // Reboot the chip 
      
    } // end if
  
    valueToDisplay = 0x07;
    showInTheDisplay();
  
  } // end if
  
  digitalWrite(pinToLED,  LOW);
  
  #ifdef  DEBUG
  Serial.print("LoRa initiated with success! ");
  Serial.println();
  delay(timeToWait);
  #endif

  LoRa.setSignalBandwidth(BW);
  LoRa.setSpreadingFactor(SF);
  LoRa.setTxPower(PA_Value, PA_SET);
  LoRa.setFrequency(BAND_END_DEVICE1);
  LoRa.setCodingRate4(CR);
  LoRa.enableCrc();
  
  valueToDisplay = 0x08;
  showInTheDisplay();
  delay(2000*timeToWait);
  
} // end setupLoRa

//*****************************************************************************************************************
// Setup Wifi
//*****************************************************************************************************************

void setupWifi()
{
  
  readStringLikeChar(ssidEEPROM, netSSID, MAX_VALUE);
  readStringLikeChar(passwordEEPROM, netPassword, MAX_VALUE); 
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidEEPROM, passwordEEPROM);                   // Begin the wifi 
  // WiFi.begin(SSID, PASSWORD);                          // Begin the wifi 
  
  #ifdef  DEBUG
  Serial.println("Started");
  Serial.println("Connecting...");
  Serial.println();
  delay(1000*timeToWait);
  #endif  
  
  lcd.clear();
  
  while (WiFi.status() != WL_CONNECTED)                     // Wait wifi to connect
  {
    
    digitalWrite(pinToLED,  HIGH);
    
    counterReconnectSetupWifi++;
    
    #ifdef  DEBUG
    Serial.print("Connecting WiFi...");                   // Wifi connecting
    Serial.println();                           // Wifi connecting
    Serial.print("Counter for reboot Wi-Fi: ");               // Counter reboot force 
    Serial.println(counterReconnectSetupWifi);                // Counter reboot force 
    Serial.println();   
    delay(100*timeToWait);
    #endif
    
    digitalWrite(pinToLED,  LOW);
    delay(100*timeToWait);
    
    if(counterReconnectSetupWifi == maxCounterReconnectSetupWifi)
    {
      
      counterReconnectSetupWifi = minCounterReconnectSetupWifi;
      
      #ifdef  DEBUG
      Serial.print("Reboot force! ");                 // Reboot force 
      Serial.println();                   
      delay(timeToWait);
      #endif
      
      ESP.restart();                            // Reboot the chip 
      
    } // end if   
    
    valueToDisplay = 0x05;
    showInTheDisplay();
    
  } // end while
  
  digitalWrite(pinToLED,    HIGH);
  
  //IP_LOCAL = String() + WiFi.localIP()[0] + “.” + WiFi.localIP()[1] + “.” + WiFi.localIP()[2] + “.” + WiFi.localIP()[3];  
  // snprintf (IP_LOCAL, MSG_BUFFER_SIZE, "%s", WiFi.localIP().toString().c_str());
  
  #ifdef  DEBUG
  Serial.println("Connected in Network WiFi");                // Wifi connected in Network
  Serial.print("IP obtained: ");
    Serial.println(WiFi.localIP());
  Serial.println();
  Serial.print("Setup Wifi ok! ");
  Serial.println(); 
  delay(timeToWait);  
  #endif
    
  valueToDisplay = 0x06;
  showInTheDisplay();
  delay(2000*timeToWait);
    
  client01.setInsecure(); 
  
  #ifdef  DEBUG
  Serial.print("Setup LoRa ok! ");
  Serial.println();
  delay(timeToWait);
  #endif    
    
  lcd.clear();
  valueToDisplay = 0x09;
  showInTheDisplay();   
    
} // end setupWifi

//*****************************************************************************************************************
// Write string to EEPROM
//*****************************************************************************************************************

void writeStringToEEPROM(char add, String data)
{
  
  int sizeString = data.length();

  for(int i = 0; i < sizeString; i++) EEPROM.write(add + i, data[i]);
  
  EEPROM.commit();
  
} // end writeStringToEEPROM

//*****************************************************************************************************************
// Read string like char
//*****************************************************************************************************************

void readStringLikeChar(char data[], char add, int maxValue)
{
  
  char value[maxValue];
  
  for (int i = 0; i < maxValue; i++)  value[i] = EEPROM.read(i + add);
  
  strncpy(data, value, maxValue);

} // end readStringLikeChar

//*****************************************************************************************************************
// Read string like int
//*****************************************************************************************************************

int readStringLikeInt(int add, int maxValue)
{
  
  String data = "";
  char pos;
  
  for (int i = 0; i < maxValue; i++)
  {
    
    pos   = EEPROM.read(i + add);
    data  = data + pos;
    
  } // end for  
  
  return  data.toInt();
  
} // end readStringLikeInt

//*****************************************************************************************************************
// Read string like string
//*****************************************************************************************************************
String readStringLikeString(int add, int maxValue)
{
  
  String valueString = "";
  char pos;
  
  for (int i = 0; i < maxValue; i++)
  {
    
    pos         = EEPROM.read(i + add);
    valueString     = valueString + pos;
    
  } // end for  
  
  return  valueString;
    
} // end readStringLikeString

//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
// Functions to interface
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************

//*****************************************************************************************************************
// Interface to setup BLE
//*****************************************************************************************************************

void interfaceSetupBLE()
{
  
  tone(pinToBuzzer, NOTE_C4, 100, 0);
  digitalWrite(pinToLED,    HIGH);
  delay(100*timeToWait);
  noTone(pinToBuzzer, 0);
  digitalWrite(pinToLED,    LOW);
  delay(100*timeToWait);  
  
} // end interfaceSetupBLE

//*****************************************************************************************************************
// Interface BLE
//*****************************************************************************************************************

void interfaceBLE()
{
  
  // Let's convert the value to a char array:
  char txString[11];                          // Make sure this is big enuffz.
  snprintf(txString, 20, "%s","ENTRADA-OK");            // float_val, min_width, digits_after_decimal, char_buffer.
  
  characteristicTX->setValue(txString);                 // Set the value that send.       
  characteristicTX->notify();                     // Send the value of the smartphone.
  
} // end interfaceBLE

//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
// End settings
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************

//*****************************************************************************************************************
// Loop infinite - TESTAR                       
//*****************************************************************************************************************

void loop()
{
  
  while(1)
  {
    
    if(Serial.available() > 0)
    {
    
      valueSerialNow = Serial.parseInt();
      
      if    (valueSerialNow ==  1)  
      {
        
        frequencyNow = BAND_END_DEVICE1;
        LoRa.setFrequency(BAND_END_DEVICE1);
      
      }
      
      else if (valueSerialNow ==  2)  
      {
        
        frequencyNow = BAND_END_DEVICE2;
        LoRa.setFrequency(BAND_END_DEVICE2);
      
      }
      
      else if (valueSerialNow ==  3)  
      {
        
        frequencyNow = BAND_END_DEVICE3;
        LoRa.setFrequency(BAND_END_DEVICE3);
        
      } 
      
      else if (valueSerialNow ==  4)  
      {
        
        frequencyNow = BAND_END_DEVICE4;
        LoRa.setFrequency(BAND_END_DEVICE4);
        
      } 
      
      else if (valueSerialNow ==  5)  
      {
        
        frequencyNow = BAND_END_DEVICE5;
        LoRa.setFrequency(BAND_END_DEVICE5);
        
      }
      
      else if (valueSerialNow ==  6)  
      {
        
        frequencyNow = BAND_END_DEVICE6;
        LoRa.setFrequency(BAND_END_DEVICE6);
        
      } 
      
      else if (valueSerialNow ==  7)  
      {
        
        frequencyNow = BAND_END_DEVICE7;
        LoRa.setFrequency(BAND_END_DEVICE7);
        
      } 
      
      else if (valueSerialNow ==  8)  
      {
        
        frequencyNow = BAND_END_DEVICE8;
        LoRa.setFrequency(BAND_END_DEVICE8);
        
      } 
      
      else if (valueSerialNow ==  9)
      {
        
        frequencyNow = BAND_END_DEVICE9;
        LoRa.setFrequency(BAND_END_DEVICE9);
        
      } 
      
      else if (valueSerialNow ==  10) 
      {
        
        frequencyNow = BAND_END_DEVICE10;
        LoRa.setFrequency(BAND_END_DEVICE10);
        
      } 
      
      else if (valueSerialNow ==  11) 
      {
        
        frequencyNow = BAND_END_DEVICE11;
        LoRa.setFrequency(BAND_END_DEVICE11);
        
      }
      
      else if (valueSerialNow ==  12) 
      {
        
        frequencyNow = BAND_END_DEVICE12;
        LoRa.setFrequency(BAND_END_DEVICE12);
        
      }
      
      else if (valueSerialNow ==  13) 
      { 
      
        frequencyNow = BAND_END_DEVICE13;
        LoRa.setFrequency(BAND_END_DEVICE13);
        
      } 
      
      else if (valueSerialNow ==  14) 
      {
        
        frequencyNow = BAND_END_DEVICE14;
        LoRa.setFrequency(BAND_END_DEVICE14);
        
      }
      
      Serial.println(valueSerialNow);
      
      lcd.clear();
      lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
      lcd.print("  FREQUENCIA:  "); 
      lcd.setCursor(minScreenWidthPosition+2,maxScreenHeightPosition);
      lcd.print(frequencyNow); 
      lcd.setCursor(minScreenWidthPosition+13,maxScreenHeightPosition);
      lcd.print("Hz"); 
    
    } // end if
    
    // counterFirst++;
    // counterSecond++;
    
    // if(counterFirst    ==  255)  counterFirst  = 0;
    // if(counterSecond   ==  512)  counterSecond = 256;
    
    LoRa.beginPacket();
    LoRa.write(10000);
    // LoRa.write(counterFirst);
    // LoRa.write(counterSecond);
    LoRa.endPacket();
    
    #ifdef  DEBUG_NO
    Serial.println("Send LoRa OK! ");
    Serial.print(counterFirst);
    Serial.print(" | ");
    Serial.println(counterSecond);
    delay(timeToWait);
    #endif
    
  } // end while
  
  timeToSendWeb = millis();
  
  while(millis() - timeToSendWeb  < timeToSendWebNow)
  {
    
    if(EEPROM.read(flagSDCard)  ==  0x01)
    {
      
      if  (WiFi.status() != WL_CONNECTED)
      {
      
        if(flagReceiveLoRaSDCard)
        {
        
          counterStatusWifi++;
          saveDataInSDCard();
          flagReceiveLoRaSDCard = false;
          
          if(counterStatusWifi == valueToResetDevice) ESP.restart();
          
        } // end if
      
      } // end if

    } // end if
    else
    {
        
      if  (WiFi.status() != WL_CONNECTED) setupWifi();
      
    } // end lese
    
    toVerifierWifi();
    
    // To receive LoRa
    onReceive(LoRa.parsePacket());
    
    // To reset
    functionPushButton();
      
  } // end while
  
  #ifdef  DEBUG_TIME
  timeBefore = millis();
  #endif  
  // To web
  while(flagReceiveLoRa)
  {
    
    valueToDisplay = 0x0C;
    showInTheDisplay();
    
    dataBaseWeb();
    
    if(flagReceiveLoRa)
    {
      
      char  valueServer   = EEPROM.read(flagConfigBLE);
      valueServer       &=  (0xF0);
            
      if    (valueServer  ==  0xA0)
      {
        
        sendToGS();
        sendToTAGO();       
        
      } // end if 
      else if (valueServer  ==  0xB0) 
      {
        
        sendToGS();
        
      } // end else if
      else if (valueServer  ==  0xC0) 
      {
        
        sendToTAGO();
        
      } // end else if
      else if (valueServer  ==  0xD0)
      {
        
        // Empty
        
      } // end else if      
      else if (valueServer  ==  0xE0)
      {
        
        // Empty
        
      } // end else if      
        
    } // end if

  } // end if 
  #ifdef  DEBUG_TIME
  timeAfter = millis(); 
  Serial.print("Time is: ");
  Serial.println(timeAfter-timeBefore);
  #endif
  
  valueToDisplay = 0x0B;
  showInTheDisplay(); 
  
  if(flagToSendLoRa)  sendMessageLoRa();
  
  timeToSendWeb = 0;
  
} // end loop

//*****************************************************************************************************************
// Function to read RTC
//*****************************************************************************************************************

void readRTC()
{
  
  rtc.getDate(year, month, day, weekday);
  rtc.getTime(hour, minute, second, period);
  if (!(second % 3)) rtc.setMode(1 - rtc.getMode());
  rtc.getTime(hour, minute, second, period);
  
  #ifdef  DEBUG_NO
  Serial.print(w[weekday - 1]);
  Serial.print("  ");
  Serial.print(day, DEC);
  Serial.print("/");
  Serial.print(m[month - 1]);
  Serial.print("/");
  Serial.print(year + 2000, DEC);
  Serial.print("  ");
  Serial.print(hour, DEC);
  Serial.print(":");
  Serial.print(minute, DEC);
  Serial.print(":");
  Serial.print(second, DEC);
  //Serial.print(rtc.getMode() ? (period ? " PM" : " AM") : "");
  Serial.println();
  #endif
  
  #ifdef  DEBUG_NO
  Serial.print("Read RTC ok! ");
  Serial.println();
  delay(timeToWait);
  #endif  

  if(period)
  {
  
    if(hour ==  1)  hour = 13;
    if(hour ==  2)  hour = 14;
    if(hour ==  3)  hour = 15;
    if(hour ==  4)  hour = 16;
    if(hour ==  5)  hour = 17;
    if(hour ==  6)  hour = 18;
    if(hour ==  7)  hour = 19;
    if(hour ==  8)  hour = 20;
    if(hour ==  9)  hour = 21;
    if(hour ==  10) hour = 22;
    if(hour ==  11) hour = 23;

  } // end if
  
} //  end readRTC

//*****************************************************************************************************************
// Function to push button
//*****************************************************************************************************************

void functionPushButton()
{
  
  //To push button
  if(!digitalRead (pinToPushButton))    flagToPushButton  = true;
  if(!digitalRead (pinToPushButton) &&    flagToPushButton)
  {
    
    while(!digitalRead(pinToPushButton))
    {
      
      digitalWrite(pinToLED,    HIGH);
      tone(pinToBuzzer, NOTE_C4, 100, 0);
      
      counterPushButton++;
      #ifdef  DEBUG
      Serial.print("COUNTER TO PUSH BUTTON: ");   
      Serial.println(counterPushButton);  
      #endif      
      delay(100*timeToWait);
      
      if  (counterPushButton  ==  valueToRestart) 
      {
        
        EEPROM.write(flagSetupEEPROM, 0x00);  
        EEPROM.commit();
        delay(100*timeToWait);
        ESP.restart();
        
      } // end if 

    } // end while
    
    dataGAT.snd_ed  = serialNumberEDToSend01;
    dataGAT.data01  = dataGAT.data01+100;
    
    if    (dataGAT.data02 == 0) dataGAT.data02 = 1;
    else if (dataGAT.data02 == 1) dataGAT.data02 = 0;   
    
    for(int i = 0; i < maxSendLoRa; i++)  sendMessageLoRa();

  } // end if
  
} // end functionPushButton

//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
// To LoRa
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************

//*****************************************************************************************************************
// Function to OnReceive LoRa - TESTAR
//*****************************************************************************************************************

void onReceive(int packetSize)
{

  if (packetSize == 0)          return;     
  
  dataED.snd_ed     = LoRa.read();
  LoRa.readBytes((uint8_t*)&dataED, sizeof(dataED));
  dataED.counterLoRa    = LoRa.read();
  
  valueToDisplay = 0x0A;
  showInTheDisplay();
  
  if((dataED.snd_ed !=  serialNumberEDToSend01))
  {
    
    if(dataED.snd_ed  !=  serialNumberEDToSend02)   
    {
      
      if(dataED.snd_ed  !=  serialNumberEDToSend03)   
      {
      
        if(dataED.snd_ed  !=  serialNumberEDToSend04)   
        {
      
          if(dataED.snd_ed  !=  serialNumberEDToSend05)   
          {
            
            if(dataED.snd_ed  !=  serialNumberEDToSend06)   
            {
              
              if(dataED.snd_ed  !=  serialNumberEDToSend07)   
              {
                
                if(dataED.snd_ed  !=  serialNumberEDToSend08) 
                {
                  
                  if(dataED.snd_ed  !=  serialNumberEDToSend09) 
                  {
                  
                    if(dataED.snd_ed  !=  serialNumberEDToSend10) 
                    {
                      
                      if(dataED.snd_ed  !=  serialNumberEDToSend11) 
                      {
                        
                        if(dataED.snd_ed  !=  serialNumberEDToSend12) 
                        {
                          
                          if(dataED.snd_ed  !=  serialNumberEDToSend13) 
                          {
                            
                            if(dataED.snd_ed  !=  serialNumberEDToSend14) 
                            {
                              
                              if(dataED.snd_ed  !=  serialNumberEDToSend15) 
                              {
                                
                                if(dataED.snd_ed  !=  serialNumberEDToSend16) 
                                {
                  
                                  dataED.snd_ed   = dataED0.snd_ed;
                                  dataED.data01   = dataED0.data01;
                                  dataED.data02   =   dataED0.data02;
                                  dataED.data03   =   dataED0.data03;
                                  dataED.data04   =   dataED0.data04;
                                  dataED.data05   =   dataED0.data05;
                                  dataED.data06   =   dataED0.data06;
                                  dataED.data07   =   dataED0.data07;
                                  dataED.data08   =   dataED0.data08;
                                  dataED.data09   =   dataED0.data09;
                                  dataED.data10   =   dataED0.data10;
                                  dataED.data11   =   dataED0.data11;
                                  dataED.data12   =   dataED0.data12;
                                  dataED.data13   =   dataED0.data13;
                                  dataED.data14   =   dataED0.data14;
                                  dataED.data15   =   dataED0.data15;
                                  dataED.data16   =   dataED0.data16; 
                                  dataED.counterLoRa  =   dataED0.counterLoRa;      
                                  return;
                                  
                                } // end if
                                else      valueDataBaseLoRa = 16;
                                
                              } // end if 
                              else        valueDataBaseLoRa = 15;
                              
                            } // end if
                            else          valueDataBaseLoRa = 14;
                            
                          } // end if
                          else            valueDataBaseLoRa = 13;
                          
                        } // end if
                        else              valueDataBaseLoRa = 12;
                        
                      } // end if
                      else                valueDataBaseLoRa = 11;
                      
                    } // end if
                    else                  valueDataBaseLoRa = 10;
                    
                  } // end if
                  else                    valueDataBaseLoRa = 9;
                  
                } // end if
                else                      valueDataBaseLoRa = 8;
                
              } // end if
              else                        valueDataBaseLoRa = 7;
              
            } // end if
            else                          valueDataBaseLoRa = 6;
            
          } // end if
          else                            valueDataBaseLoRa = 5;
          
        } // end if
        else                              valueDataBaseLoRa = 4;

      } // end if
      else                                valueDataBaseLoRa = 3;

    } // end if
    else                                  valueDataBaseLoRa = 2;
    
  } // end if
  else                                    valueDataBaseLoRa = 1;
      
  #ifdef  DEBUG
  Serial.println("************************************"); 
  Serial.println("************************************"); 
  Serial.println("RECEIVE MESSAGE LORA RIGHT! "); 
  Serial.print("Send message LoRa to ID: ");
  Serial.println(dataED.snd_ed);
  Serial.print("Temperature intern: ");
  Serial.println(dataED.data01);
  Serial.print("Pressure: ");
  Serial.println(dataED.data02);
  Serial.print("Altitude: ");
  Serial.println(dataED.data03);
  Serial.print("Temperature extern: ");
  Serial.println(dataED.data04);
  Serial.print("Humidity extern: ");
  Serial.println(dataED.data05);
  Serial.print("Lux: ");
  Serial.println(dataED.data06);
  Serial.print("Output: ");
  Serial.println(dataED.data07);
  Serial.print("Wind: ");
  Serial.println(dataED.data08);
  Serial.print("Analog 01: ");
  Serial.println(dataED.data09);
  Serial.print("Analog 02: ");
  Serial.println(dataED.data10);
  Serial.print("4-20mA 01: ");
  Serial.println(dataED.data11);
  Serial.print("4-20mA 02: ");
  Serial.println(dataED.data12);
  Serial.print("4-20mA 03: ");
  Serial.println(dataED.data13);
  Serial.print("4-20mA 04: ");
  Serial.println(dataED.data14);
  Serial.print("Relay status: ");
  Serial.println(dataED.data15);
  Serial.print("RFID: ");
  Serial.println(dataED.data16);  
  Serial.print("Counter is: ");
  Serial.println(dataED.counterLoRa);     
  Serial.println("************************************"); 
  Serial.println("************************************"); 
  Serial.println();
  delay(timeToWait);
  #endif  
  
  #ifdef  DEBUG
  Serial.print("The value of the RSSI is: ");
  Serial.println(LoRa.packetRssi());
  Serial.print("The value of the SNR is: ");
  Serial.println(LoRa.packetSnr());
  Serial.print("Value of the database: ");
  Serial.println(valueDataBaseLoRa);  
  Serial.println();
  delay(timeToWait);
  #endif
  
  dataBaseLoRa();

  flagReceiveLoRa     = true;
  flagReceiveLoRaSDCard   = true;
  
  delay(250*timeToWait);
  valueToDisplay = 0x0B;
  showInTheDisplay();

  #ifdef  DEBUG
  Serial.println("Receive LoRa ok! ");
  Serial.println();
  delay(timeToWait);  
  #endif  
  
} // end onReceive

//*****************************************************************************************************************
// Send message LoRa - TESTAR                           
//*****************************************************************************************************************

void sendMessageLoRa()
{

  LoRa.beginPacket();
  LoRa.write(dataGAT.sng_ed);
  LoRa.write((uint8_t*)&dataGAT, sizeof(dataGAT));
  LoRa.endPacket(); 

  #ifdef  DEBUG
  Serial.println("************************************"); 
  Serial.println("SEND MESSAGE LORA");  
  Serial.print("Serial number GATEWAY: ");
  Serial.println(dataGAT.sng_ed);
  Serial.print("Serial number END-DEVICE: ");
  Serial.println(dataGAT.snd_ed);
  Serial.print("Value RFID: ");
  Serial.println(dataGAT.data01);
  Serial.print("Status relay: ");
  Serial.println(dataGAT.data02);     
  Serial.println("************************************"); 
  Serial.println();
  delay(timeToWait);
  #endif  
  
  flagToSendLoRa = false;
  
  #ifdef  DEBUG
  Serial.println("Sent LoRa ok! ");
  Serial.println();
  delay(timeToWait);  
  #endif  
      
} // end sendMessageLoRa

//*****************************************************************************************************************
// Data base LoRa - TESTAR                          
//*****************************************************************************************************************

void dataBaseLoRa()
{
  
  if    (valueDataBaseLoRa  ==  1)  
  {
    
    dataED1.snd_ed      = dataED.snd_ed;
    dataED1.data01      = dataED.data01;
    dataED1.data02      =   dataED.data02;
    dataED1.data03      =   dataED.data03;
    dataED1.data04      =   dataED.data04;
    dataED1.data05      =   dataED.data05;
    dataED1.data06      =   dataED.data06;
    dataED1.data07      =   dataED.data07;
    dataED1.data08      =   dataED.data08;
    dataED1.data09      =   dataED.data09;
    dataED1.data10      =   dataED.data10;
    dataED1.data11      =   dataED.data11;
    dataED1.data12      =   dataED.data12;
    dataED1.data13      =   dataED.data13;
    dataED1.data14      =   dataED.data14;
    dataED1.data15      =   dataED.data15;
    dataED1.data16      =   dataED.data16;  
    dataED1.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa1      = LoRa.packetRssi();
    valueSNRLoRa1       = LoRa.packetSnr();
    
    flagToDadaBase01    = true;
    valueToSendLoRaDataBase = 1;
  
  } // end if
  else if (valueDataBaseLoRa  ==  2)
  {

    dataED2.snd_ed      = dataED.snd_ed;
    dataED2.data01      = dataED.data01;
    dataED2.data02      =   dataED.data02;
    dataED2.data03      =   dataED.data03;
    dataED2.data04      =   dataED.data04;
    dataED2.data05      =   dataED.data05;
    dataED2.data06      =   dataED.data06;
    dataED2.data07      =   dataED.data07;
    dataED2.data08      =   dataED.data08;
    dataED2.data09      =   dataED.data09;
    dataED2.data10      =   dataED.data10;
    dataED2.data11      =   dataED.data11;
    dataED2.data12      =   dataED.data12;
    dataED2.data13      =   dataED.data13;
    dataED2.data14      =   dataED.data14;
    dataED2.data15      =   dataED.data15;
    dataED2.data16      =   dataED.data16;  
    dataED2.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa2      = LoRa.packetRssi();
    valueSNRLoRa2       = LoRa.packetSnr(); 

    flagToDadaBase02    = true; 
    valueToSendLoRaDataBase = 2;    

  } // end else if      
  else if (valueDataBaseLoRa  ==  3)
  {

    dataED3.snd_ed      = dataED.snd_ed;
    dataED3.data01      = dataED.data01;
    dataED3.data02      =   dataED.data02;
    dataED3.data03      =   dataED.data03;
    dataED3.data04      =   dataED.data04;
    dataED3.data05      =   dataED.data05;
    dataED3.data06      =   dataED.data06;
    dataED3.data07      =   dataED.data07;
    dataED3.data08      =   dataED.data08;
    dataED3.data09      =   dataED.data09;
    dataED3.data10      =   dataED.data10;
    dataED3.data11      =   dataED.data11;
    dataED3.data12      =   dataED.data12;
    dataED3.data13      =   dataED.data13;
    dataED3.data14      =   dataED.data14;
    dataED3.data15      =   dataED.data15;
    dataED3.data16      =   dataED.data16;  
    dataED3.counterLoRa   =   dataED.counterLoRa;

    valueRSSILoRa3      = LoRa.packetRssi();
    valueSNRLoRa3       = LoRa.packetSnr();
    
    flagToDadaBase03    = true;
    valueToSendLoRaDataBase = 3;

  } // end else if
  else if (valueDataBaseLoRa  ==  4)
  {

    dataED4.snd_ed      = dataED.snd_ed;
    dataED4.data01      = dataED.data01;
    dataED4.data02      =   dataED.data02;
    dataED4.data03      =   dataED.data03;
    dataED4.data04      =   dataED.data04;
    dataED4.data05      =   dataED.data05;
    dataED4.data06      =   dataED.data06;
    dataED4.data07      =   dataED.data07;
    dataED4.data08      =   dataED.data08;
    dataED4.data09      =   dataED.data09;
    dataED4.data10      =   dataED.data10;
    dataED4.data11      =   dataED.data11;
    dataED4.data12      =   dataED.data12;
    dataED4.data13      =   dataED.data13;
    dataED4.data14      =   dataED.data14;
    dataED4.data15      =   dataED.data15;
    dataED4.data16      =   dataED.data16;  
    dataED4.counterLoRa   =   dataED.counterLoRa;

    valueRSSILoRa4      = LoRa.packetRssi();
    valueSNRLoRa4       = LoRa.packetSnr();
    
    flagToDadaBase04    = true; 
    valueToSendLoRaDataBase = 4;

  } // end else if    
  else if (valueDataBaseLoRa  ==  5)
  {

    dataED5.snd_ed      = dataED.snd_ed;
    dataED5.data01      = dataED.data01;
    dataED5.data02      =   dataED.data02;
    dataED5.data03      =   dataED.data03;
    dataED5.data04      =   dataED.data04;
    dataED5.data05      =   dataED.data05;
    dataED5.data06      =   dataED.data06;
    dataED5.data07      =   dataED.data07;
    dataED5.data08      =   dataED.data08;
    dataED5.data09      =   dataED.data09;
    dataED5.data10      =   dataED.data10;
    dataED5.data11      =   dataED.data11;
    dataED5.data12      =   dataED.data12;
    dataED5.data13      =   dataED.data13;
    dataED5.data14      =   dataED.data14;
    dataED5.data15      =   dataED.data15;
    dataED5.data16      =   dataED.data16;  
    dataED5.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa5      = LoRa.packetRssi();
    valueSNRLoRa5       = LoRa.packetSnr(); 

    flagToDadaBase05    = true; 
    valueToSendLoRaDataBase = 5;

  } // end else if    
  else if (valueDataBaseLoRa  ==  6)
  {

    dataED6.snd_ed      = dataED.snd_ed;
    dataED6.data01      = dataED.data01;
    dataED6.data02      =   dataED.data02;
    dataED6.data03      =   dataED.data03;
    dataED6.data04      =   dataED.data04;
    dataED6.data05      =   dataED.data05;
    dataED6.data06      =   dataED.data06;
    dataED6.data07      =   dataED.data07;
    dataED6.data08      =   dataED.data08;
    dataED6.data09      =   dataED.data09;
    dataED6.data10      =   dataED.data10;
    dataED6.data11      =   dataED.data11;
    dataED6.data12      =   dataED.data12;
    dataED6.data13      =   dataED.data13;
    dataED6.data14      =   dataED.data14;
    dataED6.data15      =   dataED.data15;
    dataED6.data16      =   dataED.data16;  
    dataED6.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa6      = LoRa.packetRssi();
    valueSNRLoRa6       = LoRa.packetSnr();   

    flagToDadaBase06    = true;
    valueToSendLoRaDataBase = 6;

  } // end else if  
  else if (valueDataBaseLoRa  ==  7)
  {

    dataED7.snd_ed      = dataED.snd_ed;
    dataED7.data01      = dataED.data01;
    dataED7.data02      =   dataED.data02;
    dataED7.data03      =   dataED.data03;
    dataED7.data04      =   dataED.data04;
    dataED7.data05      =   dataED.data05;
    dataED7.data06      =   dataED.data06;
    dataED7.data07      =   dataED.data07;
    dataED7.data08      =   dataED.data08;
    dataED7.data09      =   dataED.data09;
    dataED7.data10      =   dataED.data10;
    dataED7.data11      =   dataED.data11;
    dataED7.data12      =   dataED.data12;
    dataED7.data13      =   dataED.data13;
    dataED7.data14      =   dataED.data14;
    dataED7.data15      =   dataED.data15;
    dataED7.data16      =   dataED.data16;  
    dataED7.counterLoRa   =   dataED.counterLoRa;

    valueRSSILoRa7      = LoRa.packetRssi();
    valueSNRLoRa7       = LoRa.packetSnr();
    
    flagToDadaBase07    = true; 
    valueToSendLoRaDataBase = 7;

  } // end else if  
  else if (valueDataBaseLoRa  ==  8)
  {

    dataED8.snd_ed      = dataED.snd_ed;
    dataED8.data01      = dataED.data01;
    dataED8.data02      =   dataED.data02;
    dataED8.data03      =   dataED.data03;
    dataED8.data04      =   dataED.data04;
    dataED8.data05      =   dataED.data05;
    dataED8.data06      =   dataED.data06;
    dataED8.data07      =   dataED.data07;
    dataED8.data08      =   dataED.data08;
    dataED8.data09      =   dataED.data09;
    dataED8.data10      =   dataED.data10;
    dataED8.data11      =   dataED.data11;
    dataED8.data12      =   dataED.data12;
    dataED8.data13      =   dataED.data13;
    dataED8.data14      =   dataED.data14;
    dataED8.data15      =   dataED.data15;
    dataED8.data16      =   dataED.data16;  
    dataED8.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa8      = LoRa.packetRssi();
    valueSNRLoRa8       = LoRa.packetSnr(); 

    flagToDadaBase08    = true; 
    valueToSendLoRaDataBase = 8;

  } // end else if
  // Extra
  else if (valueDataBaseLoRa  ==  9)
  {

    dataED8.snd_ed      = dataED.snd_ed;
    dataED8.data01      = dataED.data01;
    dataED8.data02      =   dataED.data02;
    dataED8.data03      =   dataED.data03;
    dataED8.data04      =   dataED.data04;
    dataED8.data05      =   dataED.data05;
    dataED8.data06      =   dataED.data06;
    dataED8.data07      =   dataED.data07;
    dataED8.data08      =   dataED.data08;
    dataED8.data09      =   dataED.data09;
    dataED8.data10      =   dataED.data10;
    dataED8.data11      =   dataED.data11;
    dataED8.data12      =   dataED.data12;
    dataED8.data13      =   dataED.data13;
    dataED8.data14      =   dataED.data14;
    dataED8.data15      =   dataED.data15;
    dataED8.data16      =   dataED.data16;  
    dataED8.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa9      = LoRa.packetRssi();
    valueSNRLoRa9       = LoRa.packetSnr(); 

    flagToDadaBase09    = true; 
    valueToSendLoRaDataBase = 9;

  } // end else if
  else if (valueDataBaseLoRa  ==  10)
  {

    dataED8.snd_ed      = dataED.snd_ed;
    dataED8.data01      = dataED.data01;
    dataED8.data02      =   dataED.data02;
    dataED8.data03      =   dataED.data03;
    dataED8.data04      =   dataED.data04;
    dataED8.data05      =   dataED.data05;
    dataED8.data06      =   dataED.data06;
    dataED8.data07      =   dataED.data07;
    dataED8.data08      =   dataED.data08;
    dataED8.data09      =   dataED.data09;
    dataED8.data10      =   dataED.data10;
    dataED8.data11      =   dataED.data11;
    dataED8.data12      =   dataED.data12;
    dataED8.data13      =   dataED.data13;
    dataED8.data14      =   dataED.data14;
    dataED8.data15      =   dataED.data15;
    dataED8.data16      =   dataED.data16;  
    dataED8.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa10     = LoRa.packetRssi();
    valueSNRLoRa10      = LoRa.packetSnr(); 

    flagToDadaBase10    = true; 
    valueToSendLoRaDataBase = 10;

  } // end else if  
  else if (valueDataBaseLoRa  ==  11)
  {

    dataED8.snd_ed      = dataED.snd_ed;
    dataED8.data01      = dataED.data01;
    dataED8.data02      =   dataED.data02;
    dataED8.data03      =   dataED.data03;
    dataED8.data04      =   dataED.data04;
    dataED8.data05      =   dataED.data05;
    dataED8.data06      =   dataED.data06;
    dataED8.data07      =   dataED.data07;
    dataED8.data08      =   dataED.data08;
    dataED8.data09      =   dataED.data09;
    dataED8.data10      =   dataED.data10;
    dataED8.data11      =   dataED.data11;
    dataED8.data12      =   dataED.data12;
    dataED8.data13      =   dataED.data13;
    dataED8.data14      =   dataED.data14;
    dataED8.data15      =   dataED.data15;
    dataED8.data16      =   dataED.data16;  
    dataED8.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa11     = LoRa.packetRssi();
    valueSNRLoRa11      = LoRa.packetSnr(); 

    flagToDadaBase11    = true; 
    valueToSendLoRaDataBase = 11;

  } // end else if  
  else if (valueDataBaseLoRa  ==  12)
  {

    dataED8.snd_ed      = dataED.snd_ed;
    dataED8.data01      = dataED.data01;
    dataED8.data02      =   dataED.data02;
    dataED8.data03      =   dataED.data03;
    dataED8.data04      =   dataED.data04;
    dataED8.data05      =   dataED.data05;
    dataED8.data06      =   dataED.data06;
    dataED8.data07      =   dataED.data07;
    dataED8.data08      =   dataED.data08;
    dataED8.data09      =   dataED.data09;
    dataED8.data10      =   dataED.data10;
    dataED8.data11      =   dataED.data11;
    dataED8.data12      =   dataED.data12;
    dataED8.data13      =   dataED.data13;
    dataED8.data14      =   dataED.data14;
    dataED8.data15      =   dataED.data15;
    dataED8.data16      =   dataED.data16;  
    dataED8.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa12     = LoRa.packetRssi();
    valueSNRLoRa12      = LoRa.packetSnr(); 

    flagToDadaBase12    = true; 
    valueToSendLoRaDataBase = 12;

  } // end else if  
  else if (valueDataBaseLoRa  ==  13)
  {

    dataED8.snd_ed      = dataED.snd_ed;
    dataED8.data01      = dataED.data01;
    dataED8.data02      =   dataED.data02;
    dataED8.data03      =   dataED.data03;
    dataED8.data04      =   dataED.data04;
    dataED8.data05      =   dataED.data05;
    dataED8.data06      =   dataED.data06;
    dataED8.data07      =   dataED.data07;
    dataED8.data08      =   dataED.data08;
    dataED8.data09      =   dataED.data09;
    dataED8.data10      =   dataED.data10;
    dataED8.data11      =   dataED.data11;
    dataED8.data12      =   dataED.data12;
    dataED8.data13      =   dataED.data13;
    dataED8.data14      =   dataED.data14;
    dataED8.data15      =   dataED.data15;
    dataED8.data16      =   dataED.data16;  
    dataED8.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa13     = LoRa.packetRssi();
    valueSNRLoRa13      = LoRa.packetSnr(); 

    flagToDadaBase13    = true; 
    valueToSendLoRaDataBase = 13;

  } // end else if  
  else if (valueDataBaseLoRa  ==  14)
  {

    dataED8.snd_ed      = dataED.snd_ed;
    dataED8.data01      = dataED.data01;
    dataED8.data02      =   dataED.data02;
    dataED8.data03      =   dataED.data03;
    dataED8.data04      =   dataED.data04;
    dataED8.data05      =   dataED.data05;
    dataED8.data06      =   dataED.data06;
    dataED8.data07      =   dataED.data07;
    dataED8.data08      =   dataED.data08;
    dataED8.data09      =   dataED.data09;
    dataED8.data10      =   dataED.data10;
    dataED8.data11      =   dataED.data11;
    dataED8.data12      =   dataED.data12;
    dataED8.data13      =   dataED.data13;
    dataED8.data14      =   dataED.data14;
    dataED8.data15      =   dataED.data15;
    dataED8.data16      =   dataED.data16;  
    dataED8.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa14     = LoRa.packetRssi();
    valueSNRLoRa14      = LoRa.packetSnr(); 

    flagToDadaBase14    = true; 
    valueToSendLoRaDataBase = 14;

  } // end else if  
  else if (valueDataBaseLoRa  ==  15)
  {

    dataED8.snd_ed      = dataED.snd_ed;
    dataED8.data01      = dataED.data01;
    dataED8.data02      =   dataED.data02;
    dataED8.data03      =   dataED.data03;
    dataED8.data04      =   dataED.data04;
    dataED8.data05      =   dataED.data05;
    dataED8.data06      =   dataED.data06;
    dataED8.data07      =   dataED.data07;
    dataED8.data08      =   dataED.data08;
    dataED8.data09      =   dataED.data09;
    dataED8.data10      =   dataED.data10;
    dataED8.data11      =   dataED.data11;
    dataED8.data12      =   dataED.data12;
    dataED8.data13      =   dataED.data13;
    dataED8.data14      =   dataED.data14;
    dataED8.data15      =   dataED.data15;
    dataED8.data16      =   dataED.data16;  
    dataED8.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa15     = LoRa.packetRssi();
    valueSNRLoRa15      = LoRa.packetSnr(); 

    flagToDadaBase15    = true; 
    valueToSendLoRaDataBase = 15;

  } // end else if    
  else if (valueDataBaseLoRa  ==  16)
  {

    dataED8.snd_ed      = dataED.snd_ed;
    dataED8.data01      = dataED.data01;
    dataED8.data02      =   dataED.data02;
    dataED8.data03      =   dataED.data03;
    dataED8.data04      =   dataED.data04;
    dataED8.data05      =   dataED.data05;
    dataED8.data06      =   dataED.data06;
    dataED8.data07      =   dataED.data07;
    dataED8.data08      =   dataED.data08;
    dataED8.data09      =   dataED.data09;
    dataED8.data10      =   dataED.data10;
    dataED8.data11      =   dataED.data11;
    dataED8.data12      =   dataED.data12;
    dataED8.data13      =   dataED.data13;
    dataED8.data14      =   dataED.data14;
    dataED8.data15      =   dataED.data15;
    dataED8.data16      =   dataED.data16;  
    dataED8.counterLoRa   =   dataED.counterLoRa;
    
    valueRSSILoRa16     = LoRa.packetRssi();
    valueSNRLoRa16      = LoRa.packetSnr(); 

    flagToDadaBase16    = true; 
    valueToSendLoRaDataBase = 16;

  } // end else if  
  
  valueDataBaseLoRa = 0;  
  
} // end dataBaseLoRa

//*****************************************************************************************************************
// Data base Web - TESTAR - TESTAR COM DOIS DISPOSITIVOS                              
//*****************************************************************************************************************

void dataBaseWeb()
{
  
  if  (flagToDadaBase01)  
  {
    
    dataED.snd_ed     = dataED1.snd_ed;     
    dataED.data01     = dataED1.data01;     
    dataED.data02     = dataED1.data02;     
    dataED.data03     = dataED1.data03;     
    dataED.data04     = dataED1.data04;     
    dataED.data05     = dataED1.data05;     
    dataED.data06     = dataED1.data06;     
    dataED.data07     = dataED1.data07;     
    dataED.data08     = dataED1.data08;     
    dataED.data09     = dataED1.data09;   
    dataED.data10     = dataED1.data10;   
    dataED.data11     = dataED1.data11;   
    dataED.data12     = dataED1.data12;   
    dataED.data13     = dataED1.data13;   
    dataED.data14     = dataED1.data14;   
    dataED.data15     = dataED1.data15;   
    dataED.data16     = dataED1.data16;   
    dataED.counterLoRa    = dataED1.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa1;
    valueSNRLoRa        =   valueSNRLoRa1;
    
    flagToDadaBase01    = false;
    return;
  
  } // end if 
  if  (flagToDadaBase02)
  {

    dataED.snd_ed     = dataED2.snd_ed;     
    dataED.data01     = dataED2.data01;     
    dataED.data02     = dataED2.data02;     
    dataED.data03     = dataED2.data03;     
    dataED.data04     = dataED2.data04;     
    dataED.data05     = dataED2.data05;     
    dataED.data06     = dataED2.data06;     
    dataED.data07     = dataED2.data07;     
    dataED.data08     = dataED2.data08;     
    dataED.data09     = dataED2.data09;   
    dataED.data10     = dataED2.data10;   
    dataED.data11     = dataED2.data11;   
    dataED.data12     = dataED2.data12;   
    dataED.data13     = dataED2.data13;   
    dataED.data14     = dataED2.data14;   
    dataED.data15     = dataED2.data15;   
    dataED.data16     = dataED2.data16;   
    dataED.counterLoRa    = dataED2.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa2;
    valueSNRLoRa        =   valueSNRLoRa2;
    
    flagToDadaBase02    = false;
    return;

  } // end else if      
  if  (flagToDadaBase03)
  {

    dataED.snd_ed     = dataED3.snd_ed;     
    dataED.data01     = dataED3.data01;     
    dataED.data02     = dataED3.data02;     
    dataED.data03     = dataED3.data03;     
    dataED.data04     = dataED3.data04;     
    dataED.data05     = dataED3.data05;     
    dataED.data06     = dataED3.data06;     
    dataED.data07     = dataED3.data07;     
    dataED.data08     = dataED3.data08;     
    dataED.data09     = dataED3.data09;   
    dataED.data10     = dataED3.data10;   
    dataED.data11     = dataED3.data11;   
    dataED.data12     = dataED3.data12;   
    dataED.data13     = dataED3.data13;   
    dataED.data14     = dataED3.data14;   
    dataED.data15     = dataED3.data15;   
    dataED.data16     = dataED3.data16;   
    dataED.counterLoRa    = dataED3.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa3;
    valueSNRLoRa        =   valueSNRLoRa3;
    
    flagToDadaBase03    = false;
    return;

  } // end else if  
  if  (flagToDadaBase04)
  {

    dataED.snd_ed     = dataED4.snd_ed;     
    dataED.data01     = dataED4.data01;     
    dataED.data02     = dataED4.data02;     
    dataED.data03     = dataED4.data03;     
    dataED.data04     = dataED4.data04;     
    dataED.data05     = dataED4.data05;     
    dataED.data06     = dataED4.data06;     
    dataED.data07     = dataED4.data07;     
    dataED.data08     = dataED4.data08;     
    dataED.data09     = dataED4.data09;   
    dataED.data10     = dataED4.data10;   
    dataED.data11     = dataED4.data11;   
    dataED.data12     = dataED4.data12;   
    dataED.data13     = dataED4.data13;   
    dataED.data14     = dataED4.data14;   
    dataED.data15     = dataED4.data15;   
    dataED.data16     = dataED4.data16;   
    dataED.counterLoRa    = dataED4.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa4;
    valueSNRLoRa        =   valueSNRLoRa4;
    
    flagToDadaBase04    = false;
    return;

  } // end else if    
  if  (flagToDadaBase05)
  {

    dataED.snd_ed     = dataED5.snd_ed;     
    dataED.data01     = dataED5.data01;     
    dataED.data02     = dataED5.data02;     
    dataED.data03     = dataED5.data03;     
    dataED.data04     = dataED5.data04;     
    dataED.data05     = dataED5.data05;     
    dataED.data06     = dataED5.data06;     
    dataED.data07     = dataED5.data07;     
    dataED.data08     = dataED5.data08;     
    dataED.data09     = dataED5.data09;   
    dataED.data10     = dataED5.data10;   
    dataED.data11     = dataED5.data11;   
    dataED.data12     = dataED5.data12;   
    dataED.data13     = dataED5.data13;   
    dataED.data14     = dataED5.data14;   
    dataED.data15     = dataED5.data15;   
    dataED.data16     = dataED5.data16;   
    dataED.counterLoRa    = dataED5.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa5;
    valueSNRLoRa        =   valueSNRLoRa5;
    
    flagToDadaBase05    = false;
    return;

  } // end else if    
  if  (flagToDadaBase06)
  {

    dataED.snd_ed     = dataED6.snd_ed;     
    dataED.data01     = dataED6.data01;     
    dataED.data02     = dataED6.data02;     
    dataED.data03     = dataED6.data03;     
    dataED.data04     = dataED6.data04;     
    dataED.data05     = dataED6.data05;     
    dataED.data06     = dataED6.data06;     
    dataED.data07     = dataED6.data07;     
    dataED.data08     = dataED6.data08;     
    dataED.data09     = dataED6.data09;   
    dataED.data10     = dataED6.data10;   
    dataED.data11     = dataED6.data11;   
    dataED.data12     = dataED6.data12;   
    dataED.data13     = dataED6.data13;   
    dataED.data14     = dataED6.data14;   
    dataED.data15     = dataED6.data15;   
    dataED.data16     = dataED6.data16;   
    dataED.counterLoRa    = dataED6.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa6;
    valueSNRLoRa        =   valueSNRLoRa6;
    
    flagToDadaBase06    = false;
    return;

  } // end else if  
  if  (flagToDadaBase07)
  {

    dataED.snd_ed     = dataED7.snd_ed;     
    dataED.data01     = dataED7.data01;     
    dataED.data02     = dataED7.data02;     
    dataED.data03     = dataED7.data03;     
    dataED.data04     = dataED7.data04;     
    dataED.data05     = dataED7.data05;     
    dataED.data06     = dataED7.data06;     
    dataED.data07     = dataED7.data07;     
    dataED.data08     = dataED7.data08;     
    dataED.data09     = dataED7.data09;   
    dataED.data10     = dataED7.data10;   
    dataED.data11     = dataED7.data11;   
    dataED.data12     = dataED7.data12;   
    dataED.data13     = dataED7.data13;   
    dataED.data14     = dataED7.data14;   
    dataED.data15     = dataED7.data15;   
    dataED.data16     = dataED7.data16;   
    dataED.counterLoRa    = dataED7.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa7;
    valueSNRLoRa        =   valueSNRLoRa7;
    
    flagToDadaBase07    = false;
    return;

  } // end else if  
  if  (flagToDadaBase08)
  {

    dataED.snd_ed     = dataED8.snd_ed;     
    dataED.data01     = dataED8.data01;     
    dataED.data02     = dataED8.data02;     
    dataED.data03     = dataED8.data03;     
    dataED.data04     = dataED8.data04;     
    dataED.data05     = dataED8.data05;     
    dataED.data06     = dataED8.data06;     
    dataED.data07     = dataED8.data07;     
    dataED.data08     = dataED8.data08;     
    dataED.data09     = dataED8.data09;   
    dataED.data10     = dataED8.data10;   
    dataED.data11     = dataED8.data11;   
    dataED.data12     = dataED8.data12;   
    dataED.data13     = dataED8.data13;   
    dataED.data14     = dataED8.data14;   
    dataED.data15     = dataED8.data15;   
    dataED.data16     = dataED8.data16;   
    dataED.counterLoRa    = dataED8.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa8;
    valueSNRLoRa        =   valueSNRLoRa8;
    
    flagToDadaBase08    = false;
    return;

  } // end else if
  // Extra
  if  (flagToDadaBase09)
  {

    dataED.snd_ed     = dataED8.snd_ed;     
    dataED.data01     = dataED8.data01;     
    dataED.data02     = dataED8.data02;     
    dataED.data03     = dataED8.data03;     
    dataED.data04     = dataED8.data04;     
    dataED.data05     = dataED8.data05;     
    dataED.data06     = dataED8.data06;     
    dataED.data07     = dataED8.data07;     
    dataED.data08     = dataED8.data08;     
    dataED.data09     = dataED8.data09;   
    dataED.data10     = dataED8.data10;   
    dataED.data11     = dataED8.data11;   
    dataED.data12     = dataED8.data12;   
    dataED.data13     = dataED8.data13;   
    dataED.data14     = dataED8.data14;   
    dataED.data15     = dataED8.data15;   
    dataED.data16     = dataED8.data16;   
    dataED.counterLoRa    = dataED8.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa9;
    valueSNRLoRa        =   valueSNRLoRa9;
    
    flagToDadaBase09    = false;
    return;

  } // end else if  
  if  (flagToDadaBase10)
  {

    dataED.snd_ed     = dataED8.snd_ed;     
    dataED.data01     = dataED8.data01;     
    dataED.data02     = dataED8.data02;     
    dataED.data03     = dataED8.data03;     
    dataED.data04     = dataED8.data04;     
    dataED.data05     = dataED8.data05;     
    dataED.data06     = dataED8.data06;     
    dataED.data07     = dataED8.data07;     
    dataED.data08     = dataED8.data08;     
    dataED.data09     = dataED8.data09;   
    dataED.data10     = dataED8.data10;   
    dataED.data11     = dataED8.data11;   
    dataED.data12     = dataED8.data12;   
    dataED.data13     = dataED8.data13;   
    dataED.data14     = dataED8.data14;   
    dataED.data15     = dataED8.data15;   
    dataED.data16     = dataED8.data16;   
    dataED.counterLoRa    = dataED8.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa10;
    valueSNRLoRa        =   valueSNRLoRa10;
    
    flagToDadaBase10    = false;
    return;

  } // end else if  
  if  (flagToDadaBase11)
  {

    dataED.snd_ed     = dataED8.snd_ed;     
    dataED.data01     = dataED8.data01;     
    dataED.data02     = dataED8.data02;     
    dataED.data03     = dataED8.data03;     
    dataED.data04     = dataED8.data04;     
    dataED.data05     = dataED8.data05;     
    dataED.data06     = dataED8.data06;     
    dataED.data07     = dataED8.data07;     
    dataED.data08     = dataED8.data08;     
    dataED.data09     = dataED8.data09;   
    dataED.data10     = dataED8.data10;   
    dataED.data11     = dataED8.data11;   
    dataED.data12     = dataED8.data12;   
    dataED.data13     = dataED8.data13;   
    dataED.data14     = dataED8.data14;   
    dataED.data15     = dataED8.data15;   
    dataED.data16     = dataED8.data16;   
    dataED.counterLoRa    = dataED8.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa11;
    valueSNRLoRa        =   valueSNRLoRa11;
    
    flagToDadaBase11    = false;
    return;

  } // end else if  
  if  (flagToDadaBase12)
  {

    dataED.snd_ed     = dataED8.snd_ed;     
    dataED.data01     = dataED8.data01;     
    dataED.data02     = dataED8.data02;     
    dataED.data03     = dataED8.data03;     
    dataED.data04     = dataED8.data04;     
    dataED.data05     = dataED8.data05;     
    dataED.data06     = dataED8.data06;     
    dataED.data07     = dataED8.data07;     
    dataED.data08     = dataED8.data08;     
    dataED.data09     = dataED8.data09;   
    dataED.data10     = dataED8.data10;   
    dataED.data11     = dataED8.data11;   
    dataED.data12     = dataED8.data12;   
    dataED.data13     = dataED8.data13;   
    dataED.data14     = dataED8.data14;   
    dataED.data15     = dataED8.data15;   
    dataED.data16     = dataED8.data16;   
    dataED.counterLoRa    = dataED8.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa12;
    valueSNRLoRa        =   valueSNRLoRa12;
    
    flagToDadaBase12    = false;
    return;

  } // end else if  
  if  (flagToDadaBase13)
  {

    dataED.snd_ed     = dataED8.snd_ed;     
    dataED.data01     = dataED8.data01;     
    dataED.data02     = dataED8.data02;     
    dataED.data03     = dataED8.data03;     
    dataED.data04     = dataED8.data04;     
    dataED.data05     = dataED8.data05;     
    dataED.data06     = dataED8.data06;     
    dataED.data07     = dataED8.data07;     
    dataED.data08     = dataED8.data08;     
    dataED.data09     = dataED8.data09;   
    dataED.data10     = dataED8.data10;   
    dataED.data11     = dataED8.data11;   
    dataED.data12     = dataED8.data12;   
    dataED.data13     = dataED8.data13;   
    dataED.data14     = dataED8.data14;   
    dataED.data15     = dataED8.data15;   
    dataED.data16     = dataED8.data16;   
    dataED.counterLoRa    = dataED8.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa13;
    valueSNRLoRa        =   valueSNRLoRa13;
    
    flagToDadaBase13    = false;
    return;

  } // end else if  
  if  (flagToDadaBase14)
  {

    dataED.snd_ed     = dataED8.snd_ed;     
    dataED.data01     = dataED8.data01;     
    dataED.data02     = dataED8.data02;     
    dataED.data03     = dataED8.data03;     
    dataED.data04     = dataED8.data04;     
    dataED.data05     = dataED8.data05;     
    dataED.data06     = dataED8.data06;     
    dataED.data07     = dataED8.data07;     
    dataED.data08     = dataED8.data08;     
    dataED.data09     = dataED8.data09;   
    dataED.data10     = dataED8.data10;   
    dataED.data11     = dataED8.data11;   
    dataED.data12     = dataED8.data12;   
    dataED.data13     = dataED8.data13;   
    dataED.data14     = dataED8.data14;   
    dataED.data15     = dataED8.data15;   
    dataED.data16     = dataED8.data16;   
    dataED.counterLoRa    = dataED8.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa14;
    valueSNRLoRa        =   valueSNRLoRa14;
    
    flagToDadaBase14    = false;
    return;

  } // end else if  
  if  (flagToDadaBase15)
  {

    dataED.snd_ed     = dataED8.snd_ed;     
    dataED.data01     = dataED8.data01;     
    dataED.data02     = dataED8.data02;     
    dataED.data03     = dataED8.data03;     
    dataED.data04     = dataED8.data04;     
    dataED.data05     = dataED8.data05;     
    dataED.data06     = dataED8.data06;     
    dataED.data07     = dataED8.data07;     
    dataED.data08     = dataED8.data08;     
    dataED.data09     = dataED8.data09;   
    dataED.data10     = dataED8.data10;   
    dataED.data11     = dataED8.data11;   
    dataED.data12     = dataED8.data12;   
    dataED.data13     = dataED8.data13;   
    dataED.data14     = dataED8.data14;   
    dataED.data15     = dataED8.data15;   
    dataED.data16     = dataED8.data16;   
    dataED.counterLoRa    = dataED8.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa15;
    valueSNRLoRa        =   valueSNRLoRa15;
    
    flagToDadaBase15    = false;
    return;

  } // end else if
  if  (flagToDadaBase16)
  {

    dataED.snd_ed     = dataED8.snd_ed;     
    dataED.data01     = dataED8.data01;     
    dataED.data02     = dataED8.data02;     
    dataED.data03     = dataED8.data03;     
    dataED.data04     = dataED8.data04;     
    dataED.data05     = dataED8.data05;     
    dataED.data06     = dataED8.data06;     
    dataED.data07     = dataED8.data07;     
    dataED.data08     = dataED8.data08;     
    dataED.data09     = dataED8.data09;   
    dataED.data10     = dataED8.data10;   
    dataED.data11     = dataED8.data11;   
    dataED.data12     = dataED8.data12;   
    dataED.data13     = dataED8.data13;   
    dataED.data14     = dataED8.data14;   
    dataED.data15     = dataED8.data15;   
    dataED.data16     = dataED8.data16;   
    dataED.counterLoRa    = dataED8.counterLoRa;    
    
    valueRSSILoRa       =   valueRSSILoRa16;
    valueSNRLoRa        =   valueSNRLoRa16;
    
    flagToDadaBase16    = false;
    return;

  } // end else if  
  
  flagReceiveLoRa    = false;
  
} // end dataBaseWeb

//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
// To web
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************

//*****************************************************************************************************************
// Function to verifier Wifi
//*****************************************************************************************************************

void toVerifierWifi()
{
  
  valueRSSI       = 0;
  valueRSSI       = WiFi.RSSI();
    
  if(valueRSSI  < -100) valueRSSI = 0;
    
  #ifdef  DEBUG_NO
  Serial.print("Value RSSI is: ");
  Serial.println(valueRSSI);
  #endif
  
  if    (valueRSSI < -90)
  {
    
    lcd.createChar(8, level09);
    lcd.setCursor(13,1);        
    lcd.write((uint8_t)8);
    lcd.createChar(6, level07);
    lcd.setCursor(14,1);        
    lcd.write((uint8_t)6);
    lcd.createChar(6, level07);
    lcd.setCursor(15,1);        
    lcd.write((uint8_t)6);
    
    
  } // end if
  
  else if ((-90 < valueRSSI) && (valueRSSI < -80))
  {

    lcd.createChar(7, level08);
    lcd.setCursor(13,1);        
    lcd.write((uint8_t)7);
    lcd.createChar(6, level07);
    lcd.setCursor(14,1);        
    lcd.write((uint8_t)6);
    lcd.createChar(6, level07);
    lcd.setCursor(15,1);        
    lcd.write((uint8_t)6);

  } // end if 

  else if ((-80 < valueRSSI) && (valueRSSI < -70))
  {

    lcd.createChar(7, level08);
    lcd.setCursor(13,1);        
    lcd.write((uint8_t)7);
    lcd.createChar(5, level06);
    lcd.setCursor(14,1);        
    lcd.write((uint8_t)5);
    lcd.createChar(6, level07);
    lcd.setCursor(15,1);        
    lcd.write((uint8_t)6);

  } // end if   
  
  else if ((-70 < valueRSSI) && (valueRSSI < -60))
  {

    lcd.createChar(7, level08);
    lcd.setCursor(13,1);        
    lcd.write((uint8_t)7);
    lcd.createChar(4, level05);
    lcd.setCursor(14,1);        
    lcd.write((uint8_t)4);
    lcd.createChar(6, level07);
    lcd.setCursor(15,1);        
    lcd.write((uint8_t)6);

  } // end if 

  else if ((-60 < valueRSSI) && (valueRSSI < -50))
  {

    lcd.createChar(7, level08);
    lcd.setCursor(13,1);        
    lcd.write((uint8_t)7);
    lcd.createChar(3, level04);
    lcd.setCursor(14,1);        
    lcd.write((uint8_t)3);
    lcd.createChar(6, level07);
    lcd.setCursor(15,1);        
    lcd.write((uint8_t)6);

  } // end if   
  
  else if ((-50 < valueRSSI) && (valueRSSI < -40))
  {

    lcd.createChar(7, level08);
    lcd.setCursor(13,1);        
    lcd.write((uint8_t)7);
    lcd.createChar(3, level04);
    lcd.setCursor(14,1);        
    lcd.write((uint8_t)3);
    lcd.createChar(2, level03);
    lcd.setCursor(15,1);        
    lcd.write((uint8_t)2);

  } // end if   
  
  else if ((-40 < valueRSSI) && (valueRSSI < -30))
  {

    lcd.createChar(7, level08);
    lcd.setCursor(13,1);        
    lcd.write((uint8_t)7);
    lcd.createChar(3, level04);
    lcd.setCursor(14,1);        
    lcd.write((uint8_t)3);
    lcd.createChar(1, level02);
    lcd.setCursor(15,1);        
    lcd.write((uint8_t)1);

  } // end if   
  
  else if ((-30 < valueRSSI))
  {

    lcd.createChar(7, level08);
    lcd.setCursor(13,1);        
    lcd.write((uint8_t)7);
    lcd.createChar(3, level04);
    lcd.setCursor(14,1);        
    lcd.write((uint8_t)3);
    lcd.createChar(0, level01);
    lcd.setCursor(15,1);        
    lcd.write((uint8_t)0);

  } // end if   
      
} // end toVerifierWifi

//*****************************************************************************************************************
// Function to error HTTP
//*****************************************************************************************************************

void errorHTTP()
{
  
  if(httpResponseCode1  !=  200)
  {
    
    tone(pinToBuzzer, NOTE_C4, 1000, 0);
    
    #ifdef  DEBUG_NOW
    Serial.print("Erro -11. Let's start again! ");
    Serial.println();
    delay(1000*timeToWait);
    #endif

  } // end if
  
  if(httpResponseCode2  !=  200)
  {
    
    tone(pinToBuzzer, NOTE_C4, 1000, 0);
    
    #ifdef  DEBUG_NOW
    Serial.print("Erro -11. Let's start again! ");
    Serial.println();
    delay(1000*timeToWait);
    #endif

  } // end if 
  
} // end errorHTTP

//*****************************************************************************************************************
// Function to verifier response 1
//*****************************************************************************************************************

void toVerifierResponseHTTP1()
{
  
  int n = payload.length();
  char receiveValue[n+1];
  strcpy(receiveValue,payload.c_str());   
  
  if ((strcmp(receiveValue, "{\"status\":ok}")  == 0))
  {

    #ifdef  DEBUG_NOW
    Serial.print("The POST is right! ");
    Serial.println();
    delay(100*timeToWait);
    #endif

  } // end if     
  
} // end toVerifierResponseHTTP1

//*****************************************************************************************************************
// Function to verifier response 2
//*****************************************************************************************************************

void toVerifierResponseHTTP2()
{
  
  String payloadNow;
  payloadNow = payload;
  int n = payload.length();
  char receiveValue[n+1];
  strcpy(receiveValue,payload.c_str()); 

  JSONVar myObject  = JSON.parse(payloadNow); 
  
  #ifdef  DEBUG_NOW
    Serial.print("The value of the return is: ");
    Serial.println(payloadNow);
  #endif  
  
  if (JSON.typeof(myObject) == "undefined")                   // JSON.typeof(jsonVar) can be used to get the type of the var
  {
  
    #ifdef  DEBUG_NOW
    Serial.println("Parsing input failed!");
    #endif
    return;
  
  } // end if
  
  else
  {
    
    #ifdef  DEBUG_NOW
    Serial.print("JSON object = ");
    Serial.println(myObject);
    #endif
  
    JSONVar keys    = myObject.keys();                  // myObject.keys() can be used to get an array of all the keys in the object
    
    for (int i = 0; i < keys.length(); i++) 
    {
    
      JSONVar value       = myObject[keys[i]];
      
      #ifdef  DEBUG_NOW
      Serial.print(keys[i]);
      Serial.print(" = ");
      Serial.println(value);
      #endif
      
      dataReadingsArr[i]    = value;
    
    } // end for
    
    #ifdef  DEBUG_NOW
    Serial.print("1 = ");
    Serial.println(dataReadingsArr[0]);
    Serial.print("2 = ");
    Serial.println(dataReadingsArr[1]);
    Serial.print("3 = ");
    Serial.println(dataReadingsArr[2]); 
    Serial.print("4 = ");
    Serial.println(dataReadingsArr[3]);   
    Serial.print("5 = ");
    Serial.println(dataReadingsArr[4]);     
    #endif

  } // end else   
    
  if(dataReadingsArr[1] !=  String(SERIAL_NUMBER_PRINT))  return;
  
  dataGAT.snd_ed      = dataReadingsArr[2].toInt();
  dataGAT.data01      = dataReadingsArr[3].toInt();
  dataGAT.data02      = dataReadingsArr[3].toInt();
  
  flagToSendLoRa      = true;
  
  #ifdef  DEBUG_NOW
  Serial.println("************************************"); 
  Serial.println("SEND MESSAGE LORA");  
  Serial.print("Serial number GATEWAY: ");
  Serial.println(dataGAT.sng_ed);
  Serial.print("Serial number END-DEVICE: ");
  Serial.println(dataGAT.snd_ed);
  Serial.print("Value RFID: ");
  Serial.println(dataGAT.data01);
  Serial.print("Status relay: ");
  Serial.println(dataGAT.data02);     
  Serial.println("************************************"); 
  Serial.println();
  delay(timeToWait);
  #endif    
  
} // end toVerifierResponseHTTP2

//*****************************************************************************************************************
// Function to http request server
//*****************************************************************************************************************

/*
String httpGETRequest(const char* serverName) 
{

  HTTPClient http;
    
  http.begin(serverName2);                          // Your Domain name with URL path or IP address with path
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode  = http.GET();                   // Send HTTP POST request
  String payload      = "{}"; 
  
  if (httpResponseCode > 0) 
  {
  
    #ifdef  DEBUG
    Serial.print("HTTP Response code GET: ");
    Serial.println(httpResponseCode);
    #endif
    payload = http.getString();
  
  } // end if
  
  else 
  {
  
    #ifdef  DEBUG
    Serial.print("Error code GET: ");
    Serial.println(httpResponseCode);
    #endif
    
  } // end else
    
  http.end();                                 // Free resources
  
  return payload;

} // end httpGETRequest
*/

//*****************************************************************************************************************
// Function to Google Sheet             
//*****************************************************************************************************************

void sendToGS()
{
  
  #ifdef  DEBUG
  Serial.println("\nStarting connection to server...");
  #endif
  
  if (!client01.connect(serverGS, portGS))
  {
    
    #ifdef  DEBUG
    Serial.println("Connection failed!");
    #endif
  
  } // end if
  
  else
  {
  
    #ifdef  DEBUG
    Serial.println("Connected to server!");
    #endif
  
  } // end else   
  
  // Make a HTTP request:
  String Request  =   String("GET ")          + 
            "/macros/s/"          + 
            GScriptId             + 
            "/exec?"            + 
            "value1="             + String(dataED.counterLoRa)  + 
            "&value2="            + String(dataGAT.sng_ed)    + 
            "&value3="            + String(dataED.snd_ed)     + 
            "&value4="            + String(valueRSSILoRa)     + 
            "&value5="            + String(valueSNRLoRa)      + 
            "&value6="            + String(valueRSSI)       + 
            "&value7="            + String(dataED.data04)     + 
            "&value8="            + String(dataED.data05)     + 
            "&value9="            + String(dataED.data01)     + 
            "&value10="           + String(dataED.data06)     +             
            "&value11="           + String(dataED.data02)     + 
            "&value12="           + String(dataED.data03)     + 
            "&value13="           + String(dataED.data16)     + 
            "&value14="           + String(dataED.data15)     + 
            "&value15="           + String(dataED.data07)     + 
            "&value16="           + String(dataED.data08)     +       
            "&value17="           + String(dataED.data09)     + 
            "&value18="           + String(dataED.data10)     + 
            "&value19="           + String(dataED.data11)     + 
            "&value20="           + String(dataED.data12)     + 
            "&value21="           + String(dataED.data13)     + 
            "&value22="           + String(dataED.data14)     + 
            "&value23="           + String(timeToSendWebNow)    + 
            " HTTP/1.1\r\n"         +         
            "Host: script.google.com\r\n"   + 
            "User-Agent: ESP8266\r\n"     + 
            "Connection: close\r\n"     + 
            "\r\n\r\n";
  
  client01.println(Request);
  
  while (client01.connected())
  {
    
    String line = client01.readStringUntil('\n');
    
    if (line == "\r")
    {
      
      #ifdef  DEBUG
      Serial.println("headers received");
      #endif
      
      break;
    
    } // end if
    
  } // end while
  
  // if there are incoming bytes available from the server, read them and print them:
  while (client01.available())
  {
  
    char c = client01.read();
    #ifdef  DEBUG
    Serial.write(c);
    #endif
  
  } // end while
  
  client01.stop();  
  
  #ifdef  DEBUG
  Serial.println("Sent Google Sheets ok! ");
  Serial.println();
  delay(timeToWait);  
  #endif    
  
} // end sendToGS

//*****************************************************************************************************************
// Function to TAGO-IO                        
//*****************************************************************************************************************

void sendToTAGO()
{
  
  #ifdef  DEBUG
  Serial.println("Start TAGO-IO!");
  #endif
  
  if (client02.connect(serverTago, portTago)) 
  {
  
    #ifdef  DEBUG
    Serial.println("CONNECTED AT TAGO\n");
    #endif
    
    String postStr  = "";
    String postData = "variable=val1&value="+String(dataED.counterLoRa)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);
    
    postStr     = "";
    postStr     = "";
    postData    = "variable=val2&value="+String(dataGAT.sng_ed)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);

    postStr     = "";
    postStr     = "";
    postData    = "variable=val3&value="+String(dataED.snd_ed)+"\n";  
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);

    postStr     = "";
    postStr     = "";
    postData    = "variable=val4&value="+String(valueRSSILoRa)+"\n";  
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);
    
    postStr     = "";
    postStr     = "";
    postData    = "variable=val5&value="+String(valueSNRLoRa)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);    

    postStr     = "";
    postStr     = "";
    postData    = "variable=val6&value="+String(valueRSSI)+"\n";  
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);

    postStr     = "";
    postStr     = "";
    postData    = "variable=val7&value="+String(dataED.data04)+"\n";  
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);
    
    postStr     = "";
    postStr     = "";
    postData    = "variable=val8&value="+String(dataED.data05)+"\n";  
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);        
    
    postStr     = "";
    postStr     = "";
    postData    = "variable=val9&value="+String(dataED.data01)+"\n";  
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);    

    postStr     = "";
    postStr     = "";
    postData    = "variable=val10&value="+String(dataED.data06)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);    

    postStr     = "";
    postStr     = "";
    postData    = "variable=val11&value="+String(dataED.data02)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);  

    postStr     = "";
    postStr     = "";
    postData    = "variable=val12&value="+String(dataED.data03)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);
    
    postStr     = "";
    postStr     = "";
    postData    = "variable=val13&value="+String(dataED.data16)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);    
    
    postStr     = "";
    postStr     = "";
    postData    = "variable=val14&value="+String(dataED.data15)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);  

    postStr     = "";
    postStr     = "";
    postData    = "variable=val15&value="+String(dataED.data07)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);  

    postStr     = "";
    postStr     = "";
    postData    = "variable=val16&value="+String(dataED.data08)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);  

    postStr     = "";
    postStr     = "";
    postData    = "variable=val17&value="+String(dataED.data09)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);  

    postStr     = "";
    postStr     = "";
    postData    = "variable=val18&value="+String(dataED.data10)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);  

    postStr     = "";
    postStr     = "";
    postData    = "variable=val19&value="+String(dataED.data11)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);  

    postStr     = "";
    postStr     = "";
    postData    = "variable=val20&value="+String(dataED.data12)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);  

    postStr     = "";
    postStr     = "";
    postData    = "variable=val21&value="+String(dataED.data13)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);      
    
    postStr     = "";
    postStr     = "";
    postData    = "variable=val22&value="+String(dataED.data14)+"\n"; 
    postStr     = "POST /data HTTP/1.1\n";
    postStr     += "Host: api.tago.io\n";
    postStr     += "Device-Token: "+apiKey+"\n";
    postStr     += "_ssl: false\n";
    postStr     += "Content-Type: application/x-www-form-urlencoded\n";
    postStr     += "Content-Length: "+String(postData.length())+"\n";
    postStr     += "\n";
    postStr     += postData;
    client02.print(postStr);    
    
    #ifdef  DEBUG
    Serial.print("Post is: ");  
    Serial.print(postStr);
    Serial.println();  
    delay(timeToWait);
    #endif    
  
    unsigned long timeout = millis();
    
    while (client02.available() == 0) 
    {
    
      if (millis() - timeout > timeToTAGO_IO) 
      {
      
        #ifdef  DEBUG
        Serial.println(">>> client02 Timeout !");
        Serial.println();  
        delay(timeToWait);
        #endif        
        
        client02.stop();
        
        return;
      
      } // end if
      
    } // end while
  
    while(client02.available())
    {
      
      String line = client02.readStringUntil('\r');
      
      #ifdef  DEBUG
      Serial.print("String is: ");  
      Serial.print(line);
      Serial.println();  
      delay(timeToWait);
      #endif    
    
    } // end while
  
  } // end if
  
  client02.stop();  
  
  toSendServer = false;
  
  #ifdef  DEBUG
  Serial.println("Sent TAGO-IO ok! ");
  Serial.println();
  delay(timeToWait);  
  #endif    
  
} // end sendToTAGO

//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
// To display
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************

//*****************************************************************************************************************
// Function to show display
//*****************************************************************************************************************

void showInTheDisplay()
{

  if    (valueToDisplay ==  0x00)
  {
  
    lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
    lcd.print(setupBLEMessage); 
    lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
    lcd.print(waitToLCD2);  
    
  } // end if
  
  else if (valueToDisplay ==  0x01)
  {
    
    lcd.clear();
    lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
    lcd.print(finishBLEMessage); 
    lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
    lcd.print(waitToLCD);     
    
  } // end else if  
  
  else if (valueToDisplay ==  0x02)
  {
  
    lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
    lcd.print(setupBLEMessage2); 
    lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
    lcd.print(waitToLCD2);  
    
  } // end else if

  else if (valueToDisplay ==  0x03)
  {
    
    lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
    lcd.print(setupBLEMessage3); 
    lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
    lcd.print(waitToLCD);     
    
  } // end else if

  else if (valueToDisplay ==  0x04)
  {
  
    lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
    lcd.print(setupBLEMessage); 
    lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
    lcd.print(setupBLEMessage5);      
    
  } // end else if

  else if (valueToDisplay ==  0x05)
  {
    
    lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
    lcd.print(setupWifiMessage); 
    lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
    lcd.print(setupWifiMessage1); 
    
  } // end else if

  else if (valueToDisplay ==  0x06)
  {
    
    lcd.clear();
    lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
    lcd.print(setupWifiMessage2); 
    lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
    lcd.print(WiFi.localIP());    
    
  } // end else if

  else if (valueToDisplay ==  0x07)
  {

    lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
    lcd.print(setupLoRaMessage); 
    lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
    lcd.print(setupLoRaMessage1);     
    
  } // end else if

  else if (valueToDisplay ==  0x08)
  {
    
    lcd.clear();
    lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
    lcd.print(setupLoRaMessage2);     
    
  } // end else if
  
  else if (valueToDisplay ==  0x09)
  {
  
    lcd.setCursor(minScreenWidthPosition,minScreenHeightPosition); 
    lcd.print(operateMessage); 
    lcd.setCursor(minScreenWidthPosition,maxScreenHeightPosition);
    lcd.print(operateMessage1);     
    
  } // end else if

  else if (valueToDisplay ==  0x0A)
  {
  
    lcd.setCursor(minScreenWidthPosition+15,minScreenHeightPosition); 
    lcd.print("*");   
    
  } // end else if  
  
  else if (valueToDisplay ==  0x0B)
  {
  
    lcd.setCursor(minScreenWidthPosition+15,minScreenHeightPosition); 
    lcd.print(" ");   
    
  } // end else if  
  
  else if (valueToDisplay ==  0x0C)
  {
  
    lcd.setCursor(minScreenWidthPosition+15,minScreenHeightPosition); 
    lcd.print("#");   
    
  } // end else if  

  #ifdef  DEBUG_NO
  Serial.println("Show display ok! ");
  Serial.println();
  delay(timeToWait);  
  #endif    
  
} // end showInTheDisplay

//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
// To SDCard 
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************
//*****************************************************************************************************************

//*****************************************************************************************************************
// Function to save in SD Card                    
//*****************************************************************************************************************

void saveDataInSDCard()
{
  
  #ifdef  DEBUG
  Serial.println("Save data in SDCard! ");
  Serial.println();
  delay(timeToWait);  
  #endif  
  
} // end saveDataInSDCard
