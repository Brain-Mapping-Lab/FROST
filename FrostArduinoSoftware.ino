//make the delay based on the button release, not the button press             
#include <Keyboard.h>
#define PC_BRATE      57600
#define PR_PORT_O_0   7   // Pressure or Heartrate output bit 0 (LSB)
#define PR_PORT_O_1   8   // Pressure or HR output bit 1
#define PR_PORT_O_2   6   // Pressure or HR output bit 2
#define PR_PORT_O_3   9   // Pressure or HR output bit 3
#define PR_PORT_O_4   4   // Pressure or HRoutput bit 4 
#define PR_PORT_O_5   10  // Pressure or HR output bit 5 (MSB) (Note: Added in this version, previously Digital Button output)
#define DE_PORT_O     11  // Delsys EMG digital output
#define ID_PORT_O     13  // LED state Indicator (Sensor states) digital output
#define LI_PORT_O     3   // Light sensor digital output
#define CA_PORT_O     5   // Cammera LED trigger digital output
#define PD_PORT_I     A0  // Peripheral device analog input
#define HR_PORT_I     A1  // Heart rate analog input
#define PT_PORT_I     A2  // Peripheral device type analog input (e.g. Keyboard vs Pressure button)
#define LI_PORT_I     A4  // Light sensor analog input
#define BU_PORT_I     12  // Button digital input

//*************LED INDICATOR INFORMATION*********************** 
#define LED_PERIOD  1200
#define LED_STEP    200

//*************LIGHT SENSOR INFORMATION*********************** 
#define LIGHT_UNPLUG_THRESHOLD  800
#define LIGHT_BLACK_THRESHOLD   320
#define LIGHT_WHITE_THRESHOLD   10

//*************PERIPHERAL DEVICE IDENTIFICATION INFORMATION*********************** 
#define PERIPHERAL_ID_RANGE         10
#define PERIPHERAL_ID_PUSH_BUTTON   165   //Robert's PhD project
#define PERIPHERAL_ID_KEYBOARD      200   //Sarah's PhD project

//*************PUSH BUTTON INFORMATION (ROBERT'S PHD PROJECT)*********************** 
#define PUSH_BUTTON_MIN_ADC         100 //
#define PUSH_BUTTON_BOUNCE_TIME     50
#define PUSH_BUTTON_PRESS_THRESHOLD 19

//*************KEYBOARD INFORMATION (SARAH'S PHD PROJECT)*********************** 
#define KEYBOARD_RANGE 10
#define KEYBOARD_F_VAL 520
#define KEYBOARD_G_VAL 330
#define KEYBOARD_H_VAL 213
#define KEYBOARD_J_VAL 111
#define KEYBOARD_K_VAL 55

//*************ROBERT BOARD ANALOG INPUTS*********************** 
int light_adc;
int heartrate_adc;
int peripheral_id_adc;
int peripheral_value_adc;

//*************TASK VARIABLES*********************** 
bool DEBUG = false;
enum peripheral {NONE,PUSH_BUTTON,KEYBOARD}; 
peripheral PERIPHERAL_DEVICE = NONE;
int peripheral_value = 0; // Encoded 5 bit converion from peripheral analog input 

//*************FUNCTION DECLARATIONS*********************** 
void initPorts();
inline void readAnalogInputs();
inline void checkPeripheral();
inline void peripheralADCEncoder();
inline void writeLightSync();
inline void checkDebug();
inline void alarmPeripheral();

inline void processHeartRate();
inline void processPushButton();

//*************ARDUINO INITIALIZATION*********************** 
void setup() {
  initPorts();  
  Serial.begin(PC_BRATE);
  Keyboard.begin();
  digitalWrite(ID_PORT_O,HIGH);
}


//*************ARDUINO MAIN*********************** 
void loop() {
 
  readAnalogInputs();
  writeLightSync();
  checkPeripheral();
  switch(PERIPHERAL_DEVICE) {
    case NONE:
      processHeartRate();
      break;
    case PUSH_BUTTON:
      processPushButton();
      break;
    case KEYBOARD:
      //Implements Sarah's Task
      break;
     default:
      break;
  }
  //peripheral_value++;
  //delay(500);
  peripheralADCEncoder(); //It will use any value in peripheral_value variable and econde using 6 bits
  alarmPeripheral();
  checkDebug();


}

//************* GENERAL FUNCTION DEFINITIONS*********************** 
void initPorts() {
  pinMode(PR_PORT_O_0, OUTPUT);
  pinMode(PR_PORT_O_1, OUTPUT);
  pinMode(PR_PORT_O_2, OUTPUT);
  pinMode(PR_PORT_O_3, OUTPUT);
  pinMode(PR_PORT_O_4, OUTPUT);
  pinMode(PR_PORT_O_5, OUTPUT);
  pinMode(ID_PORT_O,   OUTPUT);
  pinMode(LI_PORT_O,   OUTPUT);
  pinMode(BU_PORT_I,   INPUT); 
  pinMode(DE_PORT_O,   OUTPUT); 
}

inline void readAnalogInputs() {
  light_adc = analogRead(LI_PORT_I);
  heartrate_adc = analogRead(HR_PORT_I);
  peripheral_id_adc = analogRead(PT_PORT_I);
  peripheral_value_adc = analogRead(PD_PORT_I);
}

inline void checkPeripheral() {
  if(peripheral_id_adc < PERIPHERAL_ID_PUSH_BUTTON + PERIPHERAL_ID_RANGE && peripheral_id_adc > PERIPHERAL_ID_PUSH_BUTTON - PERIPHERAL_ID_RANGE) 
    PERIPHERAL_DEVICE = PUSH_BUTTON;
  else if(peripheral_id_adc < PERIPHERAL_ID_KEYBOARD + PERIPHERAL_ID_RANGE && peripheral_id_adc > PERIPHERAL_ID_KEYBOARD - PERIPHERAL_ID_RANGE)
    PERIPHERAL_DEVICE = KEYBOARD;
  else
    PERIPHERAL_DEVICE = NONE;
}

void checkDebug() {
  if(Serial.available()>0) {  //TS = 7us
    byte ser = (byte)Serial.read();
    if(ser == 'D') {
      DEBUG = true;      
    } else if(ser == 'E') {
      DEBUG = false;      
    }      
  }
  if(DEBUG) {
    /*Serial.print(light_adc);
    Serial.print(" ");
    Serial.print((heartrate_adc>>5)&0x1F);
    Serial.print(" ");
    Serial.print(peripheral_id_adc);
    Serial.print(" ");
    Serial.println(peripheral_value_adc); */
    Serial.print(heartrate_adc);
    Serial.print(" ");
    Serial.print(peripheral_value_adc);
    Serial.print(" ");
    Serial.println(peripheral_value);
    //Serial.println(light_adc > LIGHT_BLACK_THRESHOLD);
  }
}

inline void peripheralADCEncoder() {
  
  if(peripheral_value & 0x20) {
    digitalWrite(PR_PORT_O_5,HIGH);
  } else {
    digitalWrite(PR_PORT_O_5,LOW);
  }
   
  if(peripheral_value & 0x10) {
    digitalWrite(PR_PORT_O_4,HIGH);
  } else {
    digitalWrite(PR_PORT_O_4,LOW);
  }

  if(peripheral_value & 0x08) {
    digitalWrite(PR_PORT_O_3,HIGH);
  } else {
    digitalWrite(PR_PORT_O_3,LOW);
  }

  if(peripheral_value & 0x04) {
    digitalWrite(PR_PORT_O_2,HIGH);
  } else {
    digitalWrite(PR_PORT_O_2,LOW);
  }

  if(peripheral_value & 0x02) {
    digitalWrite(PR_PORT_O_1,HIGH);
  } else {
    digitalWrite(PR_PORT_O_1,LOW);
  }

  if(peripheral_value & 0x01) {
    digitalWrite(PR_PORT_O_0,HIGH);
  } else {
    digitalWrite(PR_PORT_O_0,LOW);
  }  
}

inline void writeLightSync() {
  digitalWrite(LI_PORT_O,light_adc > LIGHT_BLACK_THRESHOLD);
  digitalWrite(DE_PORT_O,light_adc > LIGHT_BLACK_THRESHOLD);
}

inline void alarmPeripheral() {
  static unsigned long led_counter = 0;
  bool alarm_light = light_adc > LIGHT_UNPLUG_THRESHOLD;


  if(PERIPHERAL_DEVICE != NONE) {
    if(alarm_light) { 
      if(millis() - led_counter > LED_PERIOD) {
        led_counter = millis();
      } else {
        if(millis() - led_counter < 3*LED_STEP)
          digitalWrite(ID_PORT_O,HIGH);
        else
          digitalWrite(ID_PORT_O,LOW);
      }
    } else {
      digitalWrite(ID_PORT_O,HIGH);
      led_counter = millis();
    }
  } else { 
    if(millis() - led_counter > LED_PERIOD) {
      led_counter = millis();
    } else {
      if(alarm_light) {
        if(millis() - led_counter < LED_STEP)
          digitalWrite(ID_PORT_O,HIGH);
        else if(millis() - led_counter < 2*LED_STEP) 
          digitalWrite(ID_PORT_O,LOW);
        else if(millis() - led_counter < 3*LED_STEP) 
          digitalWrite(ID_PORT_O,HIGH);
        else 
          digitalWrite(ID_PORT_O,LOW);
      } else
        digitalWrite(ID_PORT_O,LOW);
    }
  } 
}

//************* SPECIFIC FUNCTION DEFINITIONS*********************** 
inline void processHeartRate() {
  peripheral_value = (heartrate_adc >> 4) & 0x3F;
}

inline void processPushButton() {
  static bool light_button_on = false;
  static unsigned long lbutton_bounce_timer;
  static bool push_button_on = false;
  static unsigned long pbutton_bounce_timer;
  
  peripheral_value = (peripheral_value_adc - PUSH_BUTTON_MIN_ADC) >> 3;
  if(peripheral_value > 31)
    peripheral_value = 31;
  else if (peripheral_value_adc < PUSH_BUTTON_MIN_ADC)
    peripheral_value = 0;

  if(light_button_on) {
    if((millis() - lbutton_bounce_timer) > PUSH_BUTTON_BOUNCE_TIME && light_adc > LIGHT_WHITE_THRESHOLD)
      light_button_on = false;
  } else if(light_adc < LIGHT_WHITE_THRESHOLD) {
      Keyboard.print('n');
      light_button_on = true;
      lbutton_bounce_timer = millis();
  }

  if(push_button_on) {
    if((millis() - pbutton_bounce_timer) > PUSH_BUTTON_BOUNCE_TIME && peripheral_value > PUSH_BUTTON_PRESS_THRESHOLD)
      push_button_on = false;
  } else if(peripheral_value < PUSH_BUTTON_PRESS_THRESHOLD) {
      Keyboard.print(' ');
      push_button_on = true;
      pbutton_bounce_timer = millis();
  }
}














