/*
 * Moved to PUBLIC GIT Feb 2020 YAY!!!! finally
 * Simple LORA communications code for Arduino.  Toggle the master/slave (1/0) in the code and program one module as master and one as slave 
 * Add any more functions and code you like for your specific project
 * battery indicator on OLEDpower switch puts board in to low power LDO Vreg
 * 
 * Parts needed:
 * Feather https://amzn.to/2V9ZAvp
 * Feather OLED https://amzn.to/32d72HD
 * 
 * Notes-Random Info I will lose:
 * OLED draws 10 mA when in use
 * Button pins are A=9 (same as batt pin ugh) B=6, C=5
 * I2C add=0x3C
 * Compare string values https://www.arduino.cc/en/Tutorial/StringComparisonOperators
 * Antenna 915 MHz - 3 inches or 7.8 cm
 * Feather Pinouts: https://learn.adafruit.com/adafruit-feather-32u4-radio-with-rfm69hcw-module/pinouts
 * Power Management: https://learn.adafruit.com/adafruit-feather-32u4-radio-with-rfm69hcw-module/power-management
 * 
 * Release history
 * V1.0 
 * Renamed to version IDs as is first functional build with radios communicating
 * Beginning to add I/O functionality from here
 * 
 */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_FeatherOLED.h>
#include <SPI.h>
#include <RH_RF95.h>

//Toggle this to determine master or slave code to be used-------------------------------------
int mode = 0;  // Mode 0 is TX code   Mode 1 is RX code

String msg; // Eric added this to globalize the message from buffer received
int signalreceived; //Globalized to define rssi gone when not received
int count = 0;// debugging counter
Adafruit_FeatherOLED oled = Adafruit_FeatherOLED();
#define VBATPIN A9
/* for feather32u4 */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#define LED 13
#define RF95_FREQ 915.0 // Define LORA board freq
RH_RF95 rf95(RFM95_CS, RFM95_INT); //radio driver

 
void setup()
{
  Serial.begin(9600);
  delay(1000);  //This MUST be here and increased to 1000 from 100, or OLED wont init- no fricken clue why 
  oled.init();
  oled.setBatteryVisible(true);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  Serial.println("Feather LoRa TX Test!");
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
    }
  Serial.println("LoRa radio init OK!");
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}
int16_t packetnum = 0;  // packet counter, we increment per xmission  

void loop()
{
dooledstuff(); //read battery level and do OLED update
  if (mode == 0) {
     doradiotx(); //Run TX MASTER code
     }
   else
     {
      doradiorx(); //Run RX SLAVE Code  
     }
}

float getBatteryVoltage() {
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  return measuredvbat;
  }
  
void dooledstuff()
{
  oled.clearDisplay(); // clear the current count
  float battery = getBatteryVoltage();
   // update the battery icon
  oled.setBattery(battery);
  oled.renderBattery();
  oled.print("C: "); //print out to OLED
  oled.println(count);
  oled.print("RSSI: ");
     if (signalreceived >0) //This is toggled on a valid packet receive
     {
      oled.println(rf95.lastRssi(), DEC);
     }
     else
     {
     oled.println ("0"); 
     }
     
  oled.println (msg); //will dislay received msg OR defined error text below
  //oled.println("---");
  oled.display(); //update oled screen
  count++; //debugging counter only
}

//_______________________________MASTER_____________________________________

void doradiotx() //TX Module code here
{
  Serial.println("Sending to rf95_server");
  char radiopacket[20] = "TX Online"; //Send the text string I'm here!
  itoa(packetnum++, radiopacket+13, 10);
  Serial.print("Sending "); Serial.println(radiopacket);
  radiopacket[19] = 0;
  Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)radiopacket, 20);
  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
    // wait for a reply from another radio
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  Serial.println("Waiting for reply..."); delay(10);
  if (rf95.waitAvailableTimeout(1000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
      {
      Serial.print("Got reply: ");
      msg = ((char*)buf); //sore the message into msg string for OLED use
      Serial.println (msg);
      //Erics code to check message received to see if it is an alarm- if so- trigger alarm=1
      //Move this code in to receiver module to trigger flash/buzzer
        if (msg == "RX Online") 
          {
            Serial.println ("Alarm Text Detected!");
            digitalWrite(LED, HIGH);
          }
      //--------------------------End maching code
      signalreceived =1; //set flag so OLED code can display that connection is alive
      Serial.println (signalreceived); //debugging only
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);  
        }
    else
       {
      Serial.println("Receive failed");
      msg = "ReceiveFail"; //set message to display properly on oled for status
      signalreceived = 0; //set flag so OLED can show connection is bad
      digitalWrite(LED, LOW);
       }
  }
  else
  {
    Serial.println("No reply. Slave node is not online........");
    msg = "No Connection"; //to display connection is dead on oled
    signalreceived = 0; //set flag so OLED can show connection is dead
    digitalWrite(LED, LOW);
  }
  delay(1000);
}


//__________________________________SLAVE__________________________

void doradiorx()
{
  Serial.println("Sending to rf95_server");
    // Send a message to rf95_server
  char radiopacket[20] = "RX Online"; //Send string to indicate online and talking
  itoa(packetnum++, radiopacket+13, 10);
  Serial.print("Sending "); Serial.println(radiopacket);
  radiopacket[19] = 0;
  Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)radiopacket, 20);
  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  Serial.println("Waiting for reply..."); delay(10);
  if (rf95.waitAvailableTimeout(3000)) //Eric changed this from 1000 and it fixed the flashing loss of com!***********
   { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
      {
      Serial.print("Got reply: ");
      msg = ((char*)buf); //sore the message into msg string for OLED use
      Serial.println (msg);
      //Erics code to check message received to see if it is an alarm- if so- trigger alarm=1
      //Move this code in to receiver module to trigger flash/buzzer
        if (msg == "TX Online") 
          {
            Serial.println ("Alarm Text Detected!");
            digitalWrite(LED, HIGH);
          }
      //--------------------------End matching code
      signalreceived =1; //set flag so OLED code can display that connection is alive
      Serial.println (signalreceived); //debugging only
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);  
        }
    else
       {
      Serial.println("Receive failed");
      msg = "ReceiveFail"; //set message to display properly on oled for status
      signalreceived = 0; //set flag so OLED can show connection is bad
      digitalWrite(LED, LOW);
       }
  }
  else
  {
    Serial.println("No reply. Master node is not online........");
    msg = "No Connection"; //to display connection is dead on oled
    signalreceived = 0; //set flag so OLED can show connection is dead
    digitalWrite(LED, LOW);
  }
  //delay(1000);
}
