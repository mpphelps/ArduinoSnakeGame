#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// Display wiring
// SCL pin to A5
// SCA pin to A4

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Joystick to arduino pin numbers
// digital pin connected to switch output
const int SW_pin = 2;
// analog pin connected to X output
const int X_pin = A1;
// analog pin connected to Y output
const int Y_pin = A0;

#define GRID_SIZE 8
#define GRID_WIDTH (SCREEN_WIDTH / GRID_SIZE)
#define GRID_HEIGHT (SCREEN_HEIGHT / GRID_SIZE)
#define MAX_SNAKE_SIZE 100
#define MIN_SNAKE_SIZE 3
#define BASE_DELAY 500  // Initial delay in milliseconds
#define SPEED_INCREMENT 20 // Decrease delay by this value for each food eaten
#define MIN_DELAY 100   // Minimum delay to ensure the game remains playable
#define SCORE_ADDRESS 0

enum Direction {UP, DOWN, LEFT, RIGHT};
Direction snakeDirection = RIGHT;
Direction nextSnakeDirection = RIGHT;

struct Point {
  uint8_t x;
  uint8_t y;
};

bool isGameOver = false;

Point snake[MAX_SNAKE_SIZE];
int snakeLength = 0;
Point food;

unsigned long lastUpdate = 0;
int update_interval = BASE_DELAY;

void setup() {
  pinMode(SW_pin, INPUT);
  digitalWrite(SW_pin, HIGH);
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

}

void loop() {
  showSplashScreen();
  initializeNewGame();
  while (!isGameOver){
    if (millis() - lastUpdate > update_interval){
      updateSnake();
      drawGame();
      lastUpdate = millis();
    }
    handleInput();
  }
}

void handleInput(){
  int x = analogRead(X_pin);
  int y = analogRead(Y_pin);

  if (x < 400 && snakeDirection != RIGHT) nextSnakeDirection = LEFT;
  else if (x > 600 && snakeDirection != LEFT) nextSnakeDirection = RIGHT;
  else if (y < 400 && snakeDirection != DOWN) nextSnakeDirection = UP;
  else if (y > 600 && snakeDirection != UP) nextSnakeDirection = DOWN;
}

void initializeNewGame(){
  // initialize the snake
  isGameOver = false;
  snakeLength = MIN_SNAKE_SIZE;
  update_interval = BASE_DELAY;

  snake[0].x = 5;
  snake[0].y = 3;

  snake[1].x = 4;
  snake[1].y = 3;

  snake[2].x = 3;
  snake[2].y = 3;

  generateFood();
  snakeDirection = RIGHT;
  nextSnakeDirection = RIGHT;
}

void updateSnake() {
  // move the snake
  for (int i = snakeLength - 1; i > 0; i--){
    snake[i] = snake[i - 1];
  }

  switch (nextSnakeDirection){
    case UP: 
      snake[0].y--;
      break;
    case DOWN: 
      snake[0].y++;
      break;
    case LEFT: 
      snake[0].x--;
      break;
    case RIGHT: 
      snake[0].x++;
      break;
  }
  snakeDirection = nextSnakeDirection;

  // check for collisions
  if (snake[0].x < 0 || snake[0].x >= GRID_WIDTH || snake[0].y < 0 || snake[0].y >= GRID_HEIGHT){
    gameOver();
  }

  for (int i = 1; i < snakeLength; i++){
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y){
      gameOver();
    }
  }

  // Check for food
  if (snake[0].x == food.x && snake[0].y == food.y){
    snake[snakeLength].x = snake[snakeLength-1].x;
    snake[snakeLength].y = snake[snakeLength-1].y;
    snakeLength++;
    update_interval = max(BASE_DELAY - ((snakeLength - MIN_SNAKE_SIZE) * SPEED_INCREMENT), MIN_DELAY);
    if (snakeLength >= MAX_SNAKE_SIZE) {
      isGameOver = true;
      showWinSplashScreen();
    }
    else{
      generateFood();
    }
    
  }
}

void drawGame(){
  display.clearDisplay();

  // draw border
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

  // draw snake
  for (int i = 0; i < snakeLength; i++){
    display.fillRect(snake[i].x * GRID_SIZE + 1, snake[i].y * GRID_SIZE + 1, GRID_SIZE - 2, GRID_SIZE - 2, SSD1306_WHITE);
  }

  // draw food
  //display.fillRect(food.x * GRID_SIZE, food.y * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  display.fillCircle(food.x * GRID_SIZE + GRID_SIZE/2, food.y * GRID_SIZE + GRID_SIZE/2, GRID_SIZE/4, SSD1306_WHITE);

  display.display();
}

void generateFood(){
  Serial.println("generating food");
  food.x = random(0,GRID_WIDTH);
  food.y = random(0,GRID_HEIGHT);

  // Ensure food doesn't spawn on the snake
  for (int i = 0; i < snakeLength; i++){
    if (food.x == snake[i].x && food.y == snake[i].y){
      Serial.println("food spawned on snake, retrying");
      generateFood();
      return;
    }
  }
}

void gameOver(){
  delay(1000);
  isGameOver = true;
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.println("GAME OVER");
  display.print("SCORE: ");
  display.println(snakeLength - MIN_SNAKE_SIZE);
  display.display();
  if (snakeLength - MIN_SNAKE_SIZE > EEPROM.read(SCORE_ADDRESS)){
    EEPROM.write(SCORE_ADDRESS,snakeLength - MIN_SNAKE_SIZE);
  }
  
  delay(2000);
}

void showSplashScreen() {
  // Clear the display
  display.clearDisplay();

  // Draw a border for the splash screen
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

  // Display the title
  display.setTextSize(2);  // Large text for the title
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 10); // Adjust position
  display.println("SNAKE");

  // draw a simple snake
  for (int i = 0; i < 8; i++) {
    display.fillRect(20 + (i * 5), 35, 5, 5, SSD1306_WHITE);
  }

  display.setTextSize(1);
  display.setCursor(15, 45);
  display.print("High Score: ");
  display.println(EEPROM.read(SCORE_ADDRESS));
  display.setCursor(15, 55);
  display.println("Press to start!");

  // Display everything on the screen
  display.display();

  // Wait for the player to press the joystick button
  while (digitalRead(SW_pin) == HIGH) {
    delay(100); // Avoid rapid polling
  }
}

void showWinSplashScreen() {
  display.clearDisplay();

  if (snakeLength - MIN_SNAKE_SIZE > EEPROM.read(SCORE_ADDRESS)){
    EEPROM.write(SCORE_ADDRESS,snakeLength - MIN_SNAKE_SIZE);
  }

  // Draw border
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

  // Display "You Win!" text
  display.setTextSize(2); // Large text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 10);
  display.print("YOU WIN!");

  // draw a simple snake
  for (int i = 0; i < 8; i++) {
    display.fillRect(20 + (i * 5), 35, 5, 5, SSD1306_WHITE);
  }

  display.setTextSize(1);
  display.setCursor(15, 45);
  display.print("Score: ");
  display.println(snakeLength - MIN_SNAKE_SIZE);
  display.setCursor(15, 55);
  display.println("Snake Master!");

  // Show the updated display
  display.display();

  // Hold the screen for a few seconds
  delay(5000); // Adjust delay as needed
  display.clearDisplay();
  display.display();
}
