#include <Arduino.h>
#include "FastAccelStepper.h"
#include <HardwareSerial.h>
#include <TMCStepper.h>
#include <SPI.h>

/* STEP PIN SETUP: THESE PINS ONLY
   AVR ARmega328p: only Pin 9 and 10.
   AVR ARmega32u4: only Pin 9, 10 and 11.
   AVR ARmega2560: only Pin 6, 7 and 8.
   ESP32: This can be any output capable port pin.
   Atmel Due: This can be one of each group of pins: 34/67/74/35, 17/36/72/37/42, 40/64/69/41, 9, 8/44, 7/45, 6
*/
#define STEP_PIN         10
#define DIR_PIN          9
#define ENABLE_PIN       5
#define PIN_SPI_SCK      22
#define PIN_SPI_MISO     19
#define PIN_SPI_MOSI     23
#define CS_PIN           18
#define STALLGUARD       21
#define R_SENSE          0.075f      // R_SENSE for current calc.

//Change these values to get different results
long long  move_to_step = 500000; //Change this value to set the position to move to (Negative will reverse)
long  set_velocity = 20000;
int  set_accel = 1000;
int  set_current = 400;

// IF StallGuard does not work, it's because these two values are not set correctly or your pins are not correct.
int  set_stall = 5;      //-64 to 63 / Start at 0
long  set_tcools = 200;   // Set slightly higher than the max TSTEP value you see

bool stalled_motor = false;
int motor_microsteps = 2;
long long current_position = 0;

FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;
TMC5160Stepper driver = TMC5160Stepper(CS_PIN, R_SENSE, PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_SCK);

#if defined(ESP32) //ESP32 needs special "IRAM_ATTR" in interrupt
void IRAM_ATTR stalled_position()
{
  stalled_motor = true;
}
#else
void stalled_position()
{
  stalled_motor = true;
}
#endif

int sg_min_val = 1023;
int sg;

void setup() {
  Serial.begin(115200);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(STALLGUARD , INPUT);
  attachInterrupt(digitalPinToInterrupt(STALLGUARD), stalled_position, RISING);

  driver.begin();
  driver.toff(4);
  driver.blank_time(24);
  driver.en_pwm_mode(0); // Turn off StealthChop to work with StallGuard2
  driver.pwm_autoscale(1);
  driver.intpol(true);
  driver.rms_current(set_current); 
  driver.sgt(set_stall);
  driver.microsteps(64);
  driver.TCOOLTHRS(set_tcools);
  driver.diag0_stall(true); 
  driver.diag0_int_pushpull(true); //Diag goes HIGH on Stall
  driver.TPWMTHRS(0);
  driver.semin(0); 
  driver.chm(false);
  driver.VDCMIN(false);
  driver.sfilt(true);
  
  engine.init();
  stepper = engine.stepperConnectToPin(STEP_PIN);
  stepper->setDirectionPin(DIR_PIN);
  stepper->setEnablePin(ENABLE_PIN);
  stepper->setAutoEnable(false); // Auto enable causes problems with SG_RESULT monitoring.
  stepper->setSpeedInHz(set_velocity);
  stepper->setAcceleration(set_accel);
  stepper->setCurrentPosition(0);

  digitalWrite(ENABLE_PIN, LOW);// Auto enable causes problems with SG_RESULT monitoring on TMC5160.
  
}

void loop()
{
  
  stalled_motor = false;
  stepper->moveTo(move_to_step);
  while (stepper->getCurrentPosition() != stepper->targetPos())
  {
    
    Serial.print("TSTEP: ");
    Serial.println(driver.TSTEP()); //Check TSTEP value

    Serial.print("SG_RESULT: ");
    Serial.println(driver.sg_result()); //Check SG_RESULT value/ The set_stall value effects this
    
    if (stalled_motor == true)
    {
      Serial.println("STALLED");
      stalled_motor = false;
      delay(1000);
   }
  }

  stalled_motor = false;
  stepper->moveTo(0);
  while (stepper->getCurrentPosition() != stepper->targetPos())
  {
    
    Serial.print("TSTEP: ");
    Serial.println(driver.TSTEP()); //Check TSTEP value

    Serial.print("SG_RESULT: ");
    Serial.println(driver.sg_result()); //Check SG_RESULT value/ The set_stall value effects this
    
    if (stalled_motor == true)
    {
      Serial.println("STALLED");
      stalled_motor = false;
      delay(1000);
   }
  }
}
