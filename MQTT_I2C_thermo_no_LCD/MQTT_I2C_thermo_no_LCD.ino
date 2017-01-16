/*
* This sketch is for testing the basic input of and connectivity of
* an MQTT device. This will be just an update to the MQTT server:
* On or Off
* We are also updating the temperature reading anytime it get more than
* 0.25 degrees fahrenheit but only once every 30 seconds
*
* We are using pull up resisters so we are looking for a button LOW
* condition when the button is pressed.
*
* Version 0.5
* Last Updated 05/10/2016
*
*
*/

//The various needed libraries.
#include <SPI.h> // SPI?
#include <Ethernet.h> //Ethernet Protocols Lib
#include <PubSubClient.h> //MQTT Lib
#include <Arduino.h> //General Arduino Commands for debugging serial.
#include <Wire.h> //I2C Lib
#include "Adafruit_MCP9808.h" //Adafruit Breakout board Lib
#include <RunningAverage.h> //A lib to handle getting an average int, from http://playground.arduino.cc/Main/RunningAverage

//Hardcode MAC Address this should be updated based on the device you are using.
byte mac[] = {0x00, 0xAA, 0xBB, 0x0D, 0x01, 0x07};

//Fall back settings if DHCP doesn't work
IPAddress ip(172, 17, 6, 3); 		//IP of this device
IPAddress myDns(172, 17, 6, 1);		//DNS Address
IPAddress gateway(172, 17, 6, 1); 	//Gateway if needed.
IPAddress subnet(255, 255, 254, 0);  	//Always needed, common is 255.255.255.0
IPAddress server(172, 17, 6, 2); 	//MQTT Server address

//MQTT Vars

long lastReconnectAttempt = 0;

//Input Vars
bool button1LastState = 0;
bool button1State = 0;
bool button2LastState = 0;
bool button2State = 0;

//Thermostat Vars
bool LED_state = 0;						//This var is used for controlling if the thermostat is on or off.
bool AC_ON = 0;
bool FAN_ON = 0;
bool HEAT_ON = 0;
int AC_TRIGGER = 66;					//What temperature the AC is set to.
int HEAT_TRIGGER = 70;					//What temperature the Heat is set to.
int RUNNING_TIMER = 0;					//How long the HVAC system has been running
int SLEEP_TIMER = 0;					//Delay of restarting the HVAC system
bool SLEEPING = 0;						//Is the system currently sleeping?
const int MAX_RUN_TIME = 7200;			//This is the longest amount of time the HVAC will stay on. Default: 7200 (2 hours). Measured in seconds.
const int MAX_SLEEP_TIME = 900;			//How long the HVAC has to be off for before it can turn back on. Default: 900 (15 minutes). Measured in seconds.


//Temperature Vars
int tempDelay = 0;
int timer = 0;
float reported_F = 0;
float reportedDelta = 0;
float current_F = 0;
float c = 0;
float last_F = 0;
float average_F = 0;
float delta_F = 0;
bool s1_state = 0;
bool temperatureUpdate = 0;
char* givenTemp;
char message_buffer[6];

//Used for incoming MQTT messages
String p = "";


//Pins 10, 11, 12, and 13 are not usable, as they are used by the ethernet shield.
const int LED = 9;
const int button1_pin = 8;
const int button2_pin = 7;
const int FAN_pin = 6;
const int AC_pin = 5;
const int HEAT_pin = 4;



//Default is 60 seconds or a delayRate: 60
const int delayRate = 60;
RunningAverage tempArray(30); //Define our Average temperature array sample size. NOTE: The higher this number the more memory used.


//Create the MCP9808 object
Adafruit_MCP9808 s1 = Adafruit_MCP9808();


EthernetClient ethClient;
PubSubClient client(ethClient);

//Non-Blocking MQTT Reconnect
boolean reconnect()
	{
	if (client.connect("HA-Thermostat1"))
		{
		// Once connected, publish an announcement...
		client.publish("Connected","Thermostat1_Connected");
		// ... and resubscribe
		//client.subscribe("device/thermostat");
		client.subscribe("thermostat");
		}
	return client.connected();
	}

//Report message recieved
void callback(char* topic, byte* payload, unsigned int length)
	{
	//Serial.print("Message arrived [");
	//Serial.print(topic);
	//Serial.print("] ");
	
	p = ""; //clear all values of p
	for (int i=0;i<length;i++)				//Rebuild the entire payload 1 char at a time saving the data to p
		{
		//Serial.print((char)payload[i]);
		p.concat((char)payload[i]);
		}
		
		
	//Serial.println();
	//Serial.print("Var P equals: ");
	//Serial.println(p);

	LED_state = digitalRead(LED);

	if (p == "on" && LED_state != HIGH)
		{
		digitalWrite(LED, HIGH);
		if (current_F > AC_TRIGGER)
			{
			client.publish("thermostat","AC_TURNED_ON");
			digitalWrite(FAN_pin, LOW);
			digitalWrite(AC_pin, LOW);
			AC_ON = 1;
			}
		//Serial.println("Writing LED HIGH");
		}
	else if (p == "off" && LED_state != LOW)
		{
		digitalWrite(LED, LOW); 			//turn LED off
		digitalWrite(AC_pin, HIGH);			//Turn AC Off
		digitalWrite(HEAT_pin, HIGH);		//Turn Heat Off
		digitalWrite(FAN_pin, HIGH);		//Turn Fan Off
		RUNNING_TIMER = 0;					//Reset Running Timer
		SLEEPING = 0;						//Disable Sleep Mode
		SLEEP_TIMER = 0;					//Reset Sleep Timer
		
		//Serial.println("Writing LED LOW");
		}
	else if (p == "set_ac_*")
		{
		//string magic to extract temperature
		//AC_TRIGGER = newTemp;
		}
	else if (p == "set_heat_*")
		{
		//string magic to extract temperature
		//HEAT_TRIGGER = newTemp;
		}
	else
		{
		client.publish("errors", "Unknown_Message_Recieved");
		//client.publish("errors", p);
		//client.publish("errors", "End_Unknown_Message")
		}
	}

	
	
/*********************************************************************************/
/*****                                SETUP                                  *****/
/*********************************************************************************/

void setup()
	{
  //Pin Setup
  pinMode(LED, OUTPUT);
  pinMode(button1_pin, INPUT);
  pinMode(button2_pin, INPUT);
  pinMode(AC_pin, OUTPUT);
  pinMode(FAN_pin, OUTPUT);
  pinMode(HEAT_pin, OUTPUT);


  //Make sure the everything is off
  digitalWrite(LED, LOW);
  digitalWrite(FAN_pin, HIGH);
  digitalWrite(AC_pin, HIGH);
  digitalWrite(HEAT_pin, HIGH);
  
	//Serial.begin(9600);
	//Serial.println("Trying to get an IP address using DHCP");
  digitalWrite(LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
	//Set the MQTT server address and port number
	client.setServer(server, 1883);
	client.setCallback(callback);

	//Ethernet Setup
  /*
	if (Ethernet.begin(mac) == 0)
		{
		//Serial.println("Failed to configure Ethernet using DHCP");
		//initialize the Ethernet device not using DHCP:
		Ethernet.begin(mac, ip, myDns, gateway, subnet);
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
    digitalWrite(LED, HIGH);
		delay(2000);
    digitalWrite(LED, LOW);
		reconnect();
		client.loop();
		delay(1000);
		client.publish("logs", "T1_DHCP_FAILURE");
		client.publish("errors", "T1_DHCP_FAILURE");
		}
*/
  Ethernet.begin(mac, ip, myDns, gateway, subnet);
  digitalWrite(LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);

/*
Ethernet.begin(mac, ip, myDns, gateway, subnet);
delay(1000);
reconnect();
client.loop();*/

	// print your local IP address:
	/*
	Serial.print("My IP address: ");
	ip = Ethernet.localIP();

	for (byte thisByte = 0; thisByte < 4; thisByte++)
	{
	// print the value of each byte of the IP address:
	Serial.print(ip[thisByte], DEC);
	Serial.print(".");
	}*/

	//Serial.println();

	//Serial.println("Connecting to MQTT client");
	reconnect();
	client.loop();
	delay(1000);
	//Serial.println("Setting up Sensor...");

	//Sensor Setup and Error Check
	if (s1.begin(0x18))
		{
		client.publish("logs", "T1_S1_GO");
		s1_state = 1;
		//Serial.println("T1_S1_GO");
		}
	else
		{
		client.publish("logs", "T1_S1_NOGO");
		client.publish("errors", "T1_S1_MIA");
		//Serial.println("T1_S1_NOGO");
		s1_state = 0;
		}
		
	//delay(1500);
	//Serial.println("Sensor Setup complete");
  
	//var setup
	lastReconnectAttempt = 0;
	button1LastState = 0;
	button1State = 0;
	button2LastState = 0;
	button2State = 0;
	tempArray.clear();
	
	
	
	if (s1_state)
		{
		s1.shutdown_wake(0); //Turn on the sensor
		c = s1.readTempC();
		current_F = c * 9.0 / 5.0 + 32; //Get data from the sensor
		delay(250);
		s1.shutdown_wake(1); //Turn off the sensor

		tempArray.addValue(current_F);
		last_F = tempArray.getAverage();
		average_F = last_F;
		reported_F = last_F;
		givenTemp = dtostrf(last_F, 5, 2, message_buffer);
		client.publish("temperature",givenTemp);
		}
	else
		{
		//Serial.println("Temperature Sensor not avaliable");
		client.publish("errors", "T1_S1_FOUND_NOTREPORTING");
		delay(5000);
		}

	} //End Setup

/*********************************************************************************/
/*****                             SETUP END                                 *****/
/*********************************************************************************/

/*********************************************************************************/
/*****                             MAIN LOOP                                 *****/
/*********************************************************************************/

void loop()
	{
	delay(100);
	
	//Non-Blocking Reconnect
	if (!client.connected())
		{
		long now = millis();

		if (now - lastReconnectAttempt > 5000)
			{
			lastReconnectAttempt = now;

			// Attempt to reconnect
			if (reconnect())
				{
				lastReconnectAttempt = 0;
				}
			}
		}
	else
		{
		// Client connected
		client.loop();
		}

	//Check State of buttons and LED for changes 
	button1State = digitalRead(button1_pin);
	button2State = digitalRead(button2_pin);
	LED_state = digitalRead(LED);
	
	/*********************************************************************************/
	/*****                   Button Press Detection                              *****/
	/*********************************************************************************/
	
	
	if (button1State == LOW && button1LastState == 0 && LED_state != HIGH) //if on button is pushed now, it isn't already on and it wasn't pushed at last check.
		{
    digitalWrite(LED, HIGH);
		if (client.publish("thermostat", "on")) //Try sending an MQTT message on
			{
			button1LastState = 1;
			}
		}

	else if (button1State == HIGH && button1LastState == 1)
		{
		button1LastState = 0;
		}

	if (button2State == LOW && button2LastState == 0 && LED_state != LOW)
		{
      digitalWrite(LED, LOW);       //turn LED off
      digitalWrite(AC_pin, HIGH);     //Turn AC Off
      digitalWrite(HEAT_pin, HIGH);   //Turn Heat Off
      digitalWrite(FAN_pin, HIGH);    //Turn Fan Off
      RUNNING_TIMER = 0;          //Reset Running Timer
      SLEEPING = 0;           //Disable Sleep Mode
      SLEEP_TIMER = 0;          //Reset Sleep Timer
		if (client.publish("thermostat", "off"))
			{
			button2LastState = 1;
			}
		}
	else if (button2State == HIGH && button2LastState == 1)
		{
		button2LastState = 0;
		}
		
	/*********************************************************************************/
	/*****                    End Button Press Detection                         *****/
	/*********************************************************************************/	
	
	/*********************************************************************************/
	/*****                    TIMER CODE AND 1 SECOND LOOP                       *****/
	/*********************************************************************************/
	
	timer++;
	if (timer == 10)
		{
		timer = 0;
		if (s1_state)
			{
			s1.shutdown_wake(0); //Turn on the sensor
			c = s1.readTempC(); //Get data from the sensor
			current_F = c * 9.0 / 5.0 + 32; //Convert C to F
			delay(250); //This delay is required to properly pull information from the sensor.
			s1.shutdown_wake(1); //Turn off the sensor
			tempArray.addValue(current_F);
			average_F = tempArray.getAverage();
			delta_F = average_F - last_F;
			
			
			
			/*********************************************************************************/
			/*****                    		TEMPERATURE UPDATES                          *****/
			/*********************************************************************************/

			if (delta_F > 0.09 || delta_F < -0.09)
				{
				last_F = average_F;

				if (!temperatureUpdate)
					{
					reportedDelta = reported_F - last_F; //calculate differnce

					if (reportedDelta > 0.25 || reportedDelta < -0.25) //Temp change more than 0.25 degrees fahrenheit
						{
						reported_F = last_F;
						givenTemp = dtostrf(reported_F, 5, 2, message_buffer); //int to string conversion
						client.publish("temperature",givenTemp); //MQTT publish
						temperatureUpdate = 1; //Prevents multiple update in a short time span
						
						}
					}
				}// End If delta_F gt or lt 0.10
			else
				{
				if (temperatureUpdate && tempDelay > delayRate - 1) //limits updates to once every 30 seconds
					{
					temperatureUpdate = 0;
					tempDelay = 0;
					}
				else if (temperatureUpdate)
					{
					tempDelay++;
					}
				}
				
			/*********************************************************************************/
			/*****                    TIMER CODE AND 1 SECOND LOOP                       *****/
			/*********************************************************************************/
			
			/*********************************************************************************/
			/*****                    			 HVAC TRIGGERS                           *****/
			/*********************************************************************************/
			
			if (digitalRead(LED)) //Is the system turned on?
				{
				if (reported_F > (AC_TRIGGER + 1.2) && !(AC_ON) && !(HEAT_ON) && !(SLEEPING)) //If it is hotter than the target temperature and the AC is off and the system is NOT sleeping, then turn it on
					{
					client.publish("thermostat","AC_TURNED_ON");
					digitalWrite(FAN_pin, LOW);
					digitalWrite(AC_pin, LOW);
					AC_ON = 1;
					}
				else if (reported_F < (AC_TRIGGER - 0.75) && AC_ON)				//Else if reported temperature is colder than target and the AC is on, turn it off
					{
					SLEEPING = 1;
					client.publish("thermostat","AC_TURNED_OFF");
					client.publish("thermostat","SLEEP_START");
					digitalWrite(AC_pin, HIGH);
					AC_ON = 0;
					}
				else if (AC_ON)											//If the AC is on, count time, check not over max time on, if it is turn it off and start sleeping
					{
					RUNNING_TIMER++;
					if (RUNNING_TIMER > MAX_RUN_TIME)
						{
						client.publish("thermostat","AC_TURNED_OFF");
						client.publish("thermostat","SLEEP_START");
						digitalWrite(AC_pin, HIGH);
						SLEEPING = 1;
						RUNNING_TIMER = 0;
						AC_ON = 0;
						}
					}
				
				else if (SLEEPING)										//If sleeping, count time, check not over max sleep, if so, end sleep.
					{
						SLEEP_TIMER++;
						if (SLEEP_TIMER > MAX_SLEEP_TIME)
							{
							client.publish("thermostat","SLEEP_END");
							client.publish("thermostat","FAN_TURNED_OFF");
							digitalWrite(FAN_pin, HIGH);
							SLEEPING = 0;
              SLEEP_TIMER = 0;
							}
					}
				}
			
			/*********************************************************************************/
			/*****                    		 END HVAC TRIGGERS                           *****/
			/*********************************************************************************/
			
			}// End S1_State
		}// End if Timer
		
	/*********************************************************************************/
	/*****                END TIMER CODE AND 1 SECOND LOOP                       *****/
	/*********************************************************************************/
	
		
	} //End Void Loop

/*********************************************************************************/
/*****                          MAIN LOOP END                                *****/
/*********************************************************************************/
