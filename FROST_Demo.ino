/*
* Florida Research Open-source Synchronization Tool (FROST)
* Firmware Example
* (c) 2020
* This code is licensed under MIT license (see LICENSE file for details)
*
* Description: This code provides information of a basic application of FROST.
* The Keyboard functionality allows to generate PC keyboard actions along with 
* the virtual serial port (COM port) capaility. In this example we use serial
* port for debugging purposes, we recommend not to use together (keyboard and 
* serial port). The main loop works as a state machine, to avoid the use of the 
* sleep function we use static long counters, some flags, and the millis() 
* function
* 
* Authors: Jose Alcantara & Robert Eisinger
* Brain Mapping Lab, University of Florida
* May 2020
* 
*/

#include <Keyboard.h>
#define PC_BRATE      57600   //Baud rate for serial port communication
#define PR_PORT_O_0   7       // Pressure or Heartrate output bit 0 (LSB)
#define PR_PORT_O_1   8       // Pressure or HR output bit 1
#define PR_PORT_O_2   6       // Pressure or HR output bit 2
#define PR_PORT_O_3   9       // Pressure or HR output bit 3
#define PR_PORT_O_4   4       // Pressure or HRoutput bit 4 
#define PR_PORT_O_5   10      // Pressure or HR output bit 5 (MSB) (Note: Added in this version, previously Digital Button output)
#define DE_PORT_O     11      // Delsys EMG digital output
#define ID_PORT_O     13      // LED state Indicator (Sensor states) digital output
#define LI_PORT_O     3       // Light sensor digital output
#define CA_PORT_O     5       // Cammera LED trigger digital output
#define PD_PORT_I     A0      // Peripheral device analog input
#define HR_PORT_I     A1      // Heart rate analog input
#define PT_PORT_I     A2      // Peripheral device type analog input (e.g. Keyboard vs Pressure button)
#define LI_PORT_I     A4      // Light sensor analog input
#define BU_PORT_I     12      // Button digital input

/************* LED INDICATOR INFORMATION *********************** 
* The LED state indicator has a 1.2 second divided in 6 steps 
* of 200ms offering 32 posible combinations for blinking patterns
*/
#define LED_PERIOD  1200      // In ms
#define LED_STEP    200       // In ms

/************* LIGHT SENSOR INFORMATION *********************** 
* This thresholds has been claculated over a LCD monitor over a
* square that changes from black to white. The values are the  
* number obtained from the ADC (10 bits)
*/
#define LIGHT_UNPLUG_THRESHOLD  800   // Over this threshold the sensor is not connected
#define LIGHT_BLACK_THRESHOLD   320   // Over this threshold up to the previous one the sensor is over the black square 
#define LIGHT_WHITE_THRESHOLD   10    // Over this threshold up to the previous one the sensor is over the white square 

/************* PERIPHERAL DEVICE IDENTIFICATION INFORMATION *********************** 
* This ADC values indicate which peripheral device is being used and change the functionality
* of FROST in te loop process. More devices can be added depending Program Memory Capacity 
*/
#define PERIPHERAL_ID_RANGE         10    // Range in which the resistor may vary 
#define PERIPHERAL_ID_PUSH_BUTTON   165   // ADC value for the push button sensor
#define PERIPHERAL_ID_KEYBOARD      200   // ADC value for the keyboard

/************* PUSH BUTTON INFORMATION *********************** 
* Definitions for the push button sensor
*/
#define PUSH_BUTTON_MIN_ADC         100   // Minimum value at obtained at maximum pressure 
#define PUSH_BUTTON_BOUNCE_TIME     50    // Time in ms to read an stable value from the sensor
#define PUSH_BUTTON_PRESS_THRESHOLD 19    // This value defines when a presure is a valid activation

/************* KEYBOARD INFORMATION *********************** 
* Definitions for the keyboard
*/
#define KEYBOARD_RANGE      10    // Range in which the resistors in the Keyboard may vary 
#define KEYBOARD_RED_VAL    330   // ADC value when the red button is pressed
#define KEYBOARD_GREEN_VAL  213   // ADC value when the green button is pressed
#define KEYBOARD_BLUE_VAL   111   // ADC value when the blue button is pressed

/************* FROST ANALOG INPUTS *********************** 
* Definitions for variables used independent of the 
* peripheral device
*/
int light_adc;              // ADC value of the light sensor
int heartrate_adc;          // ADC value of the heart rate sensor 
int peripheral_id_adc;      // ADC value of the peripheral ID
int peripheral_value_adc;   // ADC value of the peirpheral analog input

/************* TASK VARIABLES *********************** 
*  Variables to manage FROST in the main loop
*/
bool DEBUG = false;                           // In Debug mode on the values are sent by serial port
enum peripheral {NONE,PUSH_BUTTON,KEYBOARD};  // Declaring this enum with every device used makes clearer to read the main loop
peripheral PERIPHERAL_DEVICE = NONE;          // The default value is none peripheral device connected
int peripheral_value = 0;                     // Digitized value in 6 or 5 bit form convertion to this value depends on the type of task

//************* FUNCTION DECLARATIONS *********************** 
void initPorts();
inline void readAnalogInputs();
inline void checkPeripheral();
inline void peripheralADCEncoder();
inline void writeLightSync();
inline void checkDebug();
inline void alarmPeripheral();
inline void processHeartRate();
inline void processPushButton();
inline void processKeyboard();

//************* ARDUINO INITIALIZATION *********************** 
void setup() {
  initPorts();  
  Serial.begin(PC_BRATE);         // Serial Port Communication
  Keyboard.begin();               // Initialize USB Keyboard fucntionality
  digitalWrite(ID_PORT_O,HIGH);   // Turn on Indicator LED
}

//************* ARDUINO MAIN *********************** 
void loop() {
 
  readAnalogInputs();       
  writeLightSync();
  checkPeripheral();
  switch(PERIPHERAL_DEVICE) {
    case NONE:
      processHeartRate();   // When none device peripheral is detected the heart rate is sent
      break;
    case PUSH_BUTTON:
      processPushButton();
      break;
    case KEYBOARD:
      processKeyboard();
      break;
     default:
      break;
  }
  peripheralADCEncoder();   // It will use any value in peripheral_value variable and econde using 5 or 6 bits
  alarmPeripheral();        // Alarms will change the LED indicator pattern
  checkDebug();             // If Debug mode on, the analog values are sent through the serial port

}

/******************************************* 
******* GENERAL FUNCTION DEFINITIONS *******
********************************************/

/*******************************************************
*  iniPorts: Initializes digital input/outputs 
********************************************************/
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

/**********************************************************
*  readAnalogInputs: Reads analog inputs from the sensors
***********************************************************/
inline void readAnalogInputs() {
  light_adc = analogRead(LI_PORT_I);
  heartrate_adc = analogRead(HR_PORT_I);
  peripheral_id_adc = analogRead(PT_PORT_I);
  peripheral_value_adc = analogRead(PD_PORT_I);
}

/**********************************************************
*  checkPeripheral: Reads the resistor ID value and checks
*  the type of peripheral device
***********************************************************/
inline void checkPeripheral() {
  if(peripheral_id_adc < PERIPHERAL_ID_PUSH_BUTTON + PERIPHERAL_ID_RANGE && peripheral_id_adc > PERIPHERAL_ID_PUSH_BUTTON - PERIPHERAL_ID_RANGE) 
    PERIPHERAL_DEVICE = PUSH_BUTTON;
  else if(peripheral_id_adc < PERIPHERAL_ID_KEYBOARD + PERIPHERAL_ID_RANGE && peripheral_id_adc > PERIPHERAL_ID_KEYBOARD - PERIPHERAL_ID_RANGE)
    PERIPHERAL_DEVICE = KEYBOARD;
  else
    PERIPHERAL_DEVICE = NONE;
}

/***********************************************************
*  checkDebug: Checks if a serial command has been received
*  character 'D' to activated and 'E' to deactivated
************************************************************/
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
    Serial.print(light_adc);
    Serial.print(" ");
    //Serial.print((heartrate_adc>>5)&0x1F);
    //Serial.print(" ");
    Serial.print(peripheral_id_adc);
    Serial.print(" ");
    Serial.println(peripheral_value_adc); 
    Serial.print(heartrate_adc);
    Serial.print(" ");
    Serial.print(peripheral_value_adc);
    Serial.print(" ");
    Serial.println(peripheral_value);
    //Serial.println(light_adc > LIGHT_BLACK_THRESHOLD);
  }
}

/***************************************************************
*  peripheralADCEncoder: Converts very bit in peripheral_value 
*  and send to the digital output from MSB to LSB
****************************************************************/
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

/*********************************************************
*  writeLightSync: Converts the light analog input to a 
*  digital output based on LIGHT_BLACK_THRESHOLD
**********************************************************/
inline void writeLightSync() {
  digitalWrite(LI_PORT_O,light_adc > LIGHT_BLACK_THRESHOLD);
  digitalWrite(DE_PORT_O,light_adc > LIGHT_BLACK_THRESHOLD);
}

/*******************************************************
*  alarmPeripheral: The alarms can be modified depending 
*  if the sensors are connected
********************************************************/
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

/******************************************** 
******* SPECIFIC FUNCTION DEFINITIONS *******
*********************************************/

/*******************************************************
*  processHeartRate: converts the hart rate value in a 
*  6 bit data
********************************************************/
inline void processHeartRate() {
  peripheral_value = (heartrate_adc >> 4) & 0x3F;
}

/*****************************************************************
*  processPushButton: Detects whe the push button is activated
*  giving some delay for bouncing stabilization 
******************************************************************/
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

/**********************************************************
*  processKeyboard: Detects any of the three keys has been
*  activated and after the bouncing delay the value is read
*  and transmited to the computer and the acquisiton system
***********************************************************/
inline void processKeyboard() { 
  static bool keyboard_on = false;
  static unsigned long keyboard_bounce_timer;
  static bool keyboard_wait = false;

  if(keyboard_on) {
    if(keyboard_wait) {
      if(peripheral_value_adc > KEYBOARD_RED_VAL + KEYBOARD_RANGE)
        keyboard_on = false;
    } else {
      if((millis() - keyboard_bounce_timer) > PUSH_BUTTON_BOUNCE_TIME ) {
        if(peripheral_value_adc < KEYBOARD_RED_VAL + KEYBOARD_RANGE && peripheral_value_adc > KEYBOARD_RED_VAL - KEYBOARD_RANGE) {
          Keyboard.print('R');
          peripheral_value = 0x01;            
        } else if(peripheral_value_adc < KEYBOARD_GREEN_VAL + KEYBOARD_RANGE && peripheral_value_adc > KEYBOARD_GREEN_VAL - KEYBOARD_RANGE) {
          Keyboard.print('G');
          peripheral_value = 0x02;       
        } else if(peripheral_value_adc < KEYBOARD_BLUE_VAL + KEYBOARD_RANGE && peripheral_value_adc > KEYBOARD_BLUE_VAL - KEYBOARD_RANGE) {
          Keyboard.print('B');
          peripheral_value = 0x04;       
        }
        keyboard_wait = true;
      }
    }
  } else if(peripheral_value_adc < KEYBOARD_RED_VAL + KEYBOARD_RANGE) {
    keyboard_on = true;
    keyboard_wait = false;
    keyboard_bounce_timer = millis();
  } else {
    peripheral_value = 0;
  }
}
