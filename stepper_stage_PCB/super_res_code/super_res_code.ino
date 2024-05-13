#include <math.h>

// Stepper box 1 I/O pin definitions
#define stepPin1  10
#define dirPin1   11
#define enPin1    12

// Stepper box 2 I/O pin definitions
#define stepPin2  9
#define dirPin2   8
#define enPin2    7

// Dual axis joystick I/O definitions 
#define joy_in1   A1
#define joy_in2   A0

// Optical gate switches I/O definitions
#define gate_switch1 4
#define gate_switch2 3

// Proven deadzone = 200
#define joy_deadzone 200

#define analog_in_max_range   880
#define analog_in_half_range  analog_in_max_range/2

// How long the stepper should wait in microseconds between steps 
#define startup_speed         700
#define slowest_speed         7000
#define second_fastest        3500
#define second_slowest        1453
#define fastest_speed         700

// Any analog read value outside of this zone is automatically disregarded
#define movement_zone         (analog_in_half_range-joy_deadzone)

/* This defines different speed regions based on the joystick's angle
   This is based off of the joystick's angle from in the "middle" position
   The middle position is also 512 according to analogRead();
*/
#define limit_1               280
#define limit_2               360
#define limit_3               400

// Define the global variables for stepper 1 
unsigned int joy_input1 = 0;
int ena1 = LOW;
int direction_bool1 = LOW;
unsigned long step_1_time_index = micros();
unsigned int step_1_time_tolerance = 0;
int64_t num_steps1 = 0;
// These variables were supposed to be used during the initial startup sequence
// This seequence hasn't been perfected yet. 
int is_off_track1 = LOW;
int is_at_middle1 = LOW;

int midpoint = 0; 

// Define the global variables for stepper 2
unsigned int joy_input2 = 0;
int ena2 = LOW;
int direction_bool2 = HIGH;
unsigned long step_2_time_index = micros();
unsigned int step_2_time_tolerance = 0;
int64_t num_steps2 = 0;
int is_off_track2 = LOW;
int is_at_middle2 = LOW;

void get_joystick_inputs();

void get_is_off_tracks();

void get_back_on_track();

void set_direction_bools();

void set_en_pins();

void startup();

void perception();

void joystick_direction(const int& in, int& dir, int& ena, const int& off_track);

unsigned int stepper_wait_time(const int& input);

void planning();

void move_stepper(const unsigned int& step_pin, unsigned long& time_index, const unsigned int& time_tolerance, const unsigned int& direction_bool, int64_t& num_steps, const int& ena);

void action();

void print(uint64_t value);

unsigned int uint64_to_int(uint64_t value) {
  int return_value = 0;
  for (int i = 0; i < 2000; i++) {
    value /= 10;
  }
  return_value = value;
  return return_value;
}

void setup() {
  Serial.begin(9600);
  
  // Setup pins for stepper driver 1
  pinMode(stepPin1, OUTPUT);
  pinMode(dirPin1, OUTPUT);
  pinMode(enPin1, OUTPUT);
  
  // Setup pins for stepper driver 1
  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2, OUTPUT);
  pinMode(enPin2, OUTPUT);

  // Setup joystick inputs
  pinMode(joy_in1, INPUT);
  pinMode(joy_in2, INPUT);

  // Setup photodiode inputs
  pinMode(gate_switch1, INPUT);
  pinMode(gate_switch2, INPUT);
  
  // Enable stepper driver
  set_en_pins();

  // Set stepper driver directions
  set_direction_bools();

//  startup();
}

void loop() {
  // This is the core of the code. The Nano repeats this code indefinitely

  perception();

  planning();

  action();
}

void get_joystick_inputs() {
  joy_input1 = analogRead(joy_in1);
  joy_input2 = analogRead(joy_in2);
}

void get_is_off_tracks() {
  // Determines whether or not the photodiodes on each axis are triggered or not

  is_off_track1 = digitalRead(gate_switch1);
  is_off_track2 = digitalRead(gate_switch2);
}

void get_back_on_track() {
  /*
  This was an attempt to automate the process of getting the stage to recenter on
  the metal fins on a particular axis. However, this largely failed.
  */

  if(num_steps1 < 0 ){
    direction_bool1 = !direction_bool1;
  }

  if(is_off_track2) {
    direction_bool2 = !direction_bool2;
  }

  set_direction_bools();

  do {
    get_is_off_tracks();
    
    if(is_off_track1) {
      move_stepper(stepPin1, step_1_time_index, startup_speed*4, direction_bool1, num_steps1, ena1);
    }
    
    if(is_off_track2) {
      move_stepper(stepPin2, step_2_time_index, startup_speed*4, direction_bool2, num_steps2, ena2);
    }
  } while(is_off_track1 || is_off_track2);
}

void set_en_pins() {
  digitalWrite(enPin1, ena1);
  digitalWrite(enPin2, ena2);
}

void set_direction_bools() {
  digitalWrite(dirPin1, direction_bool1);
  digitalWrite(dirPin2, direction_bool2);
}

void startup() {
  do {
    get_is_off_tracks();
    
    if(!(is_off_track1)) {
      move_stepper(stepPin1, step_1_time_index, startup_speed, direction_bool1, num_steps1, ena1);
    }
    
    if(!(is_off_track2)) {
      move_stepper(stepPin2, step_2_time_index, startup_speed, direction_bool2, num_steps2, ena2);
    }
  } while(!(is_off_track1) || !(is_off_track2));

  get_back_on_track();

  num_steps1 = 0;
  num_steps2 = 0;
}

void perception() {

  get_joystick_inputs(); 

  get_is_off_tracks();
}

void joystick_direction(const int& in, int& dir, int& ena, const int& off_track) {
  if (in < (analog_in_half_range - joy_deadzone)) {
    // Movement enabled, moving in one direction
    ena = LOW;
    dir = LOW;
  }
  else if (in > (analog_in_half_range + joy_deadzone)) {
    // Movement enabled, moving in opposite direction
    ena = LOW;
    dir = HIGH;
  }
  else {
    // Movement disabled
    ena = HIGH;
  }
}

unsigned int stepper_wait_time(const int& input) {
  // The joystick angle determines the speed of the steppers.
  // This method assigns this speed.

  int scrubbed_input = abs(input - analog_in_half_range);
  if (scrubbed_input >= 0 && scrubbed_input < limit_1) {
    return slowest_speed;
  }
  else if (scrubbed_input >= limit_1 && scrubbed_input < limit_2) {
    return second_slowest; 
  }
  else if (scrubbed_input >= limit_2 && scrubbed_input < limit_3){
    return second_fastest;
  }
  else {
    return fastest_speed;
  }
}

void planning() {
  joystick_direction(joy_input1, direction_bool1, ena1, is_off_track1);
  step_1_time_tolerance = stepper_wait_time(joy_input1);
  
  joystick_direction(joy_input2, direction_bool2, ena2, is_off_track2);
  step_2_time_tolerance = stepper_wait_time(joy_input2);
}

void move_stepper(const unsigned int& step_pin, unsigned long& time_index, const unsigned int& time_tolerance, const unsigned int& direction_bool, int64_t& num_steps, const int& ena) {
  if ((micros() - time_index) > time_tolerance) {
    digitalWrite(step_pin, HIGH);
    digitalWrite(step_pin, LOW);
    if (ena == LOW) {
      if(direction_bool == HIGH) {
        num_steps++;
      }
      else if (direction_bool == LOW) {
        num_steps--;
      }
    }
    time_index = micros();
  }
}

void action() {
  set_en_pins();

  set_direction_bools();

  move_stepper(stepPin1, step_1_time_index, step_1_time_tolerance, direction_bool1, num_steps1, ena1);
  move_stepper(stepPin2, step_2_time_index, step_2_time_tolerance, direction_bool2, num_steps2, ena2); 

//  get_back_on_track();
}

void print(int64_t value){


    const int NUM_DIGITS    = log10(value) + 1;

    char sz[NUM_DIGITS + 1];
    
    sz[NUM_DIGITS] =  0;
    for ( size_t i = NUM_DIGITS; i--; value /= 10)
    {
        sz[i] = '0' + (value % 10);
    }
    
    Serial.println(sz);
}
