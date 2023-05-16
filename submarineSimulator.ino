#include<Uduino.h>
Uduino uduino("submarineController"); // Declare and name your object

//what about losing 2 bit of precision to store everything into a short (better handling for the arduino). 
//If we loose them we can just write a single packet of 6 bytes..
#define LOW_PRECISION_MODE

#define precision 10

#ifdef LOW_PRECISION_MODE 
  
  // 8 bit range (-128, 127) = 255 val
  #define range 128 
  #define dead_zone 10

#else 

  // 10 bit range (-512, 511) = 1023 val
  #define range 512
  #define dead_zone 25
  
#endif

#define make_joysticks_offesets(n) \
  short joystick##n##_x_offset = 0; \
  short joystick##n##_y_offset = 0; 

// Define the pins for the sensors
const int joystick1_x_pin = A0;
const int joystick1_y_pin = A1;
const int joystick2_x_pin = A3;
const int joystick2_y_pin = A4;
const int pot_pin         = A6;

const int button1_pin = 2;
const int button2_pin = 3;
const int button3_pin = 4;
const int button4_pin = 5;


//to reduce ram usage we can use just a sigle int = 32 bit to store the 10 + 10 + 10 = 30 bit values
//it makes easier to send the data over the serial since we reduce the size of the packet
//Example: instead of sending 9 * 32 bit (int) values, we can pack them to have a packet size of 2 * 32 bit for the joystick movement and 1 * 16 bit for other stuff
// = 10 bytes in total instead of 36 bytes
#ifdef LOW_PRECISION_MODE 

  typedef union {

    struct {
      short x : 8;
      short y : 8; 
    };

    short val;
  }joystick_data_packet;

#else 

  typedef union {

    struct {
      int x : 10;
      int y : 10;
      int unused : 12;
    };

    int val;
  }joystick_data_packet;
  
#endif

typedef union {

  struct {
    short pot  : 10;
    short btn1 : 1; 
    short btn2 : 1; 
    short btn3 : 1; 
    short btn4 : 1;
    short unused : 2; 
  };

  short val;
}other_data_packet;

joystick_data_packet joystick1;
joystick_data_packet joystick2;
other_data_packet other;

make_joysticks_offesets(1)
make_joysticks_offesets(2)

void CalibrateJoystick(short& x_val, short& y_val, const short jPin_x, const short jPin_y){

  const short n_sample = 20;

  const short max_range = 1<<precision; // bit = 1024 values
  const short min_range = 0;

  for (short i=0; i < n_sample; i++) {
    x_val += map(analogRead(jPin_x), min_range, max_range, -range, range);
    y_val += map(analogRead(jPin_y), min_range, max_range, -range, range);
  }
  
  x_val /= n_sample;
  y_val /= n_sample;
}

inline void InitPin(){
  // Set the pins for the buttons to input mode
  pinMode(button1_pin, INPUT);
  pinMode(button2_pin, INPUT);
  pinMode(button3_pin, INPUT);
  pinMode(button4_pin, INPUT);
}

void InitJoysticks(){
  InitPin();

  #define CalibrateJoystick(n) CalibrateJoystick(joystick##n##_x_offset, joystick##n##_y_offset, joystick##n##_x_pin, joystick##n##_y_pin)
  
  CalibrateJoystick(1);
  CalibrateJoystick(2);
}

void ReadController(){
  uduino.print(joystick1.val);
  uduino.print("|");
  uduino.print(joystick2.val); 
  uduino.print("|"); 
  uduino.println(other.val);
}

void setup()
{
  Serial.begin(9600);

  uduino.addCommand("r", ReadController);
  uduino.addInitFunction(InitJoysticks);
}

inline int map_input(const int val, const int offset){
  const int return_val = map(val, 0, (1<<precision), -range, (range-1)) - offset;
  return (return_val < dead_zone && return_val > -dead_zone) ? 0 : return_val;
}


void loop()
{
  uduino.update();
  
  //macro that helps us to easier to get the val from the analog read
  //arg: joystick number, axis (x || y)
  //returns: a mapped value of the joystick data
  #define getJoyData(num, which) map_input(analogRead(joystick##num##_##which##_pin), joystick##num##_##which##_offset)

  // Read the analog values from the joysticks and potentiometer
  joystick1 = {getJoyData(1, x), getJoyData(1, y)};
  joystick2 = {getJoyData(2, x), getJoyData(2, y)};

  //macro to convert the digitalRead values from 0-255 to 0-1
  #define digitalReadBit(x) (digitalRead(x) > 0 ? 1 : 0)
  
  other = {
    analogRead(pot_pin),
    digitalReadBit(button1_pin),
    digitalReadBit(button2_pin),
    digitalReadBit(button3_pin),
    digitalReadBit(button4_pin)
  };
}
