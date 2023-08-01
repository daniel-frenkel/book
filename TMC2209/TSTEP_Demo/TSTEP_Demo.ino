#include <Arduino.h>
#include "FastAccelStepper.h"
#include <HardwareSerial.h>
#include <TMCStepper.h>
#include <SPI.h>

#define ENABLE_PIN       51
#define RXD2             17
#define TXD2             16
#define STALLGUARD       21         //Be sure to only use intrrupt pins!!
#define R_SENSE          0.11f      // R_SENSE for current calc.
#define DRIVER_ADDRESS   0b00       // TMC2209 Driver address according to MS1 and MS2

//Change these values to get different results
long  set_velocity = 30000;
int  set_current = 600;
int  set_stall = 20;        //Do not set the value too high or the TMC will not detect it. Start low and work your way up
int  set_tcools = 200;      //Watch the Serial monitor. Try to stall the motor with your fingers. Notice that is won't stall with LARGER than this value

bool stalled_motor = false;
int motor_microsteps = 64;

// We communicate with the TMC2209 over UART
// But the Arduino UNO only have one Serial port which is connected to the Serial Monitor
// We can use software serial on the UNO, and hardware serial on the ESP32 or Mega 2560
#if defined(ESP32) || defined(__AVR_ATmega2560__)
#define SERIAL_PORT_2    Serial2    // TMC2208/TMC2224 HardwareSerial port
#else
#include<SoftwareSerial.h>
SoftwareSerial SERIAL_PORT_2 (2, 3);   //DPin-2 will work as SRX-pin, and DPin-3 will work as STX-pin; S for soft
#endif

TMC2209Stepper driver(&SERIAL_PORT_2, R_SENSE , DRIVER_ADDRESS);

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

void setup() {
  Serial.begin(115200);
  #if defined(ESP32)
  SERIAL_PORT_2.begin(115200, SERIAL_8N1, RXD2, TXD2); // ESP32 can use any pins to Serial
  #else
  SERIAL_PORT_2.begin(115200);
  #endif
  
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(STALLGUARD , INPUT);
  attachInterrupt(digitalPinToInterrupt(STALLGUARD), stalled_position, RISING);
  driver.begin(); // Start all the UART communications functions behind the scenes
  driver.toff(4); //For operation with StealthChop, this parameter is not used, but it is required to enable the motor. In case of operation with StealthChop only, any setting is OK
  driver.blank_time(24); //Recommended blank time select value
  driver.I_scale_analog(false); // Disbaled to use the extrenal current sense resistors
  driver.internal_Rsense(false); // Use the external Current Sense Resistors. Do not use the internal resistor as it can't handle high current.
  driver.mstep_reg_select(true); //Microstep resolution selected by MSTEP register and NOT from the legacy pins.
  driver.microsteps(motor_microsteps); //Set the number of microsteps. Due to the "MicroPlyer" feature, all steps get converterd to 256 microsteps automatically. However, setting a higher step count allows you to more accurately more the motor exactly where you want.
  driver.TPWMTHRS(0); //DisableStealthChop PWM mode/ Page 25 of datasheet
  driver.semin(0); // Turn off smart current control, known as CoolStep. It's a neat feature but is more complex and messes with StallGuard.
  driver.shaft(true); // Set the shaft direction.
  driver.en_spreadCycle(false); // Disable SpreadCycle. We want StealthChop becuase it works with StallGuard.
  driver.pdn_disable(true); // Enable UART control
  driver.VACTUAL(0); //We use the internal pulse generator of the TMC2209 so that we can monitor the SG_RESULT
  driver.index_step(true); //Index pin output step pulses from internal pulse generator
  driver.rms_current(set_current);
  driver.SGTHRS(set_stall);
  driver.TCOOLTHRS(set_tcools);
  digitalWrite(ENABLE_PIN, LOW);
}

void loop()
{
  Serial.println(driver.TSTEP());
  for (int i = 0; i < set_velocity; i = i + 10)
  {
    driver.VACTUAL(i);
    Serial.println(driver.TSTEP());
    if (stalled_motor)
    {
      Serial.println("STALLED");
      stalled_motor = false;
    }
  }

  for (int i = set_velocity; i > 0; i = i - 10)
  {
    driver.VACTUAL(-i);
    Serial.println(driver.TSTEP());
    if (stalled_motor)
    {
      Serial.println("STALLED");
      stalled_motor = false;
    }
  }
}
