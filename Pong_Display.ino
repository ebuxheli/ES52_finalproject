/*
  ES52 Final Project: Analog Pong

  Name: Pong_Display.ino
  By: Adham, Austin and Enxhi. 
  Date: November 30th, 2017
  --- Credit to Adafruit for TFT initialization code ---

  This is an Arduino sketch which takes in analog signals from analog
  circuitry - which control the position and velocity of the ball, position
  and movement of the paddle, and reference voltages for the arena boundaries 
  - and displays these signals on a TFT 2.2" 240x320 ILI9340 display produced 
  by Adafruit. 
  
  The display shows gameplay in a user-friendly manner through a colorful and 
  beautiful display. Additionally, input from the analog circuitry is used to 
  register the scoring of a point, which occurs when the ball hits the paddle, 
  triggering a HIGH signal to digital pin 2 incrementing 74HC161 counters 
  outputting their count to 2 Numitrons thus displaying scores from 0 - 99 points.

  mru: Enxhi Buxheli 12/9/2017 - cleaning up the code
*/

// Adding libraries
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"

// Header guard to prevent dual imports from GFX and ILI9340 libraries
#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

// I/O Pin Constants
#define _sclk SCK   // Display: Serial clock pin
#define _miso MISO  // Display: MISO pin
#define _mosi MOSI  // Display: MOSI pin 
#define _cs 12      // Display: CS pin
#define _dc 10      // Display: DC pin
#define _rst 11     // Display: RESET pin

#define ballX A0        // Ball: x-position
#define ballY A1        // Ball: y-position
#define paddleY A2      // Paddle: y-position of midpoint
#define wallTop A3      // Wall: top
#define wallBot A4      // Wall: bottom
#define counter 2       // Counter: score signal output to counters
#define reset 3         // Game: Reset signal
#define restart 5       // Restart: signal from the monostable that triggers the game over

#define PADDLEBOUNCE 4  // DEBUG PADDLE: test of game prior to full functionality

// Program CONSTANTS
const boolean debug = false; // DEBUG TESTING IF TRUE
int thickness = 10; // Constant thickness of boundaries and paddle

// Program Variables
int topWall;        // Variable that stores analog read of the top wall
int botWall;        // Variable that stores analog read of the bottom wall
int difficulty;     // The paddle's height
int yPaddle;        // Midpoint of paddle's y-position
int score;          // Current game score
int highScore;      // High score storage
boolean pause;      // Pausing of the game on a game over

long lastMillis;    // DEBUG SCORING: time of last hit
long curMillis;     // DEBUG SCORING: time of current hit

// Hardware SPI - display runs faster than software equivalent
Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);


void setup() {
  // Initialize our I/O pins
  pinMode(counter, OUTPUT);
  pinMode(reset, OUTPUT);
  pinMode(_cs, OUTPUT);
  pinMode(_dc, OUTPUT);
  pinMode(_rst, OUTPUT);
  pinMode(restart, INPUT);
  
  // DEBUG testing game functionality
  if (debug == true){
    pinMode(PADDLEBOUNCE, OUTPUT);
    digitalWrite(PADDLEBOUNCE, LOW);
  }
    
  // Setup serial connection
  Serial.begin(9600);

  // Begin use of display
  tft.begin();
  Serial.println("TFT initiated");

  // Restart game: draw arena borders
  restartGame();

  // On complete reset of the game, high score reset to 0
  highScore = 0;
  
  pause = false; // Allows game to start, if true, game stops
}

void loop(void) {
  // Draw the movement of the paddle throughout the game while pause is false
  drawPaddle(!pause);

  // Draw the movement of the ball throughout the game while pause is false
  drawBall(!pause);
}



/*********************************************************
 * void restartGame()
 *
 * Function to restart the game when turned on or when 
 * reset button pressed upon a "GAME OVER" or whenever
 * the user feels like pressing the reset button. This 
 * also resets the counters and the ball's position.
 *********************************************************/
void restartGame(){  
  Serial.println("GAME RESET");

  // Send a reset signal so the score and ball position resets
  digitalWrite(reset, LOW);   // reset signal: resets counters and ball position
  delay(50);                  // delay in place for numitron reset [too fast o.w.]
  digitalWrite(reset, HIGH);  // reset is Active Low
  score = 0;                  // reset score to 0 in code
  
  // Clear screen [LEFT-RIGHT] to reset it for gameplay
  tft.setRotation(2);
  tft.fillScreen(ILI9340_BLACK);
  
  // Rotate the display so (0,0) is in top left corner
  tft.setRotation(3);
  
  // Drawing arena with chosen thickness of pixels
  tft.fillRect(0, 0, tft.width()-50, thickness, ILI9340_CYAN);                        // top wall
  tft.fillRect(0, 0, thickness, tft.height(), ILI9340_CYAN);                          // left wall
  tft.fillRect(0, tft.height()-thickness, tft.width()-50, tft.height(), ILI9340_CYAN);// bottom wall
}


/*********************************************************
 * void drawPaddle()
 *
 * Function to draw the paddle as the user input from the 
 * IR sensor changes. This function allows for the smooth
 * movement of the paddle by compensating for the slow 
 * refresh rate of the TFT display with a cushion around
 * the paddle.
 *********************************************************/
void drawPaddle(boolean){
  // Reference voltages to be used in mapping 
  topWall = analogRead(wallTop);
  botWall = analogRead(wallBot);

  // Changing the analog read values into voltages
  float topWallVolts = topWall * (5.0 / 1023.0);
  float botWallVolts = botWall * (5.0 / 1023.0);

  // Draws a paddle which is modified by the changes in arena size
  difficulty = 240 * (1 / (topWallVolts-botWallVolts));
  
  // Read in analog paddle position
  yPaddle = analogRead(paddleY);
  yPaddle = map(yPaddle+difficulty/2, botWall, topWall,  0, 239); // maps analog values to screen
  yPaddle = constrain(yPaddle+difficulty/2, 0, 240-difficulty/2); // constrain to the screen

  // Drawing the actual paddle
  tft.fillRect(tft.width()-50, yPaddle-difficulty/2, thickness, difficulty, ILI9340_WHITE);
  
  // Compensate for slow refresh rate by having a black cushion around the paddle
  tft.fillRect(tft.width()-50, yPaddle+difficulty, thickness, 10, ILI9340_BLACK); // below paddle
  tft.fillRect(tft.width()-50, yPaddle-difficulty, thickness, 10, ILI9340_BLACK); // above paddle
}

/*********************************************************
 * void drawBall()
 *
 * Function to draw the ball throughout the game as it 
 * moves around the screen and bounces. The ball is 
 * confined by the limits of the screen and the boundaries 
 * of the arena. It has a black cushion around it so the 
 * display shows smooth movement of the ball without a  
 * trail.
 *********************************************************/
void drawBall(boolean){
  // ball position values to be mapped
  int xBall = analogRead(ballX);   
  int yBall = analogRead(ballY);

  int radius = 7; // most aesthetically pleasing ball radius

  // ball mapped to the limits of the display - map(val, lowVal, highVal, lowDisp, highDisp)
  xBall = map(xBall, (0.1/5.0)*1023, (4.0/5.0)*1023, 2*thickness+radius-3, 319);
  yBall = map(yBall, botWall, topWall, 2*thickness+radius+1, 239-5*thickness-radius);
  
  // drawing ball - function: tft.fillCircle(x, y, radius, color);
  tft.fillCircle(xBall, yBall, radius, ILI9340_RED);
  
  // compensate for slow refresh rate by having a black cushion around the paddle
  for (int k = 1; k<=5;k++){
    tft.drawRect(xBall-radius-k, yBall-radius-k, (radius+k)*2, (radius+k)*2, ILI9340_BLACK);
  }

  curMillis = millis(); // stores the current time to avoid double counting of the score
  
  // increment the score by 1 when the ball hits the paddle
  if ((yBall < yPaddle+(difficulty/2) && yBall > yPaddle-(difficulty/2)) && ((xBall > (tft.width()-50-thickness)) && 
    (xBall < tft.width()+thickness-50)) && curMillis > lastMillis+250){
    digitalWrite(counter, HIGH);      // incrementing counter
    digitalWrite(counter, LOW);
    score++;
    
    lastMillis = millis();
  }

  // adding paddle functionality when debugging
  if (debug == true){
    curMillis = millis();

    // increment score by 1 when ball hits the paddle  
    if ((yBall < yPaddle+(difficulty/2) && yBall > yPaddle-(difficulty/2)) && ((xBall > (tft.width()-50-thickness)) && 
    (xBall < tft.width()+thickness-50)) && curMillis > lastMillis+250){
      digitalWrite(PADDLEBOUNCE, HIGH); // FAKING BOUNCE
      digitalWrite(counter, HIGH);      // incrementing the score
      digitalWrite(counter, LOW);
      score++;
    
      lastMillis = millis();
    }
    else{
      digitalWrite(PADDLEBOUNCE, LOW); // FAKING BOUNCE
    }
  }
 
  // display GAME OVER when ball passes paddle
  if(analogRead(ballX) > 920 && analogRead(ballX) < 950){
    pause = true;
    gameOver(pause);
  }
}



/*********************************************************
 * void gameOver()
 *
 * Function to clear the display in order to display the 
 * GAME OVER message to the user which tells them that 
 * they lost the game. This message is sent out when the
 * user fails to hit the ball with the paddle. 
 *********************************************************/
void gameOver(boolean){
  Serial.println("GAME OVER");
    
  // Rotate screen to clear screen [LEFT-RIGHT] to prepare for GAMEOVER message
  tft.setRotation(2);
  tft.fillScreen(ILI9340_BLACK);
  Serial.println("Screen reset");

  // Figure out if high score has changed
  if (score > highScore){
    highScore = score;

    // Rotate the display so (0,0) is in the top left corner
    tft.setRotation(3);
    
    // Display a congragulations message for 3 seconds
    tft.setTextColor(ILI9340_YELLOW);
    tft.setTextSize(5);
    tft.setCursor(25,25);
    tft.println("CONGRATS!");
    tft.setCursor(25,70);
    tft.println("NEW HIGH");
    tft.setCursor(45,115);
    tft.println("SCORE!!!");
    delay(3000);
    
    tft.setRotation(2);
    tft.fillScreen(ILI9340_BLACK);
  }
  
  // Rotate the display so (0,0) is in the top left corner
  tft.setRotation(3);

  // Display GAME OVER message
  tft.setCursor(25, 85);            // Aesthetics of message
  tft.setTextColor(ILI9340_GREEN);  // Aesthetics of message   
  tft.setTextSize(5);               // Aesthetics of message
  tft.println("GAME OVER");

  // Display HIGH SCORE
  tft.setCursor(35, 135);           // Aesthetics of message
  tft.setTextColor(ILI9340_RED);    // Aesthetics of message   
  tft.setTextSize(3);               // Aesthetics of message
  tft.print("HIGH SCORE: ");
  tft.println(highScore);

  // send out reset signal to start monostable
  digitalWrite(reset, LOW);
  digitalWrite(reset, HIGH);

  // stay in gameover until the monostable triggers a reset
  while(digitalRead(restart) == HIGH){
    for (int j = 5; j > 0; j--){
      tft.setCursor(5, 165);          // Aesthetics of message
      tft.setTextColor(ILI9340_CYAN); // Aesthetics of message
      tft.setTextSize(2);             // Aesthetics of message
      tft.print("Game will restart in... ");
      tft.print(j);
      delay(1000);
      tft.fillRect(285, 165, 320, 50, ILI9340_BLACK); 
    }
  }
  // allow for the game to restart from game over
  pause = false;
  restartGame();
}
