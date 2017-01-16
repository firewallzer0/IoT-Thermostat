#include <Arduino.h>

/*
Temperature Indicator
Version 1.0
Last Updated: 04/22/2016
*/

const int temperaturePin = 0; //Temperature Input PIN, This is for the Analog TMP36GZ
const int RED_PIN = 9;
const int GREEN_PIN = 10;
const int BLUE_PIN = 11;
const int COLD = 66;
const int HOT = 74;
const int AC_PIN = 6;
const int HEAT_PIN = 5;
int tempDelay = 0;
const int delayRate = 5;
int shutDownDelay = 0;
const int shutDownDelayRate = 15;


void setup()
{
  Serial.begin(9600); // Sets up the serial monitor
  pinMode(RED_PIN, OUTPUT); //Sets the Pins to an output mode
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(AC_PIN, OUTPUT);
  pinMode(HEAT_PIN, OUTPUT);
  digitalWrite(AC_PIN, HIGH);
  digitalWrite(HEAT_PIN, HIGH);
}


void loop() //The main function aka where all the work is done.
{
  float voltage, degreesC, degreesF; //Declare high accuracy number variables
  voltage = getVoltage(temperaturePin);
  degreesC = (voltage - 0.5) * 100.0;

  // While we're at it, let's convert degrees Celsius to Fahrenheit.
  // This is the classic C to F conversion formula:

  degreesF = degreesC * (9.0/5.0) + 32.0;
  Serial.print("voltage: ");
  Serial.print(voltage);
  Serial.print("  deg C: ");
  Serial.print(degreesC);
  Serial.print("  deg F: ");
  Serial.println(degreesF);

  // These statements will print lines of data like this:
  // "voltage: 0.73 deg C: 22.75 deg F: 72.96"

  // Note that all of the above statements are "print", except
  // for the last one, which is "println". "Print" will output
  // text to the SAME LINE, similar to building a sentence
  // out of words. "Println" will insert a "carriage return"
  // character at the end of whatever it prints, moving down
  // to the NEXT line.


  setLED(COLD, HOT, degreesF);
  delay(2000); // repeat once per second (change as you wish!)
}


float getVoltage(int pin)
{
  // This function has one input parameter, the analog pin number
  // to read. You might notice that this function does not have
  // "void" in front of it; this is because it returns a floating-
  // point value, which is the true voltage on that pin (0 to 5V).
  // Here's the return statement for this function. We're doing
  // all the math we need to do within this statement:

  return (analogRead(pin) * 0.0048828125); //Since Analog 0 returns a value between  0 and 1023. 
  //We need the voltage that analog 0 is returning as that is how we calculate our tempature
  //So we take our input voltage, in this case 5 volts (also works with 3.3 volts)
  //5 volts divided by 1024 the total possible number values that can be returned as 0 is a value
  // With this we get 0.0048828125 so you multiply that against the input and get roughly the voltage the pin is seeing
}

void setLED (int lowRange, int highRange, float currentTemperature )
{
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  if (currentTemperature > highRange)
  {
    digitalWrite(RED_PIN, HIGH);
    Serial.println("Over heat limit");
    Serial.print("The current tempDelay count is: ");
    Serial.println(tempDelay);
    Serial.print("The Delay rate is: ");
    Serial.println(delayRate);
    delay(500);
    if (tempDelay > (delayRate - 1))
    {
      Serial.println("In if tempdelay > delayrate");
      if (digitalRead(AC_PIN) == HIGH)
      {
      Serial.println("In if AC_PIN HIGH");
      digitalWrite(AC_PIN, LOW);  
      }
      else 
      {
      Serial.println("In if AC_PIN LOW");
      shutDownDelay = 0;
      }
    }
    else
    {
      tempDelay++;
    } //End ELSE temp greater than delay
  } //End IF great Than high range
  else if (currentTemperature < lowRange)
  {
    digitalWrite(BLUE_PIN, HIGH);
    Serial.println("Under heat limit");
    Serial.print("The current tempDelay count is: ");
    Serial.println(tempDelay);
    Serial.print("The Delay rate is: ");
    Serial.println(delayRate);
    delay(500);
    if (tempDelay > (delayRate - 1))
    {
      if (digitalRead(HEAT_PIN) == HIGH)
      {
      digitalWrite(HEAT_PIN, LOW);
      }
      else
      {
      shutDownDelay = 0;
      }
    } //End IF tempdelay greater than delay rate
    else 
    {
    tempDelay++;
    }
  } //End ELSEIF temp less than lowRange
  else 
  {
    digitalWrite(GREEN_PIN, HIGH);
    if (digitalRead(AC_PIN) == LOW || digitalRead(HEAT_PIN) == LOW)
    {
      Serial.println("In if AC or HEAT LOW");
      Serial.print("The current shutdown count is: ");
      Serial.println(shutDownDelay);
      Serial.print("The Curent shutdown delay rate is: ");
      Serial.println(shutDownDelayRate);
      shutDownDelay++;
      if (shutDownDelay > shutDownDelayRate)
      {
        Serial.println("In if shutdown delay greater than shutdown rate");
      digitalWrite(HEAT_PIN, HIGH);
      digitalWrite(AC_PIN, HIGH);
      }
    }
    else
    {
    Serial.println("In else AC or Heat LOW");
    shutDownDelay = 0;
    }
  tempDelay = 0;
  }
}
