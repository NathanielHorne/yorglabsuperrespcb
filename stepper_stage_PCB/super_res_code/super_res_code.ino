#define stepPin1  10
#define dirPin1   11
#define enPin1    12

#define stepPin2  9
#define dirPin2   8
#define enPin2    7

#define joy_in1   A1
#define joy_in2   A0

#define gate_switch1 4
#define gate_switch2 3

// Proven deadzone = 200
#define joy_deadzone 200

#define analog_in_max_range   880
#define analog_in_half_range  analog_in_max_range/2

#define startup_speed         700
#define slowest_speed         7000
#define second_fastest        3500
#define second_slowest        1453
#define fastest_speed         700

#define movement_zone         (analog_in_half_range-joy_deadzone)
#define limit_1               280
#define limit_2               360
#define limit_3               400

unsigned int joy_input1 = 0;
int ena1 = LOW;
int direction_bool1 = LOW;
unsigned long step_1_time_index = micros();
unsigned int step_1_time_tolerance = 0;
long num_steps1 = 0;
long placeholder = 0;
int off_track1 = LOW;

unsigned int joy_input2 = 0;
int ena2 = LOW;
int direction_bool2 = HIGH;
unsigned long step_2_time_index = micros();
unsigned int step_2_time_tolerance = 0;
long num_steps2 = 0;
int off_track2 = LOW;

void set_direction_bools();

void set_en_pins();

void startup();

void perception();

void joystick_direction(const int& in, int& dir, int& ena, const int& off_track);

unsigned int stepper_wait_time(const int& input);

void planning();

void move_stepper(const unsigned int& step_pin, unsigned long& time_index, const unsigned int& time_tolerance, const unsigned int& direction_bool, long& num_steps, const int& ena);

void action();

void setup() {
  Serial.begin(9600);
  
  pinMode(stepPin1, OUTPUT);
  pinMode(dirPin1, OUTPUT);
  pinMode(enPin1, OUTPUT);
  
  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2, OUTPUT);
  pinMode(enPin2, OUTPUT);

  pinMode(joy_in1, INPUT);
  pinMode(joy_in2, INPUT);

  pinMode(gate_switch1, INPUT);
  pinMode(gate_switch2, INPUT);
  
  set_en_pins();

  set_direction_bools();

//  startup();
}

void loop() {
  
  perception();

  planning();

  action();
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
  while(digitalRead(gate_switch1) == 0) {
    move_stepper(stepPin1, step_1_time_index, startup_speed, direction_bool1, num_steps1, ena1);
  }
  direction_bool1 = HIGH;
  set_direction_bools();
  while(digitalRead(gate_switch1) == 1) {
    move_stepper(stepPin1, step_1_time_index, startup_speed, direction_bool1, num_steps1, ena1);
  }
  num_steps1 = 0;
}

void perception() {
  joy_input1 = analogRead(joy_in1);
  joy_input2 = analogRead(joy_in2);

  off_track1 = digitalRead(gate_switch1);
  off_track2 = digitalRead(gate_switch2);
}

void joystick_direction(const int& in, int& dir, int& ena, const int& off_track) {
  if(off_track == LOW) {
    if (in < (analog_in_half_range - joy_deadzone)) {
      ena = LOW;
      dir = LOW;
    }
    else if (in > (analog_in_half_range + joy_deadzone)) {
      ena = LOW;
      dir = HIGH;
    }
    else {
      ena = HIGH;
    } 
  }
  else {
    if (in < (analog_in_half_range - joy_deadzone)) {
      ena = LOW;
      dir = HIGH;
    }
    else if (in > (analog_in_half_range + joy_deadzone)) {
      ena = LOW;
      dir = LOW;
    }
    else {
      ena = HIGH;
    }
    delayMicroseconds(200); 
  }
}

unsigned int stepper_wait_time(const int& input) {
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
  joystick_direction(joy_input1, direction_bool1, ena1, off_track1);
  step_1_time_tolerance = stepper_wait_time(joy_input1);
  
  joystick_direction(joy_input2, direction_bool2, ena2, off_track2);
  step_2_time_tolerance = stepper_wait_time(joy_input2);
}

void move_stepper(const unsigned int& step_pin, unsigned long& time_index, const unsigned int& time_tolerance, const unsigned int& direction_bool, long& num_steps, const int& ena) {
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
}
