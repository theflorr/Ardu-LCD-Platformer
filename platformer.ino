/*
  DisplayPins:
   RS pin to digital pin 12
   EN pin to digital pin 11
   D4 pin to digital pin 5
   D5 pin to digital pin 4
   D6 pin to digital pin 3
   D7 pin to digital pin 2

  ButtonPins:
   Button pin to digital pin 8

  JoystickPins:
   VRx pin to A2
*/

#include <LiquidCrystal.h>

// Variable setups START
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int joystickX = A2; 
int xPos = 0;
int lastXPos = -1;
int ButtonRead = 0;
const int Button = 8;

bool wasButtonPressed = false;
unsigned long jumpTime = 0;
bool isJumping = false;
// Variable setups END

// The map. Editable and dynamic
String levelMaps[5][4] = {
  {"                    ", " Welcome Player     ", "                    ", "____________________"},
  {"                    ", " # = Wall           ", "                    ", "_______#_____#______"},
  {"                    ", " * = Death          ", "                    ", "________*___________"},
  {"                    ", " Almost there..     ", "                    ", "____#_____*_________"},
  {"                    ", " Well done Player!  ", "                    ", "____________________"}
};

// Player and objects START

byte player[8] = {
  B01110,
  B01110,
  B00100,
  B01110,
  B10101,
  B00100,
  B01010,
  B10001
};

byte player_jump[8] = {
  B00100,
  B01110,
  B10101,
  B00100,
  B01110,
  B10101,
  B01010,
  B00000
};

byte hashtag[8] = { 
  B11111,
  B10011,
  B10101,
  B11001,
  B10011,
  B10101,
  B11001,
  B11111
};

// Player and objects END

int currentScene = 0;

// setup the lcd n stuff
void setup() {
  lcd.begin(20, 4);
  lcd.createChar(0, player);
  lcd.createChar(1, player_jump);
  lcd.createChar(2, hashtag);
  pinMode(Button, INPUT_PULLUP);
  Serial.begin(9600);

  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 4; j++) {
      levelMaps[i][j] = replaceHashtags(levelMaps[i][j]);
    }
  }

  displayScene(currentScene);
}

void loop() {
  int xValue = analogRead(joystickX);
  ButtonRead = digitalRead(Button);

  // Reads if the button is pressed, if button is pressed: change variable to true and update the jump timer
  if (ButtonRead == LOW && !wasButtonPressed && !isJumping) {
    isJumping = true;
    jumpTime = millis();
  }

  int newXPos = xPos;

  // Edits player position variable when moving joystick left and right
  if (xValue > 600 && xPos < 19) {
    if (isJumping || isValidMove(xPos + 1)) {
      newXPos++;
      delay(150);
    }
  } else if (xValue < 400 && xPos > 0) {
    if (isJumping || isValidMove(xPos - 1)) {
      newXPos--;
      delay(150);
    }
  }

  // Scene switching if the player is on a edge
  if (newXPos == 19) {
    currentScene = (currentScene + 1) % 5;
    newXPos = 0;
    displayScene(currentScene);
  } else if (newXPos == 0 && currentScene > 0) {
    currentScene--;
    newXPos = 19;
    displayScene(currentScene);
  }

  bool needsRedraw = false;

  // Redraws
  if (newXPos != xPos || isJumping) {
    needsRedraw = true;

    lcd.setCursor(lastXPos, 2);
    lcd.print(" ");
    lcd.setCursor(lastXPos, 3);
    lcd.print(levelMaps[currentScene][3].charAt(lastXPos));
  }

  // Handles jumping logic. Such as core jumping, jumping ontop of a box, jumping into a spike
  if (needsRedraw) {
    xPos = newXPos;

    char tileBelow = levelMaps[currentScene][3].charAt(xPos);
    bool canStand = canStandOnTile(tileBelow);
    bool isSolidTop = (tileBelow == '#' || tileBelow == byte(2));

    if (isJumping) {
      lcd.setCursor(xPos, 2);
      lcd.write(byte(1));  

      if (millis() - jumpTime >= 1000) {
        isJumping = false;
        lcd.setCursor(xPos, 2);
        lcd.print(" "); 
      
        char currentTile = levelMaps[currentScene][3].charAt(xPos);
        if (currentTile == '*') {
          handleSpikeCollision();
        } else {
          bool isSolidTop = (currentTile == '#' || currentTile == byte(2));
          if (isSolidTop) {
            lcd.setCursor(xPos, 2);
            lcd.write(byte(0));  
          } else if (canStand) {
            lcd.setCursor(xPos, 3);
            lcd.write(byte(0));
          } else {
            xPos = lastXPos;
          }
        }
      }
    } else {
      if (isSolidTop) {
        lcd.setCursor(xPos, 2);
        lcd.write(byte(0));  
      } else if (canStand) {
        lcd.setCursor(xPos, 3);
        lcd.write(byte(0));
      } else {
        lcd.setCursor(xPos, 3);
        lcd.write(byte(0));
      }
    }

    lastXPos = xPos;
  }

  wasButtonPressed = (ButtonRead == LOW);
}

// Checks if the move is a valid move. Like if the move is into a spike, it calls handleSpikeCollision()
bool isValidMove(int targetXPos) {
  char tile = levelMaps[currentScene][3].charAt(targetXPos);
  
  if (tile == '*') {
    handleSpikeCollision();
    return false;
  }

  return (tile == '_' || tile == ' ');
}

// Charactars the player can stand on
bool canStandOnTile(char tile) {
  return (tile == '_' || tile == '#');
}

// Replaces all hashtags with a box instead, looks cooler ;)
String replaceHashtags(String levelMap) {
  String result = "";
  for (int i = 0; i < levelMap.length(); i++) {
    if (levelMap.charAt(i) == '#') {
      result += (char)2;
    } else {
      result += levelMap.charAt(i);
    }
  }
  return result;
}

// Handle death spikes/orbs
void handleSpikeCollision() {
  currentScene = 0;

  xPos = 0;
  lastXPos = -1;

  lcd.clear();
  displayScene(currentScene);

  delay(500);
}

// Display scenes
void displayScene(int sceneIndex) {
  for (int row = 0; row < 4; row++) {
    displayLevelMap(levelMaps[sceneIndex][row], row);
  }
}

// Display each levelMap. Also replace hashtags with box instead
void displayLevelMap(String levelMap, int row) {
  lcd.setCursor(0, row);
  for (int i = 0; i < levelMap.length(); i++) {
    if (levelMap.charAt(i) == '#') {
      lcd.write(byte(2));
    } else {
      lcd.print(levelMap.charAt(i));
    }
  }
}