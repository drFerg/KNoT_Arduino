#include <Arduino.h>
#define RED A5
#define GREEN A3
#define BLUE A4

#define LEDOUTPUT 4
#define LEDONOFF 3

void ledIOSetup(){
	pinMode(LEDOUTPUT, OUTPUT);
	digitalWrite(LEDOUTPUT, LOW);
	pinMode(LEDONOFF, OUTPUT);
	digitalWrite(LEDONOFF, HIGH);
}


void blinker(){
      digitalWrite(LEDOUTPUT, HIGH);
      delay(100);
      digitalWrite(LEDOUTPUT, LOW);
      delay(100);
}


void setColour(int red, int green, int blue)
{
	analogWrite(RED, 255 - red);
	analogWrite(GREEN, 255 - green);
	analogWrite(BLUE, 255 - blue);
}

void rgbSetup(){
	pinMode(RED, OUTPUT);
	pinMode(GREEN, OUTPUT);
	pinMode(BLUE, OUTPUT);

	analogWrite(RED, 255);
	analogWrite(GREEN, 255);
	analogWrite(BLUE, 255);

	analogWrite(RED, 0);
	delay(100);
	analogWrite(GREEN, 0);
	delay(100);
	analogWrite(BLUE, 0);
	delay(100);

	setColour(0,0,0);
}
