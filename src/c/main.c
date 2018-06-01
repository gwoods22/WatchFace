#include "pebble.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Credit link: https://www.sitepoint.com/how-to-add-images-into-your-pebble-watchfaces/

// 	APP_LOG(APP_LOG_LEVEL_DEBUG, "");

static BitmapLayer *meri_image_layer;
static GBitmap *meri_image;

static BitmapLayer *meri_bt_icon_layer;
static GBitmap *meri_bt_icon;

static BitmapLayer *meri_mute_icon_layer;
static GBitmap *meri_mute_icon;

static Window *s_main_window;
Layer *window_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;
uint32_t connect = 0;

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";

  snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  text_layer_set_text(s_battery_layer, battery_text);
}

static void handle_mute(Layer *window_layer, int icon_width) {		
	if (quiet_time_is_active()) {
  	meri_mute_icon_layer = bitmap_layer_create(GRect(0, 0, icon_width, icon_width));
		meri_mute_icon = gbitmap_create_with_resource(RESOURCE_ID_MUTE_ICON);	
		bitmap_layer_set_compositing_mode(meri_mute_icon_layer, GCompOpAssign);
		bitmap_layer_set_bitmap(meri_mute_icon_layer, meri_mute_icon);
		bitmap_layer_set_alignment(meri_mute_icon_layer, GAlignCenter);
		layer_add_child(window_layer, bitmap_layer_get_layer(meri_mute_icon_layer));
  } else {		
  	bitmap_layer_destroy(meri_mute_icon_layer);
	}
}

static void handle_bluetooth(bool connected) {
	
	if (connected) {
		persist_write_bool(connect, true);
		bitmap_layer_destroy(meri_bt_icon_layer);
	} else {
		if (persist_read_bool(connect)) {		
			vibes_double_pulse();
		}
		persist_write_bool(connect, false);
		meri_bt_icon_layer = bitmap_layer_create(GRect(114, 0, 30,30));
		meri_bt_icon = gbitmap_create_with_resource(RESOURCE_ID_BT_ICON);	
		bitmap_layer_set_compositing_mode(meri_bt_icon_layer, GCompOpAssign);
		bitmap_layer_set_bitmap(meri_bt_icon_layer, meri_bt_icon);
		bitmap_layer_set_alignment(meri_bt_icon_layer, GAlignCenter);
		layer_add_child(window_layer, bitmap_layer_get_layer(meri_bt_icon_layer));
	}	
}


static void handle_time(struct tm* current_time, TimeUnits units_changed) {
	static char s_time_text[] = "00:00:00";	
	static char s_day_text[] = "Mon, Jan 01";
  strftime(s_time_text, sizeof(s_time_text), "%l:%M", current_time);
  text_layer_set_text(s_time_layer, s_time_text);
	
  strftime(s_day_text, sizeof(s_day_text), "%a, %b %d", current_time);
	text_layer_set_text(s_date_layer, s_day_text);
}



static void main_window_load(Window *window) {
// 	Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
	int icon_width = 30;
	
	
	meri_image_layer = bitmap_layer_create(bounds);
  meri_image = gbitmap_create_with_resource(RESOURCE_ID_BG_IMAGE);	
  bitmap_layer_set_compositing_mode(meri_image_layer, GCompOpAssign);
  bitmap_layer_set_bitmap(meri_image_layer, meri_image);
  bitmap_layer_set_alignment(meri_image_layer, GAlignCenter);
  layer_add_child(window_layer, bitmap_layer_get_layer(meri_image_layer));
	
  
	//															 Grect( x, y, w, h )
  s_time_layer = text_layer_create(GRect(0, bounds.size.h-45, bounds.size.w, 45));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	s_date_layer = text_layer_create(GRect(0, 105, bounds.size.w, 45));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  s_battery_layer = text_layer_create(GRect(10, 0, bounds.size.w-10, 30));
  text_layer_set_text_color(s_battery_layer, GColorWhite);
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  text_layer_set_text(s_battery_layer, "100%");
	

	time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
	
	
	
  handle_time(current_time, MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_time);
	
  battery_state_service_subscribe(handle_battery);

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });


  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  handle_battery(battery_state_service_peek());
	handle_mute(window_layer, icon_width);
}

static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
	gbitmap_destroy(meri_image);
  bitmap_layer_destroy(meri_image_layer);
	gbitmap_destroy(meri_bt_icon);
  bitmap_layer_destroy(meri_bt_icon_layer);
	gbitmap_destroy(meri_mute_icon);
  bitmap_layer_destroy(meri_mute_icon_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
	window_layer = window_get_root_layer(s_main_window);
  window_stack_push(s_main_window, true);
	

	if (connection_service_peek_pebble_app_connection()) {
		persist_write_bool(connect, true);
// 		bitmap_layer_destroy(meri_bt_icon_layer);
	} else {
		persist_write_bool(connect, false);
		meri_bt_icon_layer = bitmap_layer_create(GRect(114, 0, 30,30));
		meri_bt_icon = gbitmap_create_with_resource(RESOURCE_ID_BT_ICON);	
		bitmap_layer_set_compositing_mode(meri_bt_icon_layer, GCompOpAssign);
		bitmap_layer_set_bitmap(meri_bt_icon_layer, meri_bt_icon);
		bitmap_layer_set_alignment(meri_bt_icon_layer, GAlignCenter);
		layer_add_child(window_layer, bitmap_layer_get_layer(meri_bt_icon_layer));
	}	
	
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}


