#include <Arduino.h>

/*
 * This sketch is for testing the basic input of and connectivity of
 * an MQTT device. This will be just an update to the MQTT server:
 * "Button Pressed" or "Button Not Pressed"
 * 
 * Version 0.1
 * Last Updated 04/22/2016
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
IPAddress myDns(192, 168,10, 1);
IPAddress gateway(192,168,10, 1);
IPAddress subnet(255, 255, 255, 0); // 23-bit network mask for 512 - 2 IP addresses
IPAddress server(192, 168, 10, 152); //MQTT Server address

//Declaring vars
long lastReconnectAttempt = 0;


//Report message recieved
void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  for (int i=0;i<length;i++) 
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
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
  if (client.connect("HT-arduinoClient")) 
  {
    // Once connected, publish an announcement...
    client.publish("Connected","Thermostat1_Connected");
    // ... and resubscribe
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
  } else 
  {
  // Client connected
  client.loop();
  }

  //Other Functions below like temp!!!
}
