#include "pebble.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Credit link: https://www.sitepoint.com/how-to-add-images-into-your-pebble-watchfaces/

// 	APP_LOG(APP_LOG_LEVEL_DEBUG, "");

static BitmapLayer *image_layer;
static GBitmap *image;

static BitmapLayer *bt_icon_layer;
static GBitmap *bt_icon;

static BitmapLayer *mute_icon_layer;
static GBitmap *mute_icon;

static Window *s_main_window;
Layer *window_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;
static Layer *s_canvas_layer;
uint32_t connect = 0;

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";

  snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  text_layer_set_text(s_battery_layer, battery_text);
}


static void handle_bluetooth(bool connected) {	
	if (connected) {
		persist_write_bool(connect, true);		
	} else {
		if (persist_read_bool(connect)) {
			vibes_double_pulse();			
			persist_write_bool(connect, false);	
		}
	}
	layer_set_hidden(bitmap_layer_get_layer(bt_icon_layer),connected);
	// --------------------- ^ PROBLEM LINE ^ --------------------------------------------------
}


static void handle_mute() {		
	layer_set_hidden(bitmap_layer_get_layer(mute_icon_layer),!quiet_time_is_active());
	// --------------------- ^ PROBLEM LINE ^ --------------------------------------------------
}

static void handle_time(struct tm* current_time, TimeUnits units_changed) {
	static char s_time_text[] = "00:00:00";	
	static char s_day_text[] = "Mon, Jan 01";
  strftime(s_time_text, sizeof(s_time_text), "%l:%M", current_time);
  text_layer_set_text(s_time_layer, s_time_text);
	
  strftime(s_day_text, sizeof(s_day_text), "%a, %b %d", current_time);
	text_layer_set_text(s_date_layer, s_day_text);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
	//---------TIME BACKGROUND---------//
	graphics_context_set_fill_color(ctx, GColorBlack);
	GRect rect1 = GRect(28, 110, 95, 60);
  graphics_draw_rect(ctx, rect1);
	graphics_fill_rect(ctx, rect1, 0, GCornersAll);	
	
	//---------ICONS BACKGROUND---------//
	graphics_context_set_fill_color(ctx, GColorWhite);
	GRect rect2 = GRect(0, 0, 144, 28);
  graphics_draw_rect(ctx, rect2);
	graphics_fill_rect(ctx, rect2, 0, GCornersAll);
}



static void main_window_load(Window *window) {	
	//Grect( x, y, w, h )
  GRect bounds = layer_get_frame(window_layer);
	
	
	//-------BACKGROUND IMAGE-------//
	image_layer = bitmap_layer_create(GRect(0, 28, bounds.size.w, bounds.size.h));
  image = gbitmap_create_with_resource(RESOURCE_ID_BG_IMAGE);	
  bitmap_layer_set_compositing_mode(image_layer, GCompOpAssign);
  bitmap_layer_set_bitmap(image_layer, image);
  bitmap_layer_set_alignment(image_layer, GAlignCenter);
  layer_add_child(window_layer, bitmap_layer_get_layer(image_layer));
	
	//---------DRAWING---------//
	s_canvas_layer = layer_create(bounds);
	layer_set_update_proc(s_canvas_layer, canvas_update_proc);
	layer_add_child(window_get_root_layer(window), s_canvas_layer);
	
  
	//---------TIME---------//
  s_time_layer = text_layer_create(GRect(0, bounds.size.h-45, bounds.size.w, 45));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	//---------DATE---------//
	s_date_layer = text_layer_create(GRect(0, 105, bounds.size.w, 45));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
	
	//---------BATTERY---------//
  s_battery_layer = text_layer_create(GRect(10, 0, bounds.size.w-10, 30));
  text_layer_set_text_color(s_battery_layer, GColorBlack);
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
	handle_mute();
}

static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
	gbitmap_destroy(image);
  bitmap_layer_destroy(image_layer);
	
	bitmap_layer_destroy(bt_icon_layer);	
	gbitmap_destroy(bt_icon);
	gbitmap_destroy(mute_icon);	
	bitmap_layer_destroy(mute_icon_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorWhite);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
	window_layer = window_get_root_layer(s_main_window);
  window_stack_push(s_main_window, true);
	
	//---------MUTE ICON---------//
	mute_icon_layer = bitmap_layer_create(GRect(0, 0, 30, 30));		
	bitmap_layer_set_background_color(mute_icon_layer, GColorWhite);	
// --------------------- ^ NOT SETTING BACKGROUND ^ --------------------------------------------------	
	mute_icon = gbitmap_create_with_resource(RESOURCE_ID_MUTE_ICON);	
	bitmap_layer_set_compositing_mode(mute_icon_layer, GCompOpAssign);
	bitmap_layer_set_bitmap(mute_icon_layer, mute_icon);
	bitmap_layer_set_alignment(mute_icon_layer, GAlignCenter);		
	layer_add_child(window_layer, bitmap_layer_get_layer(mute_icon_layer));

	//---------BLUETOOTH ICON---------//
	bt_icon_layer = bitmap_layer_create(GRect(114, 0, 30, 30));		
	bitmap_layer_set_background_color(bt_icon_layer, GColorWhite);
// --------------------- ^ NOT SETTING BACKGROUND ^ --------------------------------------------------	
	bt_icon = gbitmap_create_with_resource(RESOURCE_ID_BT_ICON);	
	bitmap_layer_set_compositing_mode(bt_icon_layer, GCompOpAssign);
	bitmap_layer_set_bitmap(bt_icon_layer, bt_icon);
	bitmap_layer_set_alignment(bt_icon_layer, GAlignCenter);	
	layer_add_child(window_layer, bitmap_layer_get_layer(bt_icon_layer));
	
	persist_write_bool(connect, connection_service_peek_pebble_app_connection());			
	layer_set_hidden(bitmap_layer_get_layer(bt_icon_layer),connection_service_peek_pebble_app_connection());		
	layer_set_hidden(bitmap_layer_get_layer(mute_icon_layer),!quiet_time_is_active()); 
	// --------------------- ^ layer_set_hidden works ^ --------------------------------------------------
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}


