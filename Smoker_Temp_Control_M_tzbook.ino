
/*
 * --------------------------------------------
 * Arduino code for smoker temperature control
 * --------------------------------------------
 * 
 * - Fan controlled via PWM, shutdown via transistor
 * - Temperature setting via tuttons
 * - Output to I2C display
 * 
 * --------------------------------------------
 * March 2021
 * by Mätzbook / Ma-ctrl-dot
 * 
 */

//########## Libraries ########################################################
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Bounce2.h>


//########## Pins ########################################################
#define Pin_fan    9
#define Pin_tacho  2

#define Pin_btn_up 7
#define Pin_btn_dw 5
#define Pin_btn_st 6

#define Pin_ntc   A0

#define Pin_fan_pwm   9
#define Pin_fan_rel   8
#define Pin_fan_tacho 2


//########## Variables ########################################################
Bounce btn_up = Bounce(); //Button Temp -   UP
Bounce btn_dw = Bounce(); //Button Temp - DOWN
Bounce btn_st = Bounce(); //Button Temp -  SET

int   temp_set   = 120;
int   temp_new   = 120;
float temp_mes   = 0  ;
int   temp_range = 10 ;


//Temparature-variables
const int   temp_meas_count    = 3;    //Number of measured temparuture values
float       averageTemp        = 0;    //Variable for averaging temperature
float       temp_calculating   = 0;     //Float for calculating the temperature exactly to Steinhard
const int   resistorInRow      = 10000; //Resistor connected in row to the NTC
const int   ntcResistance_std  = 10000; //Standard-resistance of the NTC
const int   ntcTemperature_std = 25;    //Standard-temperature of the NTC
const int   ntc_bCoefficient   = 3977;  //Beta-coefficient of the NTC

int    temp_averaging[temp_meas_count]; //Array for averaging temperature

//Fan-Variables
int   fan_speed              = 0;
int   fan_pwm_min            = 45;
int   fan_out                = 1;
int   tacho_request_interval = 2000;
int   pulse_stretch_interval = 2000;
float fan_rps                = 0;
int   fan_rpm                = 0;
float fan_time_rotation      = 0;
float fan_time_per_pulse     = 0;

//time-system-variables
int interval_temp = 2000;
int interval_btns =   10;
int interval_pulse= 3000;
int interval_fanst= 5000;

unsigned long millis_temp =  0;
unsigned long millis_btns =  0;
unsigned long millis_pulse = 0;
unsigned long millis_fanst = 0;

LiquidCrystal_I2C lcd(0x27, 20, 4); //LCD-Display


//########## SETUP ########################################################
void setup() 
{

  Serial.begin(9600); //Serial Monitor
  
  Serial.println("Setup started");
  
  pinMode(Pin_btn_up, INPUT); //define pins for btns as inputs
  pinMode(Pin_btn_dw, INPUT); //define pins for btns as inputs
  pinMode(Pin_btn_st, INPUT); //define pins for btns as inputs

  pinMode(Pin_ntc,    INPUT); //define pin  for ntc  as  input

  btn_up.attach(Pin_btn_up); //setting pin to Bounce-btn
  btn_dw.attach(Pin_btn_dw); //setting pin to Bounce-btn
  btn_st.attach(Pin_btn_st); //setting pin to Bounce-btn

  btn_up.interval(5); //setting measuring interval for unbouncing
  btn_dw.interval(5); //setting measuring interval for unbouncing
  btn_dw.interval(5); //setting measuring interval for unbouncing

  //setting up display and loading animation
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("loading.............");
  delay(500);
  lcd.setCursor(0,1);
  lcd.print("loading.............");
  delay(500);
  lcd.setCursor(0,2);
  lcd.print("loading.............");
  delay(500);
  lcd.setCursor(0,3);
  lcd.print("loading.............");
  delay(500);
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("TEMP-SET:");
  lcd.setCursor(13,0);
  lcd.print("New:");
  lcd.setCursor(0,1);
  lcd.print("TEMP-ACTUAL:");
  lcd.setCursor(4,2);
  lcd.print("FAN-PWM:");
  lcd.setCursor(0,3);
  lcd.print("UP");
  lcd.setCursor(16,3);
  lcd.print("DOWN");
  lcd.setCursor(8,3);
  lcd.print("SET");
  
  measureTemp();

  millis_temp = millis();
  millis_btns = millis();
  millis_pulse = millis();
  Serial.println("Setup finished");
}


//########## LOOP ########################################################
void loop() 
{
  if((millis() - interval_btns) >= millis_btns)
  {
    checkBtns();
    millis_btns = millis();
  }

  if((millis() - interval_temp) >= millis_temp)
  {
    millis_temp = millis();
    measureTemp();
   // calc_fan_speed();
  }

  if((millis() - interval_fanst) >= millis_fanst)
  {
    millis_fanst = millis();
    calc_fan_speed();
  }
  
  if((millis() - interval_pulse) >= millis_pulse)
  {
    pulse_stretch();
    millis_pulse = millis();
  }

  Serial.print("TimeMillis: ");
  Serial.print(millis());
  Serial.print("  Time-measureTemp: ");
  Serial.println(millis_temp);
  

  delay(10); //Delay to prevent the Arduino-Board ;-)
}


//########## Button-check-method ########################################################
void checkBtns()
{
 btn_up.update();
 btn_dw.update();
 btn_st.update();

 if(btn_up.fell())
 {
  Serial.println("Button 'Temp -   UP' pressed");
  temp_new+=5;
  
  lcd.setCursor(9,0);
  lcd.print(temp_set);

  lcd.setCursor(17,0);
  lcd.print(temp_new);
 }
 if(btn_dw.fell())
 {
  Serial.println("Button 'Temp - DOWN' pressed");
  temp_new-=5;
  
  lcd.setCursor(9,0);
  lcd.print(temp_set);

  lcd.setCursor(17,0);
  lcd.print(temp_new);
 }
 if(btn_st.fell())
 {
  Serial.println("Button 'Temp -  SET' pressed");
  temp_set = temp_new;
  
  lcd.setCursor(9,0);
  lcd.print(temp_set);

  lcd.setCursor(17,0);
  lcd.print(temp_new);
 }

}
//########## Temperature-measuring-method ########################################################
void measureTemp()
{
  // Taking a couple of measurments in a row for exact temperature
  for (int i=0; i < temp_meas_count; i++) {
    temp_averaging[i] = analogRead(Pin_ntc);
    delay(10);
  }
  // averaging all measurments
  averageTemp = 0;
  for (int i=0; i < temp_meas_count; i++) {
    averageTemp += temp_averaging[i];
  }
  averageTemp /= temp_meas_count;
  
  // resistance
  averageTemp = 1023 / averageTemp - 1;
  averageTemp = resistorInRow / averageTemp;
  
  // calculating the temperature with the Steinhard-calculation
  temp_calculating  = averageTemp / ntcResistance_std;      // (R/Ro)
  temp_calculating  = log(temp_calculating);                // ln(R/Ro)
  temp_calculating /= ntc_bCoefficient;                     // 1/B * ln(R/Ro)
  temp_calculating += 1.0 / (ntcTemperature_std + 273.15);  // + (1/To)
  temp_calculating  = 1.0 / temp_calculating;               // Invertieren
  temp_calculating -= 273.15;                               // Umwandeln in °C

  temp_mes = temp_calculating;
  
  Serial.print  ("New measured temperature: ");
  Serial.println(temp_mes);
  
  lcd.setCursor(13,1);
  lcd.print(temp_mes);
}


//########## Pulse-analyzing-method ########################################################
void pulse_stretch()
{
  if (fan_out == 0)
  {
    analogWrite(Pin_fan_pwm, 255);                        // Power fan
    fan_time_per_pulse = pulseIn(Pin_tacho, HIGH);        // requesting the time per pulse in microseconds
    analogWrite(Pin_fan_pwm, fan_speed);                  // re-setting the fan speed
    fan_time_rotation = ((fan_time_per_pulse * 4)/1000);  // calculating time per rotation in milliseconds
    fan_rps = (1000/fan_time_rotation);                   // calculating the rotations per second
    fan_rpm = (fan_rps*60);                               // calculating the rotations per minute

    Serial.print("FAN-RPM: ");
    Serial.println(fan_rpm);
  }
  else
  {
    Serial.print("FAN-RPM: ");
    Serial.println("0");
  }
}


//########## Pulse-analyzing-method ########################################################
void calc_fan_speed()
{
  lcd.setCursor(19,2);
  lcd.print("W");
  
  int tmp_min = temp_set - temp_range; 
  int tmp_max = temp_set + temp_range;
  
  fan_speed = map(temp_mes, tmp_min, tmp_max, 255, 0);

  if(fan_speed< fan_pwm_min)
  {
    fan_speed = 0;
    fan_out = 1;
    digitalWrite(Pin_fan_rel, LOW);
  }
  
  else if(fan_speed > 255)
  {
    fan_speed = 255;
    fan_out = 0;
    digitalWrite(Pin_fan_rel, HIGH);
  }

  fan_out = 0;
  analogWrite(Pin_fan_pwm, fan_speed);
  
  Serial.print("FAN-PWM: ");
  Serial.println(fan_speed);

  lcd.setCursor(13,2);
  lcd.print(fan_speed);
  lcd.setCursor(19,2);
  lcd.print(" ");
}
