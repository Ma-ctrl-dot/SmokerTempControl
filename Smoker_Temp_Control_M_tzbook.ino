
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


//########## Variables ########################################################
Bounce btn_up = Bounce(); //Button Temp -   UP
Bounce btn_dw = Bounce(); //Button Temp - DOWN
Bounce btn_st = Bounce(); //Button Temp -  SET

int   temp_set = 120;
int   temp_new = 120;
float temp_mes = 0  ;

//Temparature-variables
const int   temp_meas_count    = 10;    //Number of measured temparuture values
float       averageTemp        =  0;    //Variable for averaging temperature
float       temp_calculating   = 0;     //Float for calculating the temperature exactly to Steinhard
const int   resistorInRow      = 10000; //Resistor connected in row to the NTC
const int   ntcResistance_std  = 10000; //Standard-resistance of the NTC
const int   ntcTemperature_std = 25;    //Standard-temperature of the NTC
const int   ntc_bCoefficient   = 3977;  //Beta-coefficient of the NTC

int    temp_averaging[temp_meas_count]; //Array for averaging temperature

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
  btn_dw.interval(6); //setting measuring interval for unbouncing

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

  updateLCD();
  measureTemp();
  Serial.println("Setup finished");
}


//########## LOOP ########################################################
void loop() 
{
  checkBtns(); //Method for checking if any button is pressed actually
  delay(50);
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
  updateLCD();
 }
 if(btn_dw.fell())
 {
  Serial.println("Button 'Temp - DOWN' pressed");
  temp_new-=5;
  updateLCD();
 }
 if(btn_st.fell())
 {
  Serial.println("Button 'Temp -  SET' pressed");
  temp_set = temp_new;
  updateLCD();
 }

}


//########## LCD-update-method ########################################################
void updateLCD()
{
  lcd.setCursor(0,0);
  lcd.print("TEMP-SET:");
  lcd.setCursor(9,0);
  lcd.print(temp_set);
  lcd.setCursor(13,0);
  lcd.print("New:");
  lcd.setCursor(17,0);
  lcd.print(temp_new);
  lcd.setCursor(0,1);
  lcd.print("TEMP-ACTUAL:");
  lcd.setCursor(13,1);
  lcd.print(temp_mes);
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
  
  updateLCD();
}
