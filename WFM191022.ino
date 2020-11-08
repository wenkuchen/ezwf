//Title: Button Debouncing using a State Machine, EZ wire former EZWFM 2.3
//Author: Wayne Chen
//Date: Oct. 22, 2019
//
//Description:
//  -A State Machine is a useful tool to organize complex code
//  -Think of it like the next step beyone "If Else" statements
//  -This example code uses a State Machine to handle tac switch debouncing
//  -It also has a "Hold" function to enable interaction with long button presses
//
//Wiring Instructions:
//  -Wire a keypad with 5 keys with serial resistors.  (I use pin A0)
// (we will use an internal pullup resistor for Pedal button, so no need to worry about wiring a resistor)

#include <Wire.h>
#include <LiquidCrystal.h>

// Setting the LCD shields pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Define button constants
#define btnRIGHT  1
#define btnUP     2
#define btnDOWN   3
#define btnLEFT   4
#define btnSELECT 5
#define btnNONE   0

#define MaxHeatingTime 5 // 5 sec
#define PresetHeatingTime 2  // 2 sec
#define PresetBasePower 85 // PWM 85% of full machine power as for standard L&H 0.018 X 0.025

#define Pedal_Pin 2
#define Buzz_Pin 3
#define IR_ObstaclePin 12
#define PWM_Pin 11


// Creates 3 custom characters for the menu display
byte menuCursor[8] = {
  B01000, //  *
  B00100, //   *
  B00010, //    *
  B00001, //     *
  B00010, //    *
  B00100, //   *
  B01000, //  *
  B00000  //
};

byte downArrow[8] = {
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b10101, // * * *
  0b01110, //  ***
  0b00100  //   *
};

byte upArrow[8] = {
  0b00100, //   *
  0b01110, //  ***
  0b10101, // * * *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100  //   *
};

// (C)
byte CharCopyright[8] = {
 B01110,
 B10001,
 B10101,
 B10111,
 B10101,
 B10001,
 B01110,
 B00000
};

// (R)
byte CharRegistered[8] = {
 B11111,
 B10001,
 B10101,
 B10001,
 B10011,
 B10101,
 B11111,
 B00000
};


// filled
byte CharFilled[8] = {
  B00000, // 
  B11111, // *****
  B11111, // *****
  B11111, // *****
  B11111, // *****
  B11111, // *****
  B11111, // *****
  B00000  // 
};

// Empty
byte CharEmpty[8] = {
 B00000,
 B11111,
 B10001,
 B10001,
 B10001,
 B10001,
 B11111,
 B00000
};

/* You can have up to 10 menu items in the menuItems[] array below without 
having to change the base programming at all. Name them however you'd like. Beyond 10 items, 
you will have to add additional "cases" in the switch/case
section of the operateMainMenu() function below. 
You will also have to add additional void functions (i.e. menuItem11, menuItem12, etc.) to the program.
*/

String menuItems[] = {"L&H 018*025", "L&H 016*022", "Ni-Ti 021*025", "Ni-Ti 018(Rnd)", "Form Manually", "PWR Adjustment"};


//int FlagToHeat = 0;  // Flag to indicate ready to heat wire
float WireSpec = 1.0;  // Base wire as NiTi 0.018 X0.025

// Navigation button variables
//int readKey = 0;
//int savedDistance = 0;
int isStartingUp = 1;

int SettedPowerLevel = 4;  // middle on 1-7 scale, every scale increase/decrease 3.3%

// Menu control variables
int top_item = 0; // index of the main menu array
int ptr = 0;  // upper: 0  lower: 1
int activeButton = 0;

int m_level = 1; // menu level 

//Top Level Variables
int DEBUG = 1;  //Set to 1 to enable serial monitor debugging info

int state_keypad = 0;
//int state_prev = 0;

int keyActive = 0;
int keyClicked = 0;
int keyHold = 0;

int val_keypad = 0;

unsigned long t_set = 0;
unsigned long t_now = 0;
unsigned long bounce_delay = 10;
unsigned long hold_delay = 1000;

void setup() {
  // Initializes serial communication
  Serial.begin(9600);

  pinMode(IR_ObstaclePin, INPUT);
  pinMode(Buzz_Pin, OUTPUT);
  pinMode(PWM_Pin, OUTPUT);
  pinMode(Pedal_Pin, INPUT_PULLUP);

  // Initializes and clears the LCD screen
  lcd.begin(16, 2);
  lcd.clear();

  // Creates the byte for the 3 custom characters
  lcd.createChar(0, menuCursor);
  lcd.createChar(1, upArrow);
  lcd.createChar(2, downArrow);
  lcd.createChar(3, CharCopyright);
  lcd.createChar(4, CharRegistered);
  lcd.createChar(5, CharFilled);
  lcd.createChar(6, CharEmpty);

}

//**********************
void loop() {
  OpeningHello();
  menuOp();
}
//**********************


void OpeningHello(){  

  int activeButton = 0;
  
  // Opening Hello string display

    Serial.print("\nisStarting = ");
    Serial.print(isStartingUp);
  
    if (isStartingUp != 1) {
      return; 
    }
    
    isStartingUp = 0;
    Serial.print("\nisStarting = ");
    Serial.print(isStartingUp);
  
  //flash on/off data loading....
  for(int n=0; n<8; n++){
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print("Data loading...");
    delay(300);
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print("               ");   
    delay(300); 
    }
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("EZWFormer");
  lcd.write(3);
  lcd.print(" V2.2");
  lcd.setCursor(0,1);
  //lcd.print("any key to start..."); //Need to scroll string here!
  lcd.print("               ");
  lcd.blink();

  delay(2000);

  while(1) {
    SM_keypad();
    if(keyClicked) {  // any key pressed to break out
      lcd.noBlink();
      keyClicked = 0; // reset key status
      keyActive = 0;
      keyHold = 0;
      break;
    }
  }
  
}
//-------------------------End of OpenningHello()


void menuRedraw(){  // ----------------------------------
  lcd.clear();
  lcd.setCursor(15,0);
  if(top_item != 0){
    lcd.write(1); // char UP arrow if not at top item
    }
  lcd.setCursor(15,1);
  if(top_item < (sizeof(menuItems) / sizeof(menuItems[0]))-1){
    lcd.write(2); // char DOWN arrow if not at bottom item
    }
  lcd.setCursor(0,ptr);
  lcd.write(1); // char RIGHT menu arrow
  
  lcd.setCursor(1,0); // 2nd char at line 1
  lcd.print(menuItems[top_item]);
  
  lcd.setCursor(1,1); // 2nd char at line 2
  lcd.print(menuItems[top_item+1]);
}  //-------------------End of menuRedraw()


void menuOp(){  //--------------------------
  
  SM_keypad();
 
  if(keyClicked && (m_level == 1)){  // top menu level

    switch(keyActive){

      case btnRIGHT:
        m_level = 2;
        if((ptr+top_item)!= 6) { 
          // first 5 items ready to heat
          ReadHeatSignal(ptr + top_item);
        } else { 
          // last item to adjust power
          powerAdjust();
        }
        break; 
        
      case btnUP:
        if (ptr == 1) {
          ptr = 0; // move ptr to upper
        } else {
          ptr = 1; // move ptr to lower
          --top_item;
        }
        menuRedraw();
        break;
        
      case btnDOWN:
        if (ptr == 0) {
          ptr = 1; // move ptr to lower
        } else {
          ptr = 0; // move ptr to upper
          ++top_item;
          menuRedraw();
        }
        break;

      default:
        break;
    }
  }

  if(keyClicked && (m_level == 2) && (keyActive == btnLEFT)){
    m_level = 1;
    menuRedraw();
  }
} // ----- ------------- End of menuOp()

void SM_keypad() {
  //Almost every state needs these lines, so I'll put it outside the State Machine

  val_keypad = evaluateButton(analogRead(A0));

  //state_prev = state_keypad;

  //State Machine Section
  switch (state_keypad) {
    case 0: //RESET!
      //Catch all "home base" for the State MAchine
      state_keypad = 1;
      keyActive = btnNONE;
      keyClicked = btnNONE;
      keyHold = btnNONE;
    break;

    case 1: //WAIT
      //Wait for the another key 
      if (val_keypad != btnNONE) {
        state_keypad = 2;
        }
    break;

    case 2: //ARMING! button activated
      //Record the time and proceed to ARMED
      t_set = millis();
      keyActive = val_keypad;
      state_keypad = 3;
    break;

    case 3: //ARMED
      //Check to see if the proper has delay has passed.  If a bounce occures then RESET
      t_now = millis();
      if (val_keypad == keyActive) {
        if (t_now - t_set > bounce_delay) {state_keypad = 4;}
        if (val_keypad == btnNONE) {state_keypad = 0;}
      }
    break;

    case 4: //DRAWN
      //If button go released, then TRIGGER or as a click. Also check timer for a "Long Pess"
      t_now = millis();
      if (val_keypad == btnNONE) {state_keypad = 5;}
      if (t_now - t_set > hold_delay) {state_keypad = 6;}
    break;

    case 5: //TRIGGERED!
      //reset the State Machine
      state_keypad = 0;
      keyClicked = keyActive;
    break;

    case 6: //HOLD!
      //proceed to LOW WAIT
      state_keypad = 7;
      keyHold = keyActive;
    break;

    case 7: //LOW WAIT to reset 
      //wait to go back empty key, then reset
      if (val_keypad == btnNONE) {state_keypad = 0;}
    break;
  }
  
} // ----------------------------------- End of SM_keypad()


//while loop to wait for LEFT to return, or Pedal pressed or IR detected
int ReadHeatSignal(int HeatingMode) { //------------------------------------------

  lcd.clear();
  lcd.setCursor(0, 0);
  if(HeatingMode != 5)
    {lcd.print("Ready to form...");} else {
    lcd.print("Manually form...");
    }
    
  lcd.setCursor(2, 1);
  if(HeatingMode == 5) 
    {lcd.print(menuItems[HeatingMode]);} else {
      lcd.print("Up to 6 sec.");
    }
  
  switch(HeatingMode) {
    case 1:
      WireSpec = 1; // Base Wire spec as for L&H 0.018X0.025 archwire
      break;
    case 2:
      WireSpec = (16*22)/(18*25); // Base Wire spec as for L&H 0.016X0.022 archwire
      break;
    case 3:
      WireSpec = (22*25)/(18*25); // Base Wire spec as for NiTi 0.022X0.025 archwire
      break;
    case 4:
      WireSpec = (3.14*18*18)/(18*25); // Base Wire spec as for NiTi round 0.018 archwire
      break;
    case 5:
      // heating time set to 6 sec.
      break;
    case 6:
      // this selection is to adjust power
      break;
  }

  
  while (!(keyClicked && (keyActive == btnLEFT))) {

    SM_keypad(); // analog read from keypad

    if (IRDetected()) {
      ToHeatWire(HeatingMode);
    } else {
      Serial.println("IR off");
    }
    delay(100);


    if (PedalPressed()) {
      ToHeatWire(HeatingMode);
    } else {
      Serial.println("Pedal not pressed");
    }
    delay(100);

    }

    menuRedraw(); // back to top level menu
  
}//---------------------------End of ReadHeatSignal()


void ToHeatWire(int HeatingMode){

  int i;
  
  //Serial.println("\nNow heating is going on ");
  //Serial.println(HeatingMode);

  //digitalWrite(Buzz_Pin, HIGH);  // turn on buzzor

  //for(i=0;i<HeatingMode;i++){
    //digitalWrite(Buzz_Pin, HIGH);  // turn on buzzor
    //delay(500);
    //digitalWrite(Buzz_Pin, LOW);  // turn off buzzor
    //delay(250);
  //}

  i = round((PresetBasePower*(1+(SettedPowerLevel-4)*0.033)*WireSpec));
      // PresetBasePower * Setted power level 1-7 * WireSpec = 1 for TiNi 0.018 X 0.025
 
  //Serial.print("\nStart heating Mode = "); Serial.print(HeatingMode);
  
   
    if(HeatingMode != 5) {  

      digitalWrite(Buzz_Pin, HIGH);  // turn on buzzor
      analogWrite(PWM_Pin, map(i,1,100,0,255)); // turn on PWM heat output
      
      delay(round(PresetHeatingTime*1000));

      analogWrite(PWM_Pin, map(1,1,100,0,255)); // turn off PWM heat output
      digitalWrite(Buzz_Pin, LOW);  //// turn off buzzor
      delay(9000);  // wait for 9 sec after each heating
      
    } else { // Apply pwm to MOSFET maximum duration

      digitalWrite(Buzz_Pin, HIGH);  // turn on buzzor
      analogWrite(PWM_Pin, map(i,1,100,0,255)); // turn on PWM heat output
      
      delay(round(MaxHeatingTime*1000));

      analogWrite(PWM_Pin, map(1,1,100,0,255)); // turn off PWM heat output
      digitalWrite(Buzz_Pin, LOW);  //// turn off buzzor
      
      delay(9000);  // wait for 9 sec after each heating
      
    }

} // -------------------------End of ToHeatWire(int HeatingMode)


void powerAdjust(){ //--------------------------

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Power Level");

  while (1){

    LevelIndicatorBar(SettedPowerLevel);
    
    SM_keypad(); // Analog Read from A0 tracking keypad status
    
    //btn = ReadPressedKeypadA0();
    //Serial.print(btn);
    
    if((keyClicked) && (keyActive == btnUP)&& (SettedPowerLevel < 7 )){
      SettedPowerLevel++;
      LevelIndicatorBar(SettedPowerLevel);
      break;
    }

    if((keyClicked) && (keyActive == btnDOWN)&& (SettedPowerLevel >1)) {
      SettedPowerLevel--;
      LevelIndicatorBar(SettedPowerLevel);
      break;
     }

    if((keyClicked) && (keyActive == btnSELECT)) break;
    // set button to set level and done
  }
}//---------------------------End of powerAdjust()


void LevelIndicatorBar(int level){
  int i;
  lcd.setCursor(0,4);
  lcd.write(1);// up arrow
  for(i=0;i<level;i++) lcd.write(5); // filled char
  for(i=0;i<(7-level);i++) lcd.write(6); // empty char
  lcd.write(2);// down arrow
}//---------------------------End of LevelIndicatorBar()


/*
void HeatWire(int HeatingMode) { // -------------
  
  int i;
  
  i = round((PresetBasePower*(1+(SettedPowerLevel-4)*0.03)*WireSpec));
      // PresetBasePower * Setted power level 1-7 * WireSpec = 1 for TiNi 0.018 X 0.025
 
  Serial.print("\nStart heating Mode = "); Serial.print(HeatingMode);
    
    if(HeatingMode != 5) {  

      digitalWrite(Buzz_Pin, HIGH);  // turn on buzzor
      analogWrite(PWM_Pin, map(i,1,100,0,255)); // turn on PWM heat output
      
      delay(round(PresetHeatingTime*1000));

      analogWrite(PWM_Pin, map(1,1,100,0,255)); // turn off PWM heat output
      digitalWrite(Buzz_Pin, LOW);  //// turn off buzzor
      delay(9000);  // wait for 9 sec after each heating
      
    } else { // Apply pwm to MOSFET maximum duration

      digitalWrite(Buzz_Pin, HIGH);  // turn on buzzor
      analogWrite(PWM_Pin, map(i,1,100,0,255)); // turn on PWM heat output
      
      delay(round(MaxHeatingTime*1000));

      analogWrite(PWM_Pin, map(1,1,100,0,255)); // turn off PWM heat output
      digitalWrite(Buzz_Pin, LOW);  //// turn off buzzor
      
      delay(9000);  // wait for 9 sec after each heating
    }

  } //--------------------End of ToHeatWire()
*/  


int PedalPressed(){  // Pedal 2 terminal connected with 0.1uF capacitor
  
  int isPressed;
   
  if (digitalRead(Pedal_Pin) == LOW)
  {
    Serial.println("Pressed!!, Pressed!!");
    //delay(2000);
    //digitalWrite(LED, HIGH);
    isPressed = 1;
    return isPressed;
  }
  else
  {
    //Serial.println("clear");
    //digitalWrite(LED, LOW);
    Serial.println("not Pressed!!, not Pressed!!");
    isPressed = 0;
    return isPressed;
  }
  
} // ------------------End of PedalPressed()


int IRDetected(){ // For IR obstacle module
  
  int isObstacle;
   
  if (digitalRead(IR_ObstaclePin) == HIGH)
  {
    //Serial.println("OBSTACLE!!, OBSTACLE!!");
    //digitalWrite(LED, HIGH);
    isObstacle = 1;
    return isObstacle;
  }
  else
  {
    //Serial.println("clear");
    //digitalWrite(LED, LOW);
    isObstacle = 0;
    return isObstacle;
  }
  
} // ------------------End of IRDetected()




// This function is called whenever a button press is evaluated. 
// The LCD shield works by observing a voltage drop across the buttons all hooked up to A0.
int evaluateButton(int x) {
  int result = btnNONE;
  if (x < 60) {
    result = btnRIGHT;
  } else if (x < 195) {
    result = btnUP;
  } else if (x < 380) {
    result = btnDOWN;
  } else if (x < 555) {
    result = btnLEFT;  
  } else if (x < 790) {
    result = btnSELECT;
  } else if (x > 1000) {
    result = btnNONE;
  }
  
  return result;
}


/************************** Original manually form code
void menuItem5() { 

  int prevPWR;

  prevPWR = SettedPowerLevel;

  PowerLevelPage();  // Set power level page

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(menuItems[4]);
  lcd.setCursor(2, 1);
  lcd.print(" up to 5 sec.);
  
  WireSpec = 1; // Base Wire spec as for NiTi round 0.018 archwire

  ReadHeatSignal(5); 
  
  SettedPowerLevel = prevPWR; // reset the previous PWR setting

}//---------End of menuItem5()
 *****************************/

// testing by 10 20 30 40 50% for 2 sec.
void menuItem5() { 
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready to test ...");
  lcd.setCursor(2, 1);
  lcd.print("From 30% to 100%  ");
  delay(6000);
  
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(30,1,100,0,255));
  delay(2000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  
  lcd.setCursor(2, 1);
  lcd.print("30% 30% 30%");
  delay(6000);
   
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(40,1,100,0,255));
  delay(2000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  
  lcd.setCursor(2, 1);
  lcd.print("40% 40% 40%");
  delay(6000);
  
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(50,1,100,0,255));
  delay(2000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  
  lcd.setCursor(2, 1);
  lcd.print("50% 50% 50%");

  delay(6000);

  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(60,1,100,0,255));
  delay(2000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  lcd.setCursor(2, 1);
  lcd.print("60% 60% 60%");

  delay(6000);
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(70,1,100,0,255));
  delay(2000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  lcd.setCursor(2, 1);
  lcd.print("70% 70% 70%");

  delay(6000);
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(80,1,100,0,255));
  delay(2000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  lcd.setCursor(2, 1);
  lcd.print("80% 80% 80%");

  delay(6000);
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(90,1,100,0,255));
  delay(2000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  lcd.setCursor(2, 1);
  lcd.print("90% 90% 90%");

  delay(6000);
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(100,1,100,0,255));
  delay(2000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  lcd.setCursor(2, 1);
  lcd.print("100% 100% 100%");
  
  delay(6000);
  
}//---------End of menuItem5() test


void menuItem6() { 
  powerAdjust();  // Set power level page
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("  PWR Setting");
  lcd.setCursor(5,1);
  lcd.print("  Saved");
  lcd.blink();
  delay(2500);
  lcd.noBlink();
}
