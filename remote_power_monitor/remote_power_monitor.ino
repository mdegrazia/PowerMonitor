
/***************************************************
 * Remote Power Monitoring System
 * Mari DeGrazia
 * arizona4n6@gmail.com
 ************************************************** */


/***************************************************
  This is an example for our Adafruit FONA Cellular Module

  Designed specifically to work with the Adafruit FONA
  ----> http://www.adafruit.com/products/1946
  ----> http://www.adafruit.com/products/1963
  ----> http://www.adafruit.com/products/2468
  ----> http://www.adafruit.com/products/2542

  These cellular modules use TTL Serial to communicate, 2 pins are
  required to interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include "Adafruit_FONA.h"
#include <Wire.h>
#include "Adafruit_HTU21DF.h"
#include <SoftwareSerial.h>

/*************************** FONA Pins ***********************************/

// Default pins for Feather 32u4 FONA
#define FONA_RX 9
#define FONA_TX 8
#define FONA_RST 4
#define FONA_RI 7

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
Adafruit_HTU21DF htu = Adafruit_HTU21DF();

/*****************************USER CONFIG *****************************************/

char ALLOWED_NUMBER[32] = "+15205551234"; //put in the phone number that will send/receive text messages.
int lowBatteryLevel = 30;  //do not go below 10%. Battery voltage is too low under 10% to connect to GSM network
int lowTempLevel = 37; //in Fahrenheit
float voltageThreshold = 3.2; //value may need to be adjusted higer or lower

/***********************************************************************************/

/***************************LED Backpack Display*************************************/
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
Adafruit_8x16minimatrix matrix = Adafruit_8x16minimatrix();

/*****************************Program Vars *****************************************/

#define DATA_PIN A5 //pin that reads the voltage
char replybuffer[255]; // this is a large buffer for replies
int vbat; //hold battery voltage
boolean powerOut = false;
boolean tempLow = false;
int count = 0;

//bitmaps for the LED screen
static const uint8_t PROGMEM
  
   timer_bmp[]=
   {B01111110,
    B01000010,
    B00100100,
    B00011000,
    B00011000,
    B00100100,
    B01000010,
    B01111110},
    sms1_bmp[]=
   {B00111111,
    B00110000,
    B00101000,
    B00100110,
    B00100001,
    B00100000,
    B00100000,
    B00111111},
    sms2_bmp[]={
    B11111100,
    B00001100,
    B00010100,
    B01100100,
    B10000100,
    B00000100,
    B00000100,
    B11111100
      
    };


void setup() {
 
//start up the  LED Matrix display
	matrix.begin(0x70);  // pass in the address
 
 //set matrix defaults
	matrix.setTextSize(1);
	matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
	matrix.setTextColor(LED_ON);
	matrix.setRotation(3);
  
  for (int8_t x=7; x>=-120; x--) {
  	matrix.clear();
  	matrix.setCursor(x,0);
  	matrix.print("Initializing. Wait");
  	matrix.writeDisplay();
  	delay(50);
  }

	//show hourglass symbol
	matrix.clear();
	matrix.drawBitmap(4, 0, timer_bmp, 8, 8, LED_ON);
	matrix.writeDisplay();
			
	Serial.begin(115200);
	Serial.println(F("FONA Basic Test"));
	Serial.println(F("Initializing....(May take 3 seconds)"));

	// make it slow so its easy to read!
	fonaSerial->begin(4800);
	if (! fona.begin(*fonaSerial)) {
		Serial.println(F("Couldn't find FONA"));
	for (int8_t x=7; x>=-100; x--) {
		matrix.clear();
		matrix.setCursor(x,0);
		matrix.print("Cant find FONA");
		matrix.writeDisplay();
		delay(50);
  }
    while(1);
  }
   
	Serial.println(F("FONA is OK"));
	for (int8_t x=7; x>=-100; x--) {
  	matrix.clear();
  	matrix.setCursor(x,0);
  	matrix.print("Found FONA");
  	matrix.writeDisplay();
  	delay(50);
	}

	Serial.println("FONA Ready");
  
  //get signal strength
  uint8_t n = fona.getRSSI();
  
  //if the signal strenth is to low, will need to wait until it comes back up
  while (n < 5){
    for (int8_t x=7; x>=-120; x--) {
		  matrix.clear();
  		matrix.setCursor(x,0);
  		matrix.print("Signal too low. Retrying");
  		matrix.writeDisplay();
  		delay(50);
	  }
    delay(5000);
    n=fona.getRSSI();
    Serial.print(n);
  }
	
	String signal;
	if (n >= 15)
		signal = "4 Bars";
	if (n <= 14)
		signal = "3 Bars";
	if (n<=9)
		signal = "2 Bars";
	if (n<=4)
		signal = "1 Bar";
 	if (n==0)
		signal = "None";

	String stgOne = "Signal:";  
  String stgTwo = stgOne + signal;
  
	for (int8_t x=7; x>=-100; x--) {
  	matrix.clear();
  	matrix.setCursor(x,0);
  	matrix.print(stgTwo);
  	matrix.writeDisplay();
  	delay(50);
	}
	
	//show hourglass symbol while deleting messages
	matrix.clear();
	matrix.drawBitmap(4, 0, timer_bmp, 8, 8, LED_ON);
	matrix.writeDisplay();
	
	//delete any old sms messages from phone
	int8_t smsn = 1;
	delay(9000);
	int8_t smsnum = fona.getNumSMS();
	if (smsnum < 0) {
		Serial.println(F("Could not read # SMS"));
	} else {
			Serial.print(smsnum);
			Serial.println(F(" SMS's on SIM card!"));
			for ( ; smsn <= smsnum; smsn++){
				Serial.print(F("\n\rDeleting SMS #")); Serial.println(smsn);
			if (fona.deleteSMS(smsn)) {
				Serial.println(F("OK!"));
			} else {
				Serial.println(F("Couldn't delete"));
			}
			}
		}
	for (int8_t x=7; x>=-120; x--) {
		matrix.clear();
		matrix.setCursor(x,0);
		matrix.print("Monitoring Started");
		matrix.writeDisplay();
		delay(50);
	}
	sendStatus("Power Monitor up and running OK.");
}

void loop() {
	//read in voltage from data pin
	int sensorValue = analogRead(DATA_PIN);
	float volts = sensorValue * (5.0 / 1023.0);
	Serial.println(volts);
 
	if (fona.available())  {
	  Serial.println("Checking messages");
	  checkMessage();
	}

	if (volts > voltageThreshold){
	  //flash the red LED if everything is OK
		if (powerOut == true){
      for (int8_t x=7; x>=-120; x--) {
            matrix.clear();
            matrix.setCursor(x,0);
            matrix.print("Power ON.");
            matrix.writeDisplay();
            delay(50);
          }
		  sendStatus("POWER BACK ON ");
		  powerOut = false;
		}
	}

		else {
			while (volts <= voltageThreshold){
	
				count = count + 1;
				
				//check for any messages
				if (fona.available())  {
					Serial.println("Checking messages");
					checkMessage();
				}
				if (powerOut == false){	
					for (int8_t x=7; x>=-100; x--) {
            matrix.clear();
            matrix.setCursor(x,0);
            matrix.print("Power OFF.");
            matrix.writeDisplay();
            delay(50);
					}
  					sendStatus("POWER OUT. Checking every 60 Min.");
  					powerOut = true;   
				}
		
				//delays is 1 second. There are 3600 seconds per hour.
				if (count == 3600){
					sendStatus("POWER IS STILL OUT. Checking every 60 Min.");
					count = 0;  
				}
		
				//wait 1 second
				delay(1000);
				sensorValue = analogRead(DATA_PIN);
				volts = sensorValue * (5.0 / 1023.0);
			}//end while loop
 		}//end else

  //check for low temperature
	uint32_t celsius = htu.readTemperature();
	uint32_t fahrenheit = (celsius * 1.8) + 32;
	if ((fahrenheit < lowTempLevel) and (tempLow == false)){
		tempLow = true;
		sendStatus("Temperature is low.");
		
	}  
	if (fahrenheit > lowTempLevel){
		if (tempLow == true){
		sendStatus("Temperature above threshold now."); 
		}
		tempLow = false;
	}
	delay(1000);
}//end loop

//send status message to authorized phone number that has battery level, cuurent battery voltage and temperature
void sendStatus(const char*myMessage){
     //show status message on matrix
    for (int8_t x=7; x>=-75; x--) {
      matrix.clear();
      matrix.setCursor(x,0);
      matrix.print("Sending SMS");
      matrix.writeDisplay();
      delay(50);
    }
      
    //display sms icon
    matrix.clear();
    matrix.drawBitmap(0, 0, sms1_bmp, 8, 8, LED_ON);
    matrix.drawBitmap(8, 0, sms2_bmp, 8, 8, LED_ON);
    matrix.writeDisplay();
    
    delay(3000);
    matrix.clear();
    matrix.writeDisplay();
    
    char message[128];
    message[0]='\0';
    strcat(message,myMessage);
    if (! fona.getBattPercent(&vbat)) {
              Serial.println(F("Failed to read Batt"));
              strcat(message,"Error Batt");
              
            } else {
              Serial.print(F("VPct = ")); Serial.print(vbat); Serial.println(F("%"));
              
              //convert battery
              char b[4];
              String str;
              str=String(vbat);
              str.toCharArray(b,4); 
              strcat(message,"Battery Level:");     
              strcat(message,b);
              strcat(message,"% "); 
              
      }
        
    //get currrent voltage reading
    int sensorValue = analogRead(DATA_PIN);
    float volts = sensorValue * (5.0 / 1023.0);
    char result[8];
    dtostrf(volts, 2, 2, result);
    strcat(message,"Voltage: ");
    strcat(message,result);   
    
    //get temperature
    strcat(message," Temperature:");
    uint32_t celsius = htu.readTemperature();
    uint32_t fahrenheit = (celsius * 1.8) + 32;
    dtostrf(fahrenheit, 2, 0, result);
    strcat(message,result);
    fona.sendSMS(ALLOWED_NUMBER,message);
 
}

//check to see if there are any text messages. If so, send a status update
void checkMessage(){

    //handy buffer pointer
    char fonaInBuffer[64];    //for notifications from the FONA
    char* bufPtr = fonaInBuffer; 
    int slot = 0;            //this will be the slot number of the SMS
    int charCount = 0;
    //Read the notification into fonaInBuffer
    do  {
      *bufPtr = fona.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (fona.available()) && (++charCount < (sizeof(fonaInBuffer)-1)));
    
    //Add a terminal NULL to the notification string
    *bufPtr = 0;
    
    //Scan the notification string for an SMS received notification.
    //  If it's an SMS message, we'll get the slot number in 'slot'
    if (1 == sscanf(fonaInBuffer, "+CMTI: \"SM\",%d", &slot)) {
      Serial.print("slot: "); Serial.println(slot);
      char callerIDbuffer[32];  //we'll store the SMS sender number in here
      
      // Retrieve SMS sender address/phone number.
      if (! fona.getSMSSender(slot, callerIDbuffer, 31)) {
        Serial.println("Didn't find SMS message in slot!");
      }
      Serial.print(F("FROM: ")); Serial.println(callerIDbuffer);
      Serial.print(F("allowed number: ")); Serial.println(ALLOWED_NUMBER);

      // Retrieve SMS value.
      uint16_t smslen;

      if (! fona.readSMS(slot, replybuffer, 250, &smslen)) { // pass in buffer and max len!
          Serial.println("Failed!");
          
         }
      else {
        Serial.print(F("***** SMS #")); Serial.print(slot);
        Serial.print(" ("); Serial.print(smslen); Serial.println(F(") bytes *****"));
        Serial.println(replybuffer);
        Serial.println(F("*****"));
        
      }
      
      //only send back a text message if the phone number matches. We dont want to respond to spammer numbers or unauthorized numbers
      String from_num = String(callerIDbuffer);
      String allowed_num = String(ALLOWED_NUMBER);
      
      if (from_num == allowed_num){
        Serial.println("Sending response...");
        String str(replybuffer);
        sendStatus("Power Monitor is running.");
      
      }
      // delete the original msg after it is processed
      //   otherwise, we will fill up all the slots
      //   and then we won't be able to receive SMS anymore
      if (fona.deleteSMS(slot)) {
        Serial.println(F("OK!"));
      } else {
        Serial.println(F("Couldn't delete"));
      }
          
    }//if  message
 
}

