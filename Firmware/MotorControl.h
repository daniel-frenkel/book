//Uncomment this if using the additional Capacitive Buttons
//#define CAP_BUTTONS

uint16_t positionLabel;

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#define STEP_PIN  15
#define DIR_PIN  14
#define ENABLE_PIN 27
#define BUTTON1 23
#define BUTTON2 34
#define RXD2 16
#define TXD2 17
#define STALLGUARD 2
#define SENSOR1 32
#define SENSOR2 22
#define LED1 33
#define LED2 18

#define SERIAL_PORT_2    Serial2    // TMC2208/TMC2224 HardwareSerial port
#define DRIVER_ADDRESS   0b00       // TMC2209 Driver address according to MS1 and MS2
#define R_SENSE          0.10f      // R_SENSE for current calc.  

int brightness0 = 0;    // how bright the LED is
int brightness1 = 0;    // how bright the LED is
int fade0Amount = 15;    // how many points to fade the LED by
int fade1Amount = 15;

int button1Timer;
int button2Timer;
int waitButton1Timer;
int waitButton2Timer;
bool motorRunning;

int btn1Press;
int btn2Press;

int allowButtonTime;

FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;
TMC2209Stepper driver(&SERIAL_PORT_2, R_SENSE , DRIVER_ADDRESS);


void IRAM_ATTR button1pressed()
{
  #ifdef CAP_BUTTONS
  btn1Press = 1; //
  #else
  move_to_step = 0; //
  run_motor = true; //
  #endif
}

void IRAM_ATTR button2pressed()
{
  #ifdef CAP_BUTTONS
  btn2Press = 1;
  #else
  move_to_step = max_steps;
  run_motor = true;
  #endif
  
}

void IRAM_ATTR stalled_position()
{
  stalled_motor = true;
  stop_motor = true;
}

void IRAM_ATTR sensor_short()
{
  sensor2_trip = true;
}

void IRAM_ATTR sensor_long()
{
  sensor1_trip = true;
}

void IRAM_ATTR wifi_button_press()
{
  wifi_button = true;
}

void setZero()
{
  current_position = 0;
  stepper->setCurrentPosition(0);
  Serial.print("current_position: ");
  Serial.println(current_position);
  ESPUI.updateLabel(positionLabel, String(current_position));
  
}

void move_motor() {
  Serial.print("Current Position: ");
  Serial.println(current_position);

  Serial.print("Moving to Position: ");
  Serial.println(move_to_step);

  stepper->setCurrentPosition(current_position);

  stalled_motor = false;
  sensor1_trip = false;
  sensor2_trip = false;

  stepper->setAcceleration(accel);
  stepper->setSpeedInHz(max_speed);
  driver.rms_current(current);
  driver.SGTHRS(stall);
  driver.TCOOLTHRS(tcools);

  if (current_position == move_to_step)
  {
    Serial.println("ALREADY THERE!");
  }
  else if (move_to_step > current_position) // Open
  {

    Serial.println("Opening");
    stepper->moveTo(move_to_step);

    while (stepper->getCurrentPosition() != stepper->targetPos())
    {

      if (stalled_motor == true)
      {
        printf("Stalled\n");
        stepper->forceStop();
        break;
      }

      vTaskDelay(1);
    }
  }

  else if (move_to_step < current_position) //close
  {

    Serial.println("Closing");
    stepper->moveTo(move_to_step);

    while (stepper->getCurrentPosition() != stepper->targetPos())
    {

      if (stalled_motor == true) //we assume it's in the center
      {
        printf("Stalled\n");
        stepper->forceStop();
        break;
      }
      
      vTaskDelay(1);
    }
  } else
  {
    Serial.println("DO NOTHING!");
  }
  
  current_position = stepper->getCurrentPosition();
  printf("Motor Function Complete\n");
}

// put your setup code here, to run once:
void setup_motors() {

  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(STALLGUARD , INPUT);
  pinMode(WIFI_PIN , INPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(SENSOR1, INPUT);
  pinMode(SENSOR2, INPUT);
  SERIAL_PORT_2.begin(115200);

  driver.begin(); // Start all the UART communications functions behind the scenes
  driver.toff(4); //For operation with StealthChop, this parameter is not used, but it is required to enable the motor. In case of operation with StealthChop only, any setting is OK
  driver.blank_time(24); //Recommended blank time select value
  driver.I_scale_analog(false); // Disbaled to use the extrenal current sense resistors 
  driver.internal_Rsense(false); // Use the external Current Sense Resistors. Do not use the internal resistor as it can't handle high current.
  driver.mstep_reg_select(true); //Microstep resolution selected by MSTEP register and NOT from the legacy pins.
  driver.rms_current(current); // Set the current in milliamps.
  driver.SGTHRS(stall); //Set the stall value from 0-255. Higher value will make it stall quicker.
  driver.microsteps(motor_microsteps); //Set the number of microsteps. Due to the "MicroPlyer" feature, all steps get converterd to 256 microsteps automatically. However, setting a higher step count allows you to more accurately more the motor exactly where you want.
  driver.TCOOLTHRS(tcools); // Minimum speed at which point to turn on StallGuard. StallGuard does not work as very low speeds such as the beginning of acceleration so we need to keep it off until it reaches a reliable speed.
  driver.TPWMTHRS(0); //DisableStealthChop PWM mode/ Page 25 of datasheet
  driver.semin(0); // Turn off smart current control, known as CoolStep. It's a neat feature but is more complex and messes with StallGuard.

  if (open_direction == 1)
  {
    driver.shaft(true); // Set the shaft direction. 
  } else {
    driver.shaft(false); // Set the shaft direction. 
  }

  driver.en_spreadCycle(false); // Disable SpreadCycle. We want StealthChop becuase it works with StallGuard.
  driver.pdn_disable(true); // Enable UART control

  engine.init();
  stepper = engine.stepperConnectToPin(STEP_PIN);

  stepper->setDirectionPin(DIR_PIN);
  stepper->setEnablePin(ENABLE_PIN);
  stepper->setAutoEnable(true);

  attachInterrupt(STALLGUARD, stalled_position, RISING);
  attachInterrupt(WIFI_PIN, wifi_button_press, FALLING);
  attachInterrupt(BUTTON1, button1pressed, FALLING);
  attachInterrupt(BUTTON2, button2pressed, FALLING);
  attachInterrupt(SENSOR1, sensor_long, FALLING);
  attachInterrupt(SENSOR2, sensor_short, FALLING);

}

void setup_leds() {

  ledcAttachPin(LED1, 1); // assign a led pins to a channel
  ledcAttachPin(LED2, 0); // assign a led pins to a channel

  ledcSetup(0, 5000, 8); // 12 kHz PWM, 8-bit resolution
  ledcSetup(1, 5000, 8); // 12 kHz PWM, 8-bit resolution

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  ledcWrite(0, 0); // turn off LED
  ledcWrite(1, 0); // turn off LED
}
