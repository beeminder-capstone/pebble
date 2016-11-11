#include <pebble_worker.h>

// Persistent storage key
#define SETTINGS_KEY 1

// Used to identify the source of a message
#define SOURCE_FOREGROUND 0
#define SOURCE_BACKGROUND 1

// Define our settings struct
typedef struct Settings {
  bool config;
  char access_token[32];
  char username[32];
  char goal[32];
  char healthmetric;
  HealthMetric metric;
  int s_step_goal;
} Settings;

// An instance of the struct
static Settings settings; 

// Initialize the default settings
static void default_settings() {
  settings.config = false;
  strcpy(settings.access_token, "");
  strcpy(settings.username, "");
  strcpy(settings.goal, "");
  settings.healthmetric = 0;
  settings.s_step_goal = 0;
}

// Read settings from persistent storage
static void load_settings() {
  // Load the default settings
  default_settings();
  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Is step data available?
bool step_data_is_available() {
  return HealthServiceAccessibilityMaskAvailable &
    health_service_metric_accessible(HealthMetricStepCount, time_start_of_today(), time(NULL));
}

static void worker_message_handler(uint16_t type, AppWorkerMessage *message) {
  if(type == SOURCE_FOREGROUND) {
    // Get the data, if it was sent from the foreground
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  if(settings.config && tick_time->tm_hour == 23 && tick_time->tm_min == 59 && step_data_is_available()){
    // Construct a message to send
    AppWorkerMessage message = {
      .data0 = 0
    };
    
    worker_launch_app();
    
    // Send the data to the foreground app
    app_worker_send_message(SOURCE_BACKGROUND, &message);
  }
}

static void prv_init() {
  load_settings();
  
  // Subscribe to get AppWorkerMessages
  app_worker_message_subscribe(worker_message_handler);
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void prv_deinit() {
  tick_timer_service_unsubscribe();
}

int main(void) {
  prv_init();
  worker_event_loop();
  prv_deinit();
}
