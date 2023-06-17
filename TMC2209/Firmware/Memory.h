bool stalled_motor = false;
int current_position;
int max_steps;
int current;
int stall;
int accel;
int max_speed;
int tcools = (3089838.00 * pow(float(max_speed), -1.00161534)) * 1.5;
int motor_microsteps = 64;
int move_to_step = 0;
int move_to_percent = 0;
int set_zero = 0; // Set to 1 to set home position
bool run_motor = false;
int wifi_set;
bool wifi_button = false;
String ssid;
String pass;
int open_direction;

Preferences preferences;

void load_preferences() {

  Serial.println("LOADING PREFERENCES");

  wifi_set = preferences.getInt("wifi_set", 0);
  ssid = preferences.getString ("ssid", "NOT_SET");
  pass = preferences.getString ("pass", "NOT_SET");
  max_steps = preferences.getInt("max_steps", 300000);
  current = preferences.getLong("current", 1000);
  stall = preferences.getInt("stall", 10);
  accel = preferences.getInt("accel", 10000);
  max_speed = preferences.getInt("max_speed", 30000);
  open_direction =  preferences.getInt("open_dir", 0);
  
  Serial.println("FINISHED LOADING PREFERENCES");
}
