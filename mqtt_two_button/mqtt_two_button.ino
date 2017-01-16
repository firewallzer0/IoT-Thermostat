#include <Arduino.h>

/*
 * This sketch is for testing the basic input of and connectivity of
 * an MQTT device. This will be just an update to the MQTT server:
 * Of Button1 or Button2
 * 
 * We are using pull up resisters so we are looking for a button LOW
 * 
 * Version 0.3
 * Last Updated 04/29/2016
 * 
 * 
 */

//The various needed libraries.
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

//Hardcode MAC Address this should be updated based on the device you are using.
byte mac[] = {0x00, 0xAA, 0xBB, 0x0D, 0x01, 0x07};

//Fall back settings if DHCP doesn't work
IPAddress ip(192, 168, 10, 240); //First IoT device, .2 is for the MQTT server, .1 is the router, gateway and DHCP server provided by a PFsense box
IPAddress myDns(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0); // 23-bit network mask for 512 - 2 IP addresses
IPAddress server(192, 168, 10, 152); //MQTT Server address

//Declaring vars
long lastReconnectAttempt = 0;

int button1LastState = 0;
int button1State = 0;
int button2LastState = 0;
int button2State = 0;
int LED_state = 0;
String p = "";

//Pins 10, 11, 12, and 13 are not usable, as they are used by the ethernet shield. 
const int LED = 9;
const int button1_pin = 8;
const int button2_pin = 7;

//Report message recieved
void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);

  Serial.print("] "); 
  p = "";
  for (int i=0;i<length;i++) 
  {
    Serial.print((char)payload[i]);
    p.concat((char)payload[i]);
  }
  Serial.println();
  
  Serial.print("Var P equals: ");
  Serial.println(p);

  LED_state = digitalRead(LED);
  
  if (p == "Button1" && LED_state != HIGH)
  {
  digitalWrite(LED, HIGH);
  Serial.println("Writing LED HIGH");
  }
  else if (p == "Button2" && LED_state != LOW)
  {
  digitalWrite(LED, LOW);
  Serial.println("Writing LED LOW");
  }
}

EthernetClient ethClient;
PubSubClient client(ethClient);

/*
Blocks normal function when the MQTT server is unreachable.
void reconnect() 
{
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect("arduinoClient")) 
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("connected","Thermostat1_Connected");
      // ... and resubscribe
      client.subscribe("thermostat");
    } else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
*/

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



void setup() 
{
  Serial.begin(9600);
  Serial.println("Trying to get an IP address using DHCP");

  //Set the MQTT server address and port number
  client.setServer(server, 1883);
  client.setCallback(callback);
  
  if (Ethernet.begin(mac) == 0) 
    {
      Serial.println("Failed to configure Ethernet using DHCP");
      // initialize the Ethernet device not using DHCP:
      Ethernet.begin(mac, ip, myDns, gateway, subnet);
    delay(2000);
    }
  
  
  // print your local IP address:
  Serial.print("My IP address: ");
  ip = Ethernet.localIP();
  
  for (byte thisByte = 0; thisByte < 4; thisByte++) 
    {
    // print the value of each byte of the IP address:
    Serial.print(ip[thisByte], DEC);
    Serial.print(".");
    }
  
  Serial.println();







  lastReconnectAttempt = 0; 



  pinMode(LED, OUTPUT);
  pinMode(button1_pin, INPUT);
  pinMode(button2_pin, INPUT);
  digitalWrite(LED, LOW);
  button1LastState = 0;
  button1State = 0;
  button2LastState = 0;
  button2State = 0;
  
} //End Setup





void loop() 
{
  /*
  Blocking Code
  if (!client.connected()) 
  {
  reconnect();
  } 
  client.loop();
  */
  
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
  
  button1State = digitalRead(button1_pin);
  button2State = digitalRead(button2_pin);

  //Serial.print("Button 1 State is: ");
  //Serial.println(button1State);
  //Serial.print("Button 2 State is: ");
  //Serial.println(button2State);
  delay(100);
  LED_state = digitalRead(LED);
  if (button1State == LOW && button1LastState == 0 && LED_state != HIGH)
  {
    if (client.publish("thermostat", "Button1"))
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
    if (client.publish("thermostat", "Button2"))
    {
      button2LastState = 1;
    }
  }
  else if (button2State == HIGH && button2LastState == 1)
  {
    button2LastState = 0;
  }  
}
