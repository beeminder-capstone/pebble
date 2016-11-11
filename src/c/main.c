#include <pebble.h>

//dymzx9wu1097pfc1ab0ueeq5t

static Window *s_window;
static Layer *s_window_layer;
static TextLayer *s_time_layer, *date_layer, *s_step_layer, *text_layer;

static char s_current_time_buffer[16], current_date_buffer[16], s_current_steps_buffer[16], text_buffer[32];
static int s_step_count = 0;

GColor step_color;

static bool s_js_ready = false;

static bool step_data_available = false;

// Persistent storage key
#define SETTINGS_KEY 1
#define GOALS_KEY 2

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

typedef struct goal {
  int number;
  int timestamp[14];
  int value[14];
} goal;

static goal goals;

// Initialize the default settings
static void default_settings() {
  settings.config = false;
  //settings.config = true;
  strcpy(settings.access_token, "");
  strcpy(settings.username, "");
  strcpy(settings.goal, "");
  settings.healthmetric = 0;
  //settings.metric = HealthMetricStepCount;
  settings.s_step_goal = 0;
}

// Read settings from persistent storage
static void load_settings() {
  // Load the default settings
  default_settings();
  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Save the settings to persistent storage
static void save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void defaultgoals() {
  goals.number = 0;
}

static void loadgoals() {
  defaultgoals();
  persist_read_data(GOALS_KEY, &goals, sizeof(goals));
}

static void savegoals(){
  persist_write_data(GOALS_KEY, &goals, sizeof(goals));
}

static void savegoal(int timestamp, int value){
  if(goals.number < 14){
    goals.timestamp[goals.number] = timestamp;
    goals.value[goals.number] = value;
    ++goals.number;
    
    persist_write_data(GOALS_KEY, &goals, sizeof(goals));
  }
}

static void sendgoal(int timestamp, int value){
  // Begin dictionary
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);

  if(result == APP_MSG_OK) {
    // Add a key-value pair
    dict_write_cstring(iter, 2, settings.access_token);

    dict_write_cstring(iter, 3, settings.goal);

    dict_write_int32(iter, 4, timestamp);

    dict_write_int32(iter, 5, value);
    
    char datetime[32];
    
    time_t now = timestamp;
    struct tm *tick_time = localtime(&now);

    strftime(datetime, sizeof(datetime), clock_is_24h_style() ? "%a %b %d %H:%M" : "%a %b %d %l:%M %p", tick_time);
    
    dict_write_cstring(iter, 6, datetime);

    // Send the message!
    result = app_message_outbox_send();

    // Check the result
    if(result != APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the postgoal data: %d", (int)result);

      savegoal(timestamp, value);
    }

  } else {
    // The outbox cannot be used right now
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the postgoal data: %d", (int)result);

    savegoal(timestamp, value);
  }
}

static void number(int number, char * buffer){
  int thousands = number / 1000;
  int hundreds = number % 1000;

  if(thousands > 0) {
    snprintf(buffer, sizeof(buffer), "%d,%03d", thousands, hundreds);
  } else {
    snprintf(buffer, sizeof(buffer), "%d", hundreds);
  }
}

static void display_step_count() {
  static char count_buffer[16];
  static char goal_buffer[16];
  
  number(s_step_count, count_buffer);
    
  number(settings.s_step_goal, goal_buffer);
  
  snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer), "%s/%s", count_buffer, goal_buffer);
  
  #ifdef PBL_COLOR
  text_layer_set_text_color(s_step_layer, step_color);
  #endif

  text_layer_set_text(s_step_layer, s_current_steps_buffer);
  
  if(text_buffer[0] != '\0')
    text_layer_set_text(text_layer, text_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  bool initialize = false;
  
  if (!tick_time) {
    time_t now = time(NULL);
    tick_time = localtime(&now);
    
    initialize = true;
  }
  
  strftime(current_date_buffer, sizeof(current_date_buffer), "%a %b %d", tick_time);
  text_layer_set_text(date_layer, current_date_buffer);
  
  strftime(s_current_time_buffer, sizeof(s_current_time_buffer), clock_is_24h_style() ? "%H:%M" : "%l:%M %p", tick_time);
  text_layer_set_text(s_time_layer, s_current_time_buffer);
  
  if(s_js_ready && settings.config){
    if(goals.number > 0 && tick_time->tm_min % 10 == 0){
      int number = goals.number;
      int timestamp[14];
      int value[14];
      
      for(int i = 0; i < number; ++i){
        timestamp[i] = goals.timestamp[i];
        value[i] = goals.value[i];
      }
      
      goals.number = 0;
      savegoals();
      
      for(int i = 0; i < number; ++i)
        sendgoal(timestamp[i], value[i]);
    }
    
    // Get goal every 10 minutes
    if((initialize || tick_time->tm_min % 10 == 0) && step_data_available) {
      // Begin dictionary
      DictionaryIterator *iter;
      AppMessageResult result = app_message_outbox_begin(&iter);
      
      if(result == APP_MSG_OK) {
        // Add a key-value pair
        dict_write_cstring(iter, 0, settings.access_token);
        
        dict_write_cstring(iter, 1, settings.goal);
    
        // Send the message!
        result = app_message_outbox_send();
        
        // Check the result
        if(result != APP_MSG_OK) {
          APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the getgoal data: %d", (int)result);
        }
      
      } else {
        // The outbox cannot be used right now
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the getgoal data: %d", (int)result);
      }
    }
  }
}

// AppMessage receive handler
static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Assign the values to our struct
  Tuple *ready_tuple = dict_find(iter, MESSAGE_KEY_JSReady);
  Tuple *aaccess_token = dict_find(iter, MESSAGE_KEY_access_token);
  Tuple *ausername = dict_find(iter, MESSAGE_KEY_username);
  Tuple *agoal = dict_find(iter, MESSAGE_KEY_goal);
  Tuple *ametric = dict_find(iter, MESSAGE_KEY_metric);
  Tuple *arate = dict_find(iter, MESSAGE_KEY_rate);
  Tuple *aroadstatuscolor = dict_find(iter, MESSAGE_KEY_roadstatuscolor);
  Tuple *alimsum = dict_find(iter, MESSAGE_KEY_limsum);
  if(ready_tuple) {
    // PebbleKit JS is ready! Safe to send messages
    s_js_ready = true;
  }
  if (aaccess_token && ausername && agoal && ametric) {
    strcpy(settings.access_token, aaccess_token->value->cstring);
    strcpy(settings.username, ausername->value->cstring);
    strcpy(settings.goal, agoal->value->cstring);
    if(strcmp(ametric->value->cstring, "StepCount") == 0){
      settings.metric = HealthMetricStepCount;
      settings.healthmetric = 1;
    }
    else if(strcmp(ametric->value->cstring, "ActiveSeconds") == 0){
      settings.metric = HealthMetricActiveSeconds;
      settings.healthmetric = 2;
    }
    else if(strcmp(ametric->value->cstring, "SleepSeconds") == 0){
      settings.metric = HealthMetricSleepSeconds;
      settings.healthmetric = 3;
    }
    else if(strcmp(ametric->value->cstring, "WalkedDistance") == 0){
      settings.metric = HealthMetricWalkedDistanceMeters;
      settings.healthmetric = 4;
    }
    else if(strcmp(ametric->value->cstring, "ActiveKCalories") == 0){
      settings.metric = HealthMetricActiveKCalories;
      settings.healthmetric = 5;
    }
    
    settings.config = true;
    save_settings();
    
    goals.number = 0;
    savegoals();
    
    tick_handler(NULL, MINUTE_UNIT);
    
    if(app_worker_is_running()){
      // Stop the background worker
      /*AppWorkerResult result = */app_worker_kill();
    }
    
    app_worker_launch();
  }
  if(arate && aroadstatuscolor && alimsum){
    if(settings.s_step_goal != arate->value->int32){
      settings.s_step_goal = arate->value->int32;
      
      save_settings();
    }
    
    #ifdef PBL_COLOR
    if(strcmp(aroadstatuscolor->value->cstring, "Blue") == 0){
      step_color = GColorBlue;
    }
    else if(strcmp(aroadstatuscolor->value->cstring, "Green") == 0){
      step_color = GColorGreen;
    }
    else if(strcmp(aroadstatuscolor->value->cstring, "Orange") == 0){
      step_color = GColorOrange;
    }
    else if(strcmp(aroadstatuscolor->value->cstring, "Red") == 0){
      step_color = GColorRed;
    }
    #endif
    
    strcpy(text_buffer, alimsum->value->cstring);
    
    display_step_count();
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int)reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message send failed. Reason: %d", (int)reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

// Is step data available?
bool step_data_is_available() {
  return HealthServiceAccessibilityMaskAvailable &
    health_service_metric_accessible(HealthMetricStepCount, time_start_of_today(), time(NULL));
}

// Todays current step count
static void get_step_count() {
  s_step_count = (int)health_service_sum_today(settings.metric);
  
  if(settings.healthmetric == 2 || settings.healthmetric == 3)
    s_step_count /= 60;
}

static void worker_message_handler(uint16_t type, AppWorkerMessage *message) {
  if(type == SOURCE_BACKGROUND) {
    // Get the data, only if it was sent from the background
    if(settings.config && step_data_available){
      time_t now = time(NULL);
  
      get_step_count();
      
      if(s_js_ready)
        sendgoal(now, s_step_count);
      else
        savegoal(now, s_step_count);
    }
  }
}

static void health_handler(HealthEventType event, void *context) {
  if(settings.config){
    if(event == HealthEventSignificantUpdate) {
      get_step_count();
      display_step_count();
    }
    else if(event == HealthEventMovementUpdate && (settings.healthmetric == 1 || settings.healthmetric == 2 || settings.healthmetric == 4)) {
      get_step_count();
      display_step_count();
    }
    else if(event == HealthEventSleepUpdate && settings.healthmetric == 3) {
      get_step_count();
      display_step_count();
    }
  }
}

static void window_load(Window *window) {
  GRect window_bounds = layer_get_bounds(s_window_layer);

  // Create a layer to hold the current time
  s_time_layer = text_layer_create(GRect(0, (window_bounds.size.h / 2), window_bounds.size.w, 38));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(s_window_layer, text_layer_get_layer(s_time_layer));
  
  date_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(20, 10), window_bounds.size.w, 38));
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  layer_add_child(s_window_layer, text_layer_get_layer(date_layer));

  // Create a layer to hold the current step count
  s_step_layer = text_layer_create(GRect(0, (window_bounds.size.h / 2) - 38, window_bounds.size.w, 38));
  text_layer_set_text_color(s_step_layer, GColorWhite);
  text_layer_set_background_color(s_step_layer, GColorClear);
  text_layer_set_font(s_step_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_step_layer, GTextAlignmentCenter);
  layer_add_child(s_window_layer, text_layer_get_layer(s_step_layer));
  
  text_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE((window_bounds.size.h / 2) + 38, window_bounds.size.h - 38), window_bounds.size.w, 28));
  text_layer_set_text_color(text_layer, GColorWhite);
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(s_window_layer, text_layer_get_layer(text_layer));
  text_layer_set_text(text_layer, "Loading…");

  // Subscribe to health events if we can
  if(!settings.config)
    text_layer_set_text(text_layer, "Please Configure.");
  else{
    if(!s_js_ready)
      text_layer_set_text(text_layer, "Please Connect to Device.");
    
    display_step_count();
  }
  
  step_data_available = step_data_is_available();
  
  if(step_data_available) {
    health_service_events_subscribe(health_handler, NULL);
  }
}

static void window_unload(Window *window) {
  layer_destroy(text_layer_get_layer(s_time_layer));
  layer_destroy(text_layer_get_layer(s_step_layer));
}

void init() {
  load_settings();
  
  loadgoals();
  
  // Listen for AppMessages
  app_message_register_inbox_received(inbox_received_handler);
  //app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  //app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  app_message_open(128, 128);
  
  s_window = window_create();
  s_window_layer = window_get_root_layer(s_window);
  window_set_background_color(s_window, GColorBlack);

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  window_stack_push(s_window, true);
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  tick_handler(NULL, MINUTE_UNIT);
  
  if(settings.config){
    // Launch the background worker
    /*AppWorkerResult result = */app_worker_launch();
  }
  
  // Subscribe to get AppWorkerMessages
  app_worker_message_subscribe(worker_message_handler);
}

void deinit() {
	window_destroy(s_window);
	
	tick_timer_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
