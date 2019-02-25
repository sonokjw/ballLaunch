#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <string.h>  //used for some string handling and processing.
#include <mpu9255_esp32.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
 
#define BACKGROUND TFT_GREEN
#define BALL_COLOR TFT_BLUE
 
const int DT = 40; //milliseconds
const uint8_t BUTTON_PIN1 = 16;
const uint8_t BUTTON_PIN2 = 5;

int EXCITEMENT = 1000;
uint32_t primary_timer; //main loop timer
 
//physics constants:
const float MASS = 1; //for starters
const int RADIUS = 5; //radius of ball
const float K_FRICTION = 0.15;  //friction coefficient
const float K_SPRING = 0.9;  //spring coefficient
 
bool pushed_last_time; //for finding change of button (using bool type...same as uint8_t)


//others
float x;
float y;
int state = 1;
int stat = 1;
//Make a ball class

class Ball {
  private:
    //state variables:
    float x_pos; //x position
    float y_pos; //y position
    float x_vel; //x velocity
    float y_vel; //y velocity
    float x_accel; //x acceleration
    float y_accel; //y acceleration

  public:
    //constructor
    Ball();
    //functions
    void step();
    void moveBall();
    void draw(int color);
    //accessors and mutators
    void reset();
    void setX_pos(float x_p){ x_pos = x_p; };
    void setY_pos(float y_p){ y_pos = y_p; };
    void setX_vel(float x_v){ x_vel = x_v; };
    void setY_vel(float y_v){ y_vel = y_v; };
    void setX_accel(float x_a){ x_accel = x_a; };
    void setY_accel(float y_a){ y_accel = y_a; };
    float getX_pos(){return x_pos; };
    float getY_pos(){return y_pos; };
    float getX_vel(){return x_vel; };
    float getY_vel(){return y_vel; };
    float getX_accel(){return x_accel; };
    float getY_accel(){return y_accel; };

    //boundary constants:
    const int LEFT_LIMIT = RADIUS; //left side of screen limit
    const int RIGHT_LIMIT = 127-RADIUS; //right side of screen limit
    const int TOP_LIMIT = RADIUS; //top of screen limit
    const int BOTTOM_LIMIT = 159-RADIUS; //bottom of screen limit
};

//ball list
int n; //number of balls -1
Ball ball1;
Ball ball2;
Ball ball3;
Ball ball4;
Ball balls[4] = {ball1, ball2, ball3, ball4};

MPU9255 imu; //imu object called, appropriately, imu


//-------------- Ball Functions -----------------
Ball::Ball(){
  x_pos = RADIUS;
  y_pos = RADIUS;
  x_vel = 0;
  y_vel = 0;
  x_accel = 0;
  y_accel = 0;
}

void Ball::step(){
  //update acceleration (from f=ma)
  x_accel = x_accel - K_FRICTION*x_vel/MASS;
  y_accel = y_accel - K_FRICTION*y_vel/MASS;
  //integrate to get velocity from current acceleration
  x_vel = x_vel + 0.001*DT*x_accel; //integrate, 0.001 is conversion from milliseconds to seconds
  y_vel = y_vel + 0.001*DT*y_accel; //integrate!!
  //
  moveBall(); //you'll write this from scratch!
}

void Ball::moveBall(){
  //your code here
  float next_x_pos = x_pos + 0.001*x_vel*DT;
  float next_y_pos = y_pos + 0.001*y_vel*DT;
  float diff;
  
  if(next_x_pos <= LEFT_LIMIT) {
    diff = LEFT_LIMIT - next_x_pos;
    x_pos = LEFT_LIMIT + diff*K_SPRING;
    x_vel = -x_vel*K_SPRING;
  }
  else if (next_x_pos >= RIGHT_LIMIT) {
    diff = next_x_pos - RIGHT_LIMIT;
    x_pos = RIGHT_LIMIT - diff*K_SPRING;
    x_vel = -x_vel*K_SPRING;
  }
  else {
    x_pos = next_x_pos;
  }
  if(next_y_pos <= TOP_LIMIT) {
    diff = TOP_LIMIT - next_y_pos;
    y_pos = TOP_LIMIT + diff*K_SPRING;
    y_vel = -y_vel*K_SPRING;
  }
  else if (next_y_pos >= BOTTOM_LIMIT) {
    diff = next_y_pos - BOTTOM_LIMIT;
    y_pos = BOTTOM_LIMIT - diff*K_SPRING;
    y_vel = -y_vel*K_SPRING;
  }
  else {
    y_pos = next_y_pos;
  }
}

void Ball::draw(int color = 1){
  if (color == 1) {
    tft.fillCircle(x_pos,y_pos,RADIUS,BALL_COLOR); //draw new ball location
  }
  else {
    tft.fillCircle(x_pos,y_pos,RADIUS,BACKGROUND);
  }
}

void Ball::reset(){
  setX_pos(0);
  setY_pos(0);
  setX_vel(0);
  setY_vel(0);
  setX_accel(0);
  setY_accel(0);
}

//--------------------------------------------------

// button1
void ballLaunch(uint8_t input){
  Serial.println(state);
  switch(state){
    case 1: //unpushed
      if (!input && n < 4) {
        balls[n].draw();
        state = 0;
      }
      break;
    case 0: //pushed
      balls[n].draw();
      if (input == 1) {
        int timer = millis();
        balls[n].setX_accel(x*EXCITEMENT);
        balls[n].setY_accel(y*EXCITEMENT);
        state = 0;
        n++;
        while(millis() - timer < 10);
       }
  }
}

// button2
void ballReset(uint8_t input){
  Serial.println(stat);
  switch(stat) {
    case 1:
      if (!input) {
        stat = 0;
      }
      break;
    case 0:
      if (input) {
        for (int i = 0; i < n; i++){
          balls[i].reset();
        }
      }
      break;
  }
}

//-------------------------------------------

void setup() {
  Serial.begin(115200); //for debugging if needed.
  pinMode(BUTTON_PIN1,INPUT_PULLUP);
  pinMode(BUTTON_PIN2,INPUT_PULLUP);
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(BACKGROUND);
  if (imu.setupIMU(1)){
    Serial.println("IMU Connected!");
  }else{
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  pushed_last_time = false;
  primary_timer = millis();

  //initialize ball
  n = 0;
  balls[0].draw();
}
 
void loop() {
  uint8_t input1 = digitalRead(BUTTON_PIN1);
  uint8_t input2 = digitalRead(BUTTON_PIN2);
  //Serial.println(String(input1) + " , " + String(input2));
  //draw circle in previous location of ball in color background (redraws minimal num of pixels, therefore is quick!)
  for(int i = 0; i < n; i){
    balls[i].draw(0);
  }
  //if button pushed *just* pushed down, inject random force into system
  //else, just run out naturally
  imu.readAccelData(imu.accelCount);//read imu
  x = imu.accelCount[0]*imu.aRes;
  y = imu.accelCount[1]*imu.aRes;
  ballLaunch(input1);
  ballReset(input2);
  for(int i = 0; i < n; i){
    balls[i].step();
    balls[i].draw();
  }
 
  while (millis()-primary_timer < DT); //wait for primary timer to increment
  primary_timer = millis();
}
