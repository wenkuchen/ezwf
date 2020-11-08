#define Testing 0

// Define button constants

#define btnRIGHT  1
#define btnUP     2
#define btnDOWN   3
#define btnLEFT   4
#define btnSELECT 5
#define btnNONE   0

#define MaxHeatingTime 5 // 5 sec
#define PresetHeatingTime 2 // 2 sec
#define PresetBasePower 80 // PWM 80% of full machine power as for standard L&H 0.018 X 0.025

#define Pedal_Pin 2
#define Buzz_Pin 3
#define IR_ObstaclePin 12
#define PWM_Pin 11

// You can have up to 10 menu items in the menuItems[] array below without having to change the base programming at all. Name them however you'd like. Beyond 10 items, you will have to add additional "cases" in the switch/case
// section of the operateMainMenu() function below. You will also have to add additional void functions (i.e. menuItem11, menuItem12, etc.) to the program.

String menuItems[] = {"L&H 018*025", "L&H 016*022", "Ni-Ti 021*025", "Ni-Ti 018(Rnd)", "Form Manually", "PWR Adjustment"};

//int FlagToHeat = 0;  // Flag to indicate ready to heat wire
float WireSpec = 1.0;  // Base wire as NiTi 0.018 X0.025

// Navigation button variables
int readKey = 0;
int savedDistance = 0;
int isStartingUp = 1;

int SettedPowerLevel = 4;  // middle on 1-7 scale, every scale increase/decrease 3.3%

// Menu control variables
int menuPage = 0;
int maxMenuPages = round(((sizeof(menuItems) / sizeof(String)) / 2) + .5);
int cursorPosition = 0;
int activeButton = 0;

// Creates 3 custom characters for the menu display
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

// (R)
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


#include <Wire.h>
#include <LiquidCrystal.h>

// Setting the LCD shields pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

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
  mainMenuDraw();
  drawCursor();
  operateMainMenu();
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
  
  if (ReadPressedKeypadA0() != btnNONE) { // Read any keypad input
    lcd.noBlink();
  }
  //Serial.print("\nreadKey = ");
  //Serial.print(evaluateButton(readKey));
}
//-------------------------End of OpenningHello()



//while loop to wait for any key to be pressed, twice analogRead() of Pin A0
int ReadPressedKeypadA0() { //------------------------------------------

  int activeButton = 0;

  while (activeButton == 0) {

    readKey = analogRead(0);

  Serial.print("\nreadKey1 = ");
  Serial.print(readKey);
      
    if (readKey < 790) {  
      delay(100);
      readKey = analogRead(0);

  //Serial.print("\nreadKey2 = ");
  //Serial.print(readKey);      
    }

    if (evaluateButton(readKey) != btnNONE){
      activeButton = 1;  // any key pressed 
    }
  }

  return evaluateButton(readKey);

}//---------------------------End of ReadPressedKeypadA0()

//while loop to wait for LEFT to return, or Pedal pressed or IR detected
int ReadHeatSignal(int HeatingMode, float Spec) { //------------------------------------------

  int i=0;
  int j=0;

  readKey = analogRead(0);
  
  while ( i != btnLEFT ) {

    readKey = analogRead(0);

  //Serial.print("\nreadKey1 in HeatSignal = ");
  //Serial.print(readKey);
      
    if (readKey < 790) {  
      delay(100);
      readKey = analogRead(0);
    }

    delay(50);

    j = evaluateButton(readKey);
    if ((j == btnLEFT)|| (j == btnSELECT)){
      return 0;
    }
    
    i = IRDetected();
    if (i == 1) {
      ToHeatWire(HeatingMode, Spec);
    } else {
      Serial.println("IR off");
    }
    delay(50);

    i = PedalPressed();
    if (i == 1) {
      ToHeatWire(HeatingMode, Spec);
    } else {
      Serial.println("Pedal not pressed");
    }
    delay(50);

    }
  
}//---------------------------End of ReadHeatSignal()


void ToHeatWire(int HeatingMode, float Spec){

  int i;
  float j;
  
  //Serial.println("\nNow heating is going on ");
  //Serial.println(HeatingMode);

  //digitalWrite(Buzz_Pin, HIGH);  // turn on buzzor

  //for(i=0;i<HeatingMode;i++){
    //digitalWrite(Buzz_Pin, HIGH);  // turn on buzzor
    //delay(500);
    //digitalWrite(Buzz_Pin, LOW);  // turn off buzzor
    //delay(250);
  //}

  j = PresetBasePower*(1+(SettedPowerLevel-4)*0.033)*Spec;
  
  if ( j > 99.5){
    i=100;
  } else {
    i= round(PresetBasePower*(1+(SettedPowerLevel-4)*0.033)*Spec);
  }

    
  
  Serial.println("i = ");
  Serial.println(i);
  Serial.println("SetPowerLvl = ");
  Serial.println(SettedPowerLevel);
  Serial.println("WireSpec = ");
  Serial.println(WireSpec);
  
  // PresetBasePower * Setted power level 1-7 * WireSpec = 1 for TiNi 0.018 X 0.025
 
  //Serial.print("\nStart heating Mode = "); Serial.print(HeatingMode);
  
   
    if(HeatingMode != 5) {  

      digitalWrite(Buzz_Pin, HIGH);  // turn on buzzor
      analogWrite(PWM_Pin, map(i,1,100,0,255)); // turn on PWM heat output
      
      delay(round(PresetHeatingTime*1000)); //2 X 1 second

      analogWrite(PWM_Pin, map(1,1,100,0,255)); // turn off PWM heat output
      digitalWrite(Buzz_Pin, LOW);  //// turn off buzzor
      delay(8000);  // wait for 9 sec after each heating
      
    } else { // Apply pwm to MOSFET maximum duration

      digitalWrite(Buzz_Pin, HIGH);  // turn on buzzor
      analogWrite(PWM_Pin, map(i,1,100,0,255)); // turn on PWM heat output
      
      delay(round(MaxHeatingTime*1000));  // 5 sec

      analogWrite(PWM_Pin, map(1,1,100,0,255)); // turn off PWM heat output
      digitalWrite(Buzz_Pin, LOW);  //// turn off buzzor
      
      delay(8000);  // wait for 9 sec after each heating
    }

} // -------------------------End of ToHeatWire(int HeatingMode)


void PowerLevelPage(){

  int btn =0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Power Level");

  LevelIndicatorBar(SettedPowerLevel);

  while (btn != btnSELECT ){
    
    btn = ReadPressedKeypadA0();
    //Serial.print(btn);
    
    if((btn == btnUP)&& (SettedPowerLevel < 7 )){
      SettedPowerLevel++;
      LevelIndicatorBar(SettedPowerLevel);
    }

    if((btn == btnDOWN)&& (SettedPowerLevel >1)) {
      SettedPowerLevel--;
      LevelIndicatorBar(SettedPowerLevel);
    }
    
  }
}//---------------------------End of PowerLevelPage()


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


int IRDetected(){ // For FC-51 IR obstacle module
  
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



// This function will generate the 2 menu items that can fit on the screen. 
// They will change as you scroll through your menu. Up and down arrows will indicate your current menu position.
void mainMenuDraw() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(menuItems[menuPage]);
  lcd.setCursor(1, 1);
  lcd.print(menuItems[menuPage + 1]);
  if (menuPage == 0) {
    lcd.setCursor(15, 1);
    lcd.write(byte(2)); //arrow down only
  } else if (menuPage > 0 and menuPage < maxMenuPages) {
    lcd.setCursor(15, 1);
    lcd.write(byte(2)); // arrow down
    lcd.setCursor(15, 0);
    lcd.write(byte(1)); // arrow up
  } else if (menuPage == maxMenuPages) {
    lcd.setCursor(15, 0);
    lcd.write(byte(1)); //arrow up only
  }
}//----------------End of mainMenuDraw()

// When called, this function will erase the current cursor and redraw it based on the cursorPosition and menuPage variables.
void drawCursor() {
  for (int x = 0; x < 2; x++) {     // Erases current cursor
    lcd.setCursor(0, x);
    lcd.print(" ");
  }

  // The menu is set up to be progressive (menuPage 0 = Item 1 & Item 2, menuPage 1 = Item 2 & Item 3, menuPage 2 = Item 3 & Item 4), so
  // in order to determine where the cursor should be you need to see if you are at an odd or even menu page and an odd or even cursor position.
  if (menuPage % 2 == 0) {
    if (cursorPosition % 2 == 0) {  // If the menu page is even and the cursor position is even that means the cursor should be on line 1
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
    }
    if (cursorPosition % 2 != 0) {  // If the menu page is even and the cursor position is odd that means the cursor should be on line 2
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
    }
  }
  if (menuPage % 2 != 0) {
    if (cursorPosition % 2 == 0) {  // If the menu page is odd and the cursor position is even that means the cursor should be on line 2
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
    }
    if (cursorPosition % 2 != 0) {  // If the menu page is odd and the cursor position is odd that means the cursor should be on line 1
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
    }
  }
}


void operateMainMenu() {
  int activeButton = 0;
  
  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790);0; {
      delay(100);
      readKey = analogRead(0);
    }
    
    button = evaluateButton(readKey);
    
    switch (button) {
      case btnNONE: // When button returns as 0 there is no action taken
        break;
      case btnRIGHT:  // This case will execute if the "right" button is pressed
        button = 0;
        switch (cursorPosition) { // The case that is selected here is dependent on which menu page you are on and where the cursor is.
          case 0:
            menuItem1();
            break;
          case 1:
            menuItem2();
            break;
          case 2:
            menuItem3();
            break;
          case 3:
            menuItem4();
            break;
          case 4:
            menuItem5();
            break;
          case 5:
            menuItem6();
            break;
          case 6:
            menuItem7();
            break;
          case 7:
            menuItem8();
            break;
          case 8:
            menuItem9();
            break;
          case 9:
            menuItem10();
            break;
        }
        activeButton = 1;
        mainMenuDraw();
        drawCursor();
        break;
      case btnUP:
        button = 0;
        if (menuPage == 0) {
          cursorPosition = cursorPosition - 1;
          cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));
        }
        if (menuPage % 2 == 0 and cursorPosition % 2 == 0) {
          menuPage = menuPage - 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        if (menuPage % 2 != 0 and cursorPosition % 2 != 0) {
          menuPage = menuPage - 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        cursorPosition = cursorPosition - 1;
        cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));

        mainMenuDraw();
        drawCursor();
        activeButton = 1;
        break;
      case btnDOWN:
        button = 0;
        if (menuPage % 2 == 0 and cursorPosition % 2 != 0) {
          menuPage = menuPage + 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        if (menuPage % 2 != 0 and cursorPosition % 2 == 0) {
          menuPage = menuPage + 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        cursorPosition = cursorPosition + 1;
        cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));
        mainMenuDraw();
        drawCursor();
        activeButton = 1;
        break;
    }
  }
}

// This function is called whenever a button press is evaluated. The LCD shield works by observing a voltage drop across the buttons all hooked up to A0.
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

// If there are common usage instructions on more than 1 of your menu items you can call this function from the sub
// menus to make things a little more simplified. If you don't have common instructions or verbage on multiple menus
// I would just delete this void. You must also delete the drawInstructions()function calls from your sub menu functions.
void drawInstructions() {
  lcd.setCursor(0, 1); // Set cursor to the bottom line
  lcd.print("Use ");
  lcd.write(byte(1)); // Up arrow
  lcd.print("/");
  lcd.write(byte(2)); // Down arrow
  lcd.print(" buttons");
}


void menuItem1() { 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready to form...");
  lcd.setCursor(2, 1);
  lcd.print(menuItems[0]);

  
  //WireSpec = 1; // Base Wire spec as for NiTi 0.018 X 0.025 archwire

  ReadHeatSignal(1, 1.0);
}



void menuItem2() { 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready to form...");
  lcd.setCursor(2, 1);
  lcd.print(menuItems[1]);
  
  //WireSpec = 0.7822; // Base Wire spec as for NiTi 0.016 X 0.022 archwire  adjusted 0.91

  ReadHeatSignal(2, 0.82);
}

void menuItem3() { 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready to form...");
  lcd.setCursor(2, 1);
  lcd.print(menuItems[2]);
  
  //WireSpec = 1.166; // Base Wire spec as for NiTi round 0.021 X 0.025 archwire

  ReadHeatSignal(3, 1.166);
}

void menuItem4() { 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready to form...");
  lcd.setCursor(2, 1);
  lcd.print(menuItems[3]);
  
  //WireSpec = 0.565; //(3.14*(18/2)*(18/2))/(18*25); // Base Wire spec as for NiTi round 0.018 archwire

  ReadHeatSignal(4, 0.65); // adjusted for non linear factor

}


void menuItem5() { 

  //int prevPWR;

  //prevPWR = SettedPowerLevel;

  //PowerLevelPage();  // Set power level page

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(menuItems[4]);
  lcd.setCursor(2, 1);
  lcd.print(" up to 5 sec.");
  
  //WireSpec = 1; // Base Wire spec as for NiTi round 0.018 archwire

  ReadHeatSignal(5, 1.0); 
  
  //SettedPowerLevel = prevPWR; // reset the previous PWR setting

}//---------End of menuItem5()


/*
// testing by 10 20 30 40 50% for 5 sec.
void menuItem5() { 
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Testing 5 sec...");
  
  lcd.setCursor(2, 1);
  lcd.print("From 30% to 100% ");
  delay(3000);
  
  lcd.setCursor(2, 1);
  lcd.print("30% 30% 30%");
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(30,1,100,0,255));
  delay(5000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  
  delay(3000);

  
  lcd.setCursor(2, 1);
  lcd.print("40% 40% 40%"); 
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(40,1,100,0,255));
  delay(5000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  
  delay(3000);

  lcd.setCursor(2, 1);
  lcd.print("50% 50% 50%");
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(50,1,100,0,255));
  delay(5000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  
  delay(3000);
  
  lcd.setCursor(2, 1);
  lcd.print("60% 60% 60%");
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(60,1,100,0,255));
  delay(5000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  

  delay(3000);
  
  lcd.setCursor(2, 1);
  lcd.print("70% 70% 70%");
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(70,1,100,0,255));
  delay(5000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));

  delay(3000);
  
  lcd.setCursor(2, 1);
  lcd.print("80% 80% 80%");
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(80,1,100,0,255));
  delay(5000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));

  delay(3000);
  
  lcd.setCursor(2, 1);
  lcd.print("90% 90% 90%");
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(90,1,100,0,255));
  delay(5000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  
  delay(3000);
  
  lcd.setCursor(2, 1);
  lcd.print("100% 100% 100%");
  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  analogWrite(PWM_Pin, map(100,1,100,0,255));
  delay(5000);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  analogWrite(PWM_Pin, map(1,1,100,0,255));
  
}//---------End of menuItem5() test
*/

void menuItem6() { //if Testing mode, do testing
  int i;
  if(Testing == 1){
    for(i=0;i<100;i++) {
      lcd.setCursor(1, 0);
      lcd.print("Testing......");
      digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
      analogWrite(PWM_Pin, map(90,1,100,0,255));
      delay(3000);
      digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
      analogWrite(PWM_Pin, map(1,1,100,0,255));
      lcd.setCursor(2, 1);
      lcd.print("90% PWM PWR");
      delay(1000);           
    }
  }
  PowerLevelPage();  // Set power level page
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("  PWR Setting");
  lcd.setCursor(5,1);
  lcd.print("  Saved");
  lcd.blink();

  digitalWrite(Buzz_Pin,HIGH);  // turn on buzzor
  delay(300);
  digitalWrite(Buzz_Pin,LOW);  // turn off buzzor
  delay(1000);
  lcd.noBlink();
}


void menuItem7() { // Function executes when you select the 7th item from main menu
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Sub Menu 7");

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void menuItem8() { // Function executes when you select the 8th item from main menu
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Sub Menu 8");

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void menuItem9() { // Function executes when you select the 9th item from main menu
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Sub Menu 9");

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void menuItem10() { // Function executes when you select the 10th item from main menu
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Sub Menu 10");

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 4:  // This case will execute if the "back" button is pressed
        button = 0;
        activeButton = 1;
        break;
    }
  }
}
