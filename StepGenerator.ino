
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

//Define some handy pin names
#define encoderClick 2
#define encoderA 3
#define encoderB 4
#define outputPin 5
#define diroutPin 6
#define runPin 7
#define sensorPin 11
#define manualPin 9
#define dirinPin 8

//Declare global elements
LiquidCrystal_I2C lcd(0x27,16,2);  
//Variables to keep state of the encoder
 int encoderCounter=0;
 int encoderState=LOW;
 int encoderLast=LOW;
 int encoderClickState=HIGH;

 unsigned int pValue=0;
 unsigned int wValue=0;
 unsigned int pCounter=0;
 unsigned int runCounter=0;
 char row[] = " P:00000 W:00000";
 char option[] = " Test    Exit   ";
 char brand[] = "StepGenerator   ";
 char set[] = "Press to setup  ";

 boolean pulseLevel = true;
 boolean manualLevel = true;
 boolean goPulse = false;
 boolean goManual = false;
 boolean lastSensorInput = true;

 unsigned char fsmState=0;
 
 #define eventClick  1
 #define eventCW  2
 #define eventCCW  3

 #define welcomeState 0
 #define testState 1
 #define exitState 2
 #define pulseState 3
 #define widthState 4
 #define editPulse 5
 #define editWidth 6
 #define testingState 7
 #define runningState 8
 

 void FSM(unsigned char event){
    switch(fsmState){

      case welcomeState:
          if (event == eventClick) fsmState = testState;
      break;
      
      case testState: // testState
        if (event == eventCW) fsmState=exitState;
        if (event == eventCCW) fsmState=widthState;
        if (event == eventClick){
          pCounter = pValue;
          goPulse = true;
          fsmState = testingState;
        }
      break;
      case exitState:
        if (event == eventCW) fsmState=pulseState;
        if (event == eventCCW) fsmState=testState;
        if (event == eventClick) fsmState = welcomeState;
      break;
      case pulseState:
        if (event == eventCW) fsmState=widthState;
        if (event == eventCCW) fsmState=exitState;
        if (event == eventClick) fsmState = editPulse;
      break;
      case widthState:
        if (event == eventCW) fsmState=testState;
        if (event == eventCCW) fsmState=pulseState;
        if (event == eventClick) fsmState = editWidth;
      break;
      case editPulse:
        if (event == eventCW) pValue++;
        if (event == eventCCW)pValue--;
        if (event == eventClick){
          EEPROM.put(0, pValue);
          fsmState = pulseState;
        }
      break;
      case editWidth:
        if (event == eventCW) wValue++;
        if (event == eventCCW)wValue--;
        if (event == eventClick){
          EEPROM.put(2, wValue);
          fsmState = widthState;
        }
      break;
      case testingState:
      break;
      case runningState:
      break;
      default:
      fsmState=0;
      break;
    }
 }

void setup() {
  //Init the LCD
  EEPROM.get(0,pValue);
  EEPROM.get(2,wValue);
   lcd.init();                      
   lcd.clear();
   lcd.backlight();
   lcd.home();
  //Init input/output pins
  pinMode(encoderClick,INPUT_PULLUP);
  pinMode(encoderA,INPUT);
  pinMode(encoderB,INPUT);
  pinMode(outputPin,OUTPUT);
  pinMode(runPin,INPUT_PULLUP);
  pinMode(dirinPin, INPUT_PULLUP);
  pinMode(diroutPin, OUTPUT);
  pinMode(sensorPin,INPUT_PULLUP);
  pinMode(manualPin,INPUT_PULLUP);
  encoderLast = digitalRead(encoderA);  
  noInterrupts();
  //Init Timer2 interrupt
   TIMSK2 = (TIMSK2 & B11111110) | 0x01;
   TCCR2B = (TCCR2B & B11111000) | 0x07;

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;   // preload timer
  OCR1A = wValue;
  TCCR1B = B00001010;
  //TCCR1B |= (1 << CS11);    // 8 prescaler 
  //TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  TIMSK1 |= (1 << OCIE1A);


    
  interrupts();
    

}


void updateLCD(){
  lcd.home();
  lcd.noBlink();
  if (fsmState == welcomeState){
    lcd.print(brand);
    lcd.setCursor(0,1);
    lcd.print(set);
  } else if (fsmState != runningState){
    sprintf(row," P:%05u W:%05u",pValue,wValue);
    lcd.print(row);
    lcd.setCursor(0,1);
    lcd.print(option);
    lcd.blink();
  } else if (fsmState == runningState){
    sprintf(row,"Runs:%08u   ",runCounter);
    lcd.print(row);
    lcd.setCursor(0,1);
    if (goPulse){
      lcd.print("running...      ");
    } else {
      lcd.print("waiting...      ");
    }
  }
 
  switch(fsmState){
    case testState:
      lcd.setCursor(1,1);
      break;
      case exitState:
      lcd.setCursor(9,1);
      break;
      case pulseState:
      lcd.setCursor(1,0);
      break;
      case widthState:
      lcd.setCursor(9,0);
      break;
    case editPulse:
      lcd.setCursor(7,0);
      break;
    case editWidth:
      lcd.setCursor(15,0);
      break;
    case testingState:
      lcd.setCursor(4,1);
  }
  
}

int counter = 0;
boolean flagUpdateLCD = false;
ISR(TIMER2_OVF_vect){
    counter++;
    if (counter>20){
      counter=0;
      flagUpdateLCD=true;
    }

}

//ISR(TIMER1_OVF_vect)        // interrupt service routine 
ISR(TIMER1_COMPA_vect)
{  

  //Check for manualRun
  if (goManual){
     OCR1A = wValue;
     if (manualLevel){
        manualLevel=false;
        digitalWrite(outputPin,HIGH);
     } else {
        manualLevel=true;
        digitalWrite(outputPin,LOW);
     }
     return;
  }
  
  // Check for goPulse
  if (goPulse){
      //Send pulse
      OCR1A = wValue;
      if (pulseLevel){
        pulseLevel=false;
        digitalWrite(outputPin,HIGH);
      } else {
        pulseLevel=true;
        digitalWrite(outputPin,LOW);
        //Inside the Low part figure out what to do
        if (fsmState == testingState){
            //While testing decCounter
            pCounter--;
            if (pCounter==0){
              fsmState = testState; //Go back to test
              goPulse = false; // Stop the pulses
            }
        } else if (fsmState == runningState){
          //decrement pCounter
          pCounter--;
          if (pCounter==0){
            goPulse = false; //Stop running
            runCounter++;
          }
        }
      }
  }
  
}

void processEncoder(){
  encoderState = digitalRead(encoderA); // Reads the "current" state of the outputA
   // If the previous and the current state of the outputA are different, that means a Pulse has occured
   if (encoderState != encoderLast){     
     // If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
     if (digitalRead(encoderB) != encoderState) { 
       encoderCounter ++;
     } else {
       encoderCounter --;
     }
   } 
   encoderLast = encoderState; // Updates the previous state of the outputA with the current state
}



void loop() {
  digitalWrite(diroutPin,digitalRead(dirinPin));

  goManual = !digitalRead(manualPin);
  
  if (digitalRead(runPin)){
    fsmState = runningState;
  } else {
    if (fsmState == runningState){
      fsmState=welcomeState;
      runCounter=0;
    }
  }
  boolean sensorInput = digitalRead(sensorPin);
  if (sensorInput == LOW && lastSensorInput == HIGH && !goPulse){
    //Sensor detection
    pCounter = pValue;
    goPulse = true;
  }
  lastSensorInput=sensorInput;
  
  
  int auxEncoder = encoderCounter/2;
   processEncoder();
   if (auxEncoder< encoderCounter/2){
    FSM(eventCCW);
   } else if (auxEncoder > encoderCounter/2){
    FSM(eventCW);
   }
   if (flagUpdateLCD){
    flagUpdateLCD=false;
    updateLCD();
   }
   int currentClickState = digitalRead(encoderClick);
   if (currentClickState==LOW && encoderClickState==HIGH){
    FSM(eventClick);
   }
   encoderClickState=currentClickState;
   

    
}
