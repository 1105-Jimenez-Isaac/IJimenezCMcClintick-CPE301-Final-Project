#define RDA 0x80
#define TBE 0x20  

volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;
volatile unsigned char *portDDRB = (unsigned char *) 0x24;
volatile unsigned char *portB =    (unsigned char *) 0x25;

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

volatile unsigned char* port_a = (unsigned char*) 0x22;
volatile unsigned char* ddr_a = (unsigned char*) 0x21;
volatile unsigned char* pin_a = (unsigned char*) 0x20;

//include libraries
#include "DHT.h"
#include <LiquidCrystal.h>
#include <RTClib.h>
#include <Stepper.h>

//global variables
#define DHTPIN 33
#define DHTTYPE DHT11
#define STEPS 200

int err = 0;
int printTime = 0;

const int powerButton = 29; //power button pin
const int resetButton = 28; //reset button pin

int previous = 0; //variable for stepper controlled by potentiometer

//variables for dc fan
int enM = 48;
int in1M = 52;
int in2M = 50;

//LED variables
const int blue = 34;
const int green = 36;
const int yellow = 38;
const int red = 40;

//declare stepper, rtc, dht, and lcd
Stepper stepper(STEPS, 2, 3, 4, 5);
RTC_DS1307 rtc;
DHT dht(DHTPIN, DHTTYPE);
const int rs = 10, en = 11, d4 = 9, d5 = 8, d6 = 7, d7 = 6;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


volatile boolean buttonState = LOW;
volatile unsigned long lastDebounceTime = 0;
volatile unsigned long debounceDelay = 50;
volatile boolean power = HIGH;

void setup() {
  //start everything
  U0init(9600);
  dht.begin();
  lcd.begin(16,2);
  rtc.begin();
  stepper.setSpeed(100);

  //sets up dc motor
  pinMode(enM, OUTPUT);
  pinMode(in1M, OUTPUT);
  pinMode(in2M,OUTPUT);

  //starts dc motor in off state
  digitalWrite(in1M, LOW);
  digitalWrite(in2M, LOW);

  //puts RTC in 12 hour mode
 rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  pinMode(powerButton, INPUT_PULLUP);
  // Enable interrupt for start button pin
  attachInterrupt(digitalPinToInterrupt(powerButton), buttonPressed, CHANGE);

  adc_init();

  *ddr_a |= 0x01;
  *ddr_a |= 0x02;
  *ddr_a |= 0x04;
  *drr_a |= 0x08;
  *ddr_a &= EF;
  *ddr_a &= DF;
}

bool stateChange = HIGH;
void loop() {
  buttonPressed();
  if(power == LOW){
  int waterVal = adc_read(7);
  float currentTemp = dht.readTemperature(true);
  float desiredTemp = 75.0;
  if(waterVal < 250 || err == 1){
    RTCUpdate();
    error();
    err = 1;
    }
  
  else{
 
    if(currentTemp > desiredTemp){
    RTCUpdate();
    running();
  }
    else{
    RTCUpdate();
    idle();
  }
}
if(*pin_a & 0x10){
  err = 0;
}
  }
  else{
  RTCUpdate();
  lcd.clear();
  fanOff();
  *port_a &= 0xFE;
  *port_a &= 0xFD;
  *port_a &= 0xF7;
  *port_a |= 0x04;
  }
  my_delay(1000);
}

void running(){
  *port_a |= 0x01;
  *port_a &= 0xFD;
  *port_a &= 0xF7;
  *port_a &= 0xFB;
  fanOn();
  tempHumidity();
  stepperOn();
}

void idle(){
  *port_a &= 0xFE;
  *port_a &= 0xFD;
  *port_a |= 0x08;
  *port_a &= 0xFB;
  tempHumidity();
  fanOff();
  stepperOn();
}

void error(){
  *port_a &= 0xFE;
  *port_a |= 0x02;
  *port_a &= 0xF7;
  *port_a &= 0xFB;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Error: ");
  lcd.setCursor(0,1);
  lcd.print("Water Level Low");
  fanOff();
}
void tempHumidity(){
  //gets values for the temp and humidity
  float h = dht.readHumidity();
  float f = dht.readTemperature(true);

  //prints temp and humidity on lcd
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(("Humidity: "));
  lcd.print(h);
  lcd.print("%");
  lcd.setCursor(0,1);
  lcd.print(F("Temp: "));
  lcd.print(f);
  lcd.print(F("F"));
}

void stepperOn(){
  //controls stepper motor with potentiometer
  int val = adc_read(0);
  stepper.step(val - previous);
  previous = val;
  
}

void fanOn(){
  //turns on fan
  analogWrite(enM, 255);
  digitalWrite(in1M, HIGH);
  digitalWrite(in2M, LOW);
}

void fanOff(){
  digitalWrite(in1M, LOW);
  digitalWrite(in2M, LOW);
}

void RTCUpdate(){

   DateTime now = rtc.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);

    my_delay(500);

}
void buttonPressed() {
  // Read the button state with debouncing
  boolean reading = digitalRead(powerButton);
  if (reading != buttonState) {
    lastDebounceTime = millis();
  }

  if (millis() - lastDebounceTime > debounceDelay) {
    // Update the button state if it has been stable for a while
    buttonState = reading;
    // If button is pressed (active low)
    if (buttonState == LOW) {
      // Toggle LED state
      power = !power;
    }
    my_delay(200);

  }

}
void my_delay(unsigned int delay)
{
  // calc period
  double period = 1.0/double(delay);
  // 50% duty cycle
  double half_period = period/ 2.0f;
  // clock period def
  double clk_period = 0.0000000625;
  // calc ticks
  unsigned int ticks = half_period / clk_period;
  // stop the timer
  *myTCCR1B &= 0xF8;
  // set the counts
  *myTCNT1 = (unsigned int) (65536 - ticks);
  // start the timer
  * myTCCR1A = 0x0;
  * myTCCR1B |= 0b00000001;
  // wait for overflow
  while((*myTIFR1 & 0x01)==0); // 0b 0000 0000
  // stop the timer
  *myTCCR1B &= 0xF8;   // 0b 0000 0000
  // reset TOV           
  *myTIFR1 |= 0x01;
}

void U0init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}
unsigned char U0getchar()
{
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}

unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}