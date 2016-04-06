/*

MLB Live Face v2.0b1

----------------------

The MIT License (MIT)

Copyright © 2016 Chris Schlitt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

--------------------

*/

#include <pebble.h>
#include "sliding-text-layer.h"

Window *window;	
static TextLayer *s_time_layer;	
static SlidingTextLayer *s_home_team_layer;
static SlidingTextLayer *s_away_team_layer;
static SlidingTextLayer *s_game_time_layer;
static SlidingTextLayer *s_inning_layer;
static SlidingTextLayer* s_away_data_layer;
static SlidingTextLayer* s_home_data_layer;
static SlidingTextLayer* s_loading_layer;
static GFont s_font_mlb_40;
static GFont s_font_mlb_18;
static GFont s_font_capital_20;
static GFont s_font_phillies_22;
static GBitmap *s_team_logo;
static BitmapLayer *s_team_logo_layer;
static Layer *s_bso_layer;
static Layer *s_inning_state_layer;
static Layer *s_bases_layer;
static char inning[15];
static char home_score[25];
static char away_score[25];
static char s_buffer_prev[8];
uint32_t logo[31] = {RESOURCE_ID_PHILLIES, RESOURCE_ID_ANGELS, RESOURCE_ID_ASTROS, RESOURCE_ID_ATHLETICS, RESOURCE_ID_BLUEJAYS, RESOURCE_ID_BRAVES, RESOURCE_ID_BREWERS, RESOURCE_ID_CARDINALS, RESOURCE_ID_CUBS, RESOURCE_ID_DIAMONDBACKS, RESOURCE_ID_DODGERS, RESOURCE_ID_GIANTS, RESOURCE_ID_INDIANS, RESOURCE_ID_MARINERS, RESOURCE_ID_MARLINS, RESOURCE_ID_METS, RESOURCE_ID_NATIONALS, RESOURCE_ID_ORIOLES, RESOURCE_ID_PADRES, RESOURCE_ID_PHILLIES, RESOURCE_ID_PIRATES, RESOURCE_ID_RANGERS, RESOURCE_ID_RAYS, RESOURCE_ID_REDSOX, RESOURCE_ID_REDS, RESOURCE_ID_ROCKIES, RESOURCE_ID_ROYALS, RESOURCE_ID_TIGERS, RESOURCE_ID_TWINS, RESOURCE_ID_WHITESOX, RESOURCE_ID_YANKEES};
static int logo_uodate_i = 0;

// Color Resources
#define ASCII_0_VALU 48
#define ASCII_9_VALU 57
#define ASCII_A_VALU 65
#define ASCII_F_VALU 70
unsigned int HexStringToUInt(char const* hexstring)
{
    unsigned int result = 0;
    char const *c = hexstring;
    char thisC;
    while( (thisC = *c) != 0 )
    {
        unsigned int add;

        result <<= 4;
        if( thisC >= ASCII_0_VALU &&  thisC <= ASCII_9_VALU )
            add = thisC - ASCII_0_VALU;
        else if( thisC >= ASCII_A_VALU && thisC <= ASCII_F_VALU)
            add = thisC - ASCII_A_VALU + 10;
        else
        {
            printf("Unrecognised hex character \"%c\"\n", thisC);
            return 0;
        }
        result += add;
        ++c;
    }
    return result;  
}
// Debugging purposes
static void log_memory(int code){
  APP_LOG(APP_LOG_LEVEL_INFO, "%d : Memory Used = %d Free = %d", code, heap_bytes_used(), heap_bytes_free());
}

// Game Data data type
typedef struct GameDataSets {
  char home_team[4];
  char away_team[4];
  int num_games;
  int status;
  char home_pitcher[20];
  char away_pitcher[20];
  char game_time[6];
  char home_broadcast[20];
  char away_broadcast[20];
  int first;
  int second;
  int third;
  int home_score;
  int away_score;
  int inning;
  char inning_half[10];
  int balls;
  int strikes;
  int outs;
} GameData;

// Settings data type
typedef struct Settings {
  int favorite_team;
  int shake_enabeled;
  int shake_time;
  int refresh_time_off;
  int refresh_time_on;
  GColor primary_color;
  GColor secondary_color;
  GColor background_color;
} Settings;

static GameData currentGameData;
static GameData previousGameData;
static Settings userSettings;

static void initialize_settings(){
  userSettings.favorite_team = 19;
  userSettings.shake_enabeled = 1;
  userSettings.shake_time = 5;
  userSettings.refresh_time_off = 3600;
  userSettings.refresh_time_on = 180;
  userSettings.primary_color = GColorFromHEX(HexStringToUInt("FFFFFF"));
  userSettings.secondary_color = GColorFromHEX(HexStringToUInt("FFFFFF"));
  userSettings.background_color = GColorFromHEX(HexStringToUInt("000000"));
}

// Global Settings
int slide_number = 0;
int update_number = 0;
int showing_loading_screen = 1;
int showing_no_game = 0;
	
// Key values for AppMessage Dictionary
enum {
	TYPE = 0,
  NUM_GAMES = 1,
  STATUS = 2,
  HOME_TEAM = 3,
  AWAY_TEAM = 4,
  HOME_PITCHER = 5,
  AWAY_PITCHER = 6,
  GAME_TIME = 7,
  HOME_BROADCAST = 8,
  AWAY_BROADCAST = 9,
  FIRST = 10,
  SECOND = 11,
  THIRD = 12,
  HOME_SCORE = 13,
  AWAY_SCORE = 14,
  INNING = 15,
  INNING_HALF = 16,
  BALLS = 17,
  STRIKES = 18,
  OUTS = 19,
  PREF_FAVORITE_TEAM = 20,
  PREF_SHAKE_ENABELED = 21,
  PREF_SHAKE_TIME = 22,
  PREF_REFRESH_TIME_OFF = 23,
  PREF_REFRESH_TIME_ON = 24,
  PREF_PRIMARY_COLOR = 25,
  PREF_SECONDARY_COLOR = 26,
  PREF_BACKGROUND_COLOR = 27
};

// Key values for graphic instructions Dictionary
enum {
	BASES_CHANGE = 0,
  HOME_SCORE_CHANGE = 1,
  AWAY_SCORE_CHANGE = 2,
  INNING_CHANGE = 3,
  INNING_HALF_CHANGE = 4,
  BSO_CHANGE = 5
};

// On Wrist Shake Functions
static void show_broadcasts(){
  if(currentGameData.num_games > 0){
    sliding_text_layer_set_next_text(s_away_data_layer, currentGameData.away_broadcast);
    sliding_text_layer_animate_down(s_away_data_layer);
    sliding_text_layer_set_next_text(s_home_data_layer, currentGameData.home_broadcast);
    sliding_text_layer_animate_down(s_home_data_layer);
    slide_number = 1;
  }
}
static void show_pitchers(){
  if(currentGameData.num_games > 0){
    sliding_text_layer_set_next_text(s_away_data_layer, currentGameData.away_pitcher);
    sliding_text_layer_animate_up(s_away_data_layer);
    sliding_text_layer_set_next_text(s_home_data_layer, currentGameData.home_pitcher);
    sliding_text_layer_animate_up(s_home_data_layer);
    slide_number = 0;
  }
}

static void rotate_clear(SlidingTextLayer* layer_to_clear, int direction){
  sliding_text_layer_set_next_text(layer_to_clear, "");
  if(direction == 0){
    sliding_text_layer_animate_up(layer_to_clear);
  } else {
    sliding_text_layer_animate_down(layer_to_clear);
  }
}
static void hide_loading_screen(){
  sliding_text_layer_set_next_text(s_loading_layer, "");
  sliding_text_layer_animate_down(s_loading_layer);
  showing_loading_screen = 0;
}
static void change_teams(){
  if ((strcmp(currentGameData.home_team, previousGameData.home_team) != 0) || (strcmp(currentGameData.away_team, previousGameData.away_team) != 0)){
    sliding_text_layer_set_next_text(s_away_team_layer, currentGameData.away_team);
    sliding_text_layer_animate_down(s_away_team_layer);
    sliding_text_layer_set_next_text(s_home_team_layer, currentGameData.home_team);
    sliding_text_layer_animate_down(s_home_team_layer);
  }
}

static void request_update(){
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  app_message_outbox_send();
}


// Graphics Trigger Functions
static void update_bso(){
  layer_mark_dirty(s_bso_layer);
}
static void update_inning_state(){
  layer_mark_dirty(s_inning_state_layer);
}
static void update_bases(){
  layer_mark_dirty(s_bases_layer);
}

// Graphic Functions
static void startGame(){
  // Set Teams
  change_teams();
  // Animate Scores
  sliding_text_layer_set_text_alignment(s_away_data_layer, GTextAlignmentRight);
  sliding_text_layer_set_text_alignment(s_home_data_layer, GTextAlignmentRight);
  snprintf(away_score, sizeof(away_score), "%d                     ", currentGameData.away_score);
  sliding_text_layer_set_next_text(s_away_data_layer, away_score);
  sliding_text_layer_animate_down(s_away_data_layer);
  snprintf(home_score, sizeof(home_score), "%d                     ", currentGameData.home_score);
  sliding_text_layer_set_next_text(s_home_data_layer, home_score);
  sliding_text_layer_animate_down(s_home_data_layer);
  // Animate Inning
  sliding_text_layer_set_font(s_inning_layer, s_font_phillies_22);
  snprintf(inning, sizeof(inning), "      %d", currentGameData.inning);
  sliding_text_layer_set_next_text(s_inning_layer, inning);
  sliding_text_layer_animate_down(s_inning_layer);
  // Animate Game Time
  rotate_clear(s_game_time_layer, 1);
  // Redraw Inning State
  update_inning_state();
  // Redraw Balls, Strikes, Outs
  update_bso();
  // Redraw bases
  update_bases();
}
static void endGame(){
  // Set Teams
  change_teams();
  // Set Score Change
  sliding_text_layer_set_text_alignment(s_away_data_layer, GTextAlignmentRight);
  sliding_text_layer_set_text_alignment(s_home_data_layer, GTextAlignmentRight);
  if (currentGameData.away_score != previousGameData.away_score){
    snprintf(away_score, sizeof(away_score), "%d                     ", currentGameData.away_score);
    sliding_text_layer_set_next_text(s_away_data_layer, away_score);
    sliding_text_layer_animate_down(s_away_data_layer);
  }
  if (currentGameData.home_score != previousGameData.home_score){
    snprintf(home_score, sizeof(home_score), "%d                     ", currentGameData.home_score);
    sliding_text_layer_set_next_text(s_home_data_layer, home_score);
    sliding_text_layer_animate_down(s_home_data_layer);
  }
  // Animate Inning to "Final"
  sliding_text_layer_set_font(s_inning_layer, s_font_capital_20);
  sliding_text_layer_set_next_text(s_inning_layer, "Final");
  sliding_text_layer_animate_down(s_inning_layer);
  // Redraw Inning State
  update_inning_state();
  // Redraw Balls, Strikes, Outs
  update_bso();
  // Redraw bases
  update_bases();
}
static void newGame(){
  // Change teams
  change_teams();
  // Animate inning to ""
  rotate_clear(s_inning_layer, 1);
  // Animate Data to Pitchers
  sliding_text_layer_set_text_alignment(s_away_data_layer, GTextAlignmentLeft);
  sliding_text_layer_set_text_alignment(s_home_data_layer, GTextAlignmentLeft);
  sliding_text_layer_set_next_text(s_away_data_layer, currentGameData.away_pitcher);
  sliding_text_layer_animate_down(s_away_data_layer);
  sliding_text_layer_set_next_text(s_home_data_layer, currentGameData.home_pitcher);
  sliding_text_layer_animate_down(s_home_data_layer);
  // Show Game Time
  sliding_text_layer_set_next_text(s_game_time_layer, currentGameData.game_time);
  sliding_text_layer_animate_down(s_game_time_layer);
}
static void updateGame(int *instructions){
  // Update game stats based on instructions
  if (instructions[0] == 1){
    update_bases();
  }
  if (instructions[1] == 1){
    snprintf(home_score, sizeof(home_score), "%d                     ", currentGameData.home_score);
    sliding_text_layer_set_next_text(s_home_data_layer, home_score);
    sliding_text_layer_animate_down(s_home_data_layer);
  }
  if (instructions[2] == 1){
    snprintf(away_score, sizeof(away_score), "%d                     ", currentGameData.away_score);
    sliding_text_layer_set_next_text(s_away_data_layer, away_score);
    sliding_text_layer_animate_down(s_away_data_layer);
  }
  if (instructions[3] == 1){
    snprintf(inning, sizeof(inning), "      %d", currentGameData.inning);
    sliding_text_layer_set_next_text(s_inning_layer, inning);
    sliding_text_layer_animate_down(s_inning_layer);
  }
  if (instructions[4] == 1){
    update_bso();
  }
  if (instructions[5] == 1){
    update_inning_state();
  }
}
static void showNoGame(){
  if(showing_no_game == 0){
    // Hide All Text layers
    rotate_clear(s_away_data_layer, 1);
    rotate_clear(s_home_data_layer, 1);
    rotate_clear(s_inning_layer, 1);
    rotate_clear(s_away_team_layer, 1);
    rotate_clear(s_home_team_layer, 1);
    rotate_clear(s_game_time_layer, 1);
    // Show No Game
    sliding_text_layer_set_next_text(s_loading_layer, "No Game Today");
    sliding_text_layer_animate_down(s_loading_layer);
    showing_loading_screen = 1;
    showing_no_game = 1;
  }
}
static void change_colors(){
  // Set Primary Color
  sliding_text_layer_set_text_color(s_home_team_layer, userSettings.primary_color);
  sliding_text_layer_set_text_color(s_away_team_layer, userSettings.primary_color);
  sliding_text_layer_set_text_color(s_home_data_layer, userSettings.primary_color);
  sliding_text_layer_set_text_color(s_away_data_layer, userSettings.primary_color);
  sliding_text_layer_set_text_color(s_inning_layer, userSettings.primary_color);
  sliding_text_layer_set_text_color(s_loading_layer, userSettings.primary_color);
  sliding_text_layer_set_text_color(s_game_time_layer, userSettings.primary_color);
  update_bso();
  update_inning_state();
  // Set Secondary Color
  text_layer_set_text_color(s_time_layer, userSettings.secondary_color);
  update_bases();
  // Set Background Color
  window_set_background_color(window, userSettings.background_color);
  // Set Favorite Team
  if(logo_uodate_i > 0){
    gbitmap_destroy(s_team_logo);
  }
  logo_uodate_i = 1;
  s_team_logo = gbitmap_create_with_resource(logo[userSettings.favorite_team]);
  bitmap_layer_set_bitmap(s_team_logo_layer, s_team_logo);
  layer_mark_dirty(bitmap_layer_get_layer(s_team_logo_layer));
}

// Function to determine which graphics need to be updated
static void route_graphic_updates(){
  if ((currentGameData.status != previousGameData.status) || (strcmp(currentGameData.home_team, previousGameData.home_team) != 0) || (strcmp(currentGameData.away_team, previousGameData.away_team) != 0)){
    if (currentGameData.status == 2){
      // Game Started
      startGame();
    } else if (previousGameData.status == 2){
      // Game Ended
      endGame();
    } else if (previousGameData.status == 3){
      // Show a New Game
      newGame();
    } else if (currentGameData.status == 3){
      // End the Game
      endGame();
    } else {
      // New Game
      newGame();
    }
  } else if (currentGameData.status == 0 && previousGameData.status != 0){
  // } else if (currentGameData.status == 0){
    // New Game Fallback
    newGame();
  } else if (currentGameData.status == 2){
    // Declare the dictionary's iterator
    int instructions[6] = { 0, 0, 0, 0, 0, 0 };
    if ((currentGameData.first != previousGameData.first) || (currentGameData.second != previousGameData.second) || (currentGameData.third != previousGameData.third)){
      // New Bases
      instructions[0] = 1;
    }
    if (currentGameData.home_score != previousGameData.home_score){
      // New Home Score
      instructions[1] = 1;
    }
    if (currentGameData.away_score != previousGameData.away_score){
      // New Away Score
      instructions[2] = 1;
    }
    if (currentGameData.inning != previousGameData.inning){
      // New Inning
      instructions[3] = 1;
    }
    if ((currentGameData.balls != previousGameData.balls) || (currentGameData.strikes != previousGameData.strikes) || (currentGameData.outs != previousGameData.outs)){
      // New Outs
      instructions[4] = 1;
    }
    if (strcmp(currentGameData.inning_half, previousGameData.inning_half) != 0) {
      // New Inning Half
      instructions[5] = 1;
    }
    // Update graphics based on instructions
    updateGame(instructions);
  }
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
  // Does this message contain a change in type
  Tuple *type_tuple = dict_find(received, TYPE);
  int type = 1;
  if(type_tuple) {
    // This value was stored as JS Number, which is stored here as int32_t
    type = (int)type_tuple->value->int32;
  }
  if (type == 0){
    Tuple *t = dict_read_first(received);
    while(t != NULL) {
      switch(t->key) {
        case PREF_FAVORITE_TEAM:
          userSettings.favorite_team = (int)t->value->int32;
          break;
        case PREF_SHAKE_ENABELED:
          userSettings.shake_enabeled = (int)t->value->int32;
          break;
        case PREF_SHAKE_TIME:
          userSettings.shake_time = (int)t->value->int32;
          break;
        case PREF_REFRESH_TIME_OFF:
          userSettings.refresh_time_off = (int)t->value->int32;
          break;
        case PREF_REFRESH_TIME_ON:
          userSettings.refresh_time_on = (int)t->value->int32;
          break;
        case PREF_PRIMARY_COLOR:
          userSettings.primary_color = GColorFromHEX(HexStringToUInt(t->value->cstring));
          break;
        case PREF_SECONDARY_COLOR:
          userSettings.secondary_color = GColorFromHEX(HexStringToUInt(t->value->cstring));
          break;
        case PREF_BACKGROUND_COLOR:
          userSettings.background_color = GColorFromHEX(HexStringToUInt(t->value->cstring));
          break;
        default:
          break;
      }
      t = dict_read_next(received);
    }
    // Update the colors
    change_colors();
    
  } else {
    
    Tuple *type_tuple = dict_find(received, NUM_GAMES);
    int num_games = 0;
    if(type_tuple) {
      // This value was stored as JS Number, which is stored here as int32_t
      num_games = (int)type_tuple->value->int32;
    }
    if (num_games == 0){
      // Show No Game Today
      showNoGame();
    } else {
      // Hide the Loading Screen
      if(showing_loading_screen == 1){
        hide_loading_screen();
      }
      // Hide the No Game Message
      if(showing_no_game == 1){
        showing_no_game = 0;
      }
      
      // Process the data set
      // Copy the old data set
      previousGameData = currentGameData;
      
      Tuple *t = dict_read_first(received);
      while(t != NULL) {
        switch(t->key) {
          case HOME_TEAM:
            strcpy(currentGameData.home_team,t->value->cstring);
            break;
          case AWAY_TEAM:
            strcpy(currentGameData.away_team,t->value->cstring);
            break;
          case NUM_GAMES:
            currentGameData.num_games = (int)t->value->int32;
            break;
          case STATUS:
            currentGameData.status = (int)t->value->int32;
            break;
          case HOME_PITCHER:
            strcpy(currentGameData.home_pitcher,t->value->cstring);
            break;
          case AWAY_PITCHER:
            strcpy(currentGameData.away_pitcher,t->value->cstring);
            break;
          case GAME_TIME:
            strcpy(currentGameData.game_time,t->value->cstring);
            break;
          case HOME_BROADCAST:
            strcpy(currentGameData.home_broadcast,t->value->cstring);
            break;
          case AWAY_BROADCAST:
            strcpy(currentGameData.away_broadcast,t->value->cstring);
            break;
          case FIRST:
            currentGameData.first = (int)t->value->int32;
            break;
          case SECOND:
            currentGameData.second = (int)t->value->int32;
            break;
          case THIRD:
            currentGameData.third = (int)t->value->int32;
            break;
          case HOME_SCORE:
            currentGameData.home_score = (int)t->value->int32;
            break;
          case AWAY_SCORE:
            currentGameData.away_score = (int)t->value->int32;
            break;
          case INNING:
            currentGameData.inning = (int)t->value->int32;
            break;
          case INNING_HALF:
            strcpy(currentGameData.inning_half,t->value->cstring);
            break;
          case BALLS:
            currentGameData.balls = (int)t->value->int32;
            break;
          case STRIKES:
            currentGameData.strikes = (int)t->value->int32;
            break;
          case OUTS:
            currentGameData.outs = (int)t->value->int32;
            break;
          default:
            break;
        }
        t = dict_read_next(received);
      }
      
      // After processing data, route graphic updates
      route_graphic_updates();
    }
    
    
  }
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
  // Reload data
  request_update();
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  // Reload data
  request_update();
}

// Graphics Drawing Functions
static void bso_update_proc(Layer *layer, GContext *ctx) {
  if (currentGameData.status == 2){
    GRect bounds = layer_get_bounds(layer);
    // Custom drawing happens here!
    graphics_context_set_fill_color(ctx, userSettings.primary_color);
    graphics_context_set_stroke_width(ctx, 4);
    #ifdef PBL_ROUND
      if(currentGameData.outs == 2){
        graphics_fill_circle(ctx, GPoint(((bounds.size.w) / 2) + 11, (bounds.size.h - 15)), 6);
        graphics_fill_circle(ctx, GPoint(((bounds.size.w) / 2) - 11, (bounds.size.h - 15)), 6);
      } else if(currentGameData.outs > 0) {
        graphics_fill_circle(ctx, GPoint((bounds.size.w) / 2, (bounds.size.h - 14)), 6);
        if ((strcmp(currentGameData.inning_half, "Middle") == 0) || (strcmp(currentGameData.inning_half, "End") == 0)){
          graphics_fill_circle(ctx, GPoint(((bounds.size.w) / 2) - 21, (bounds.size.h - 15)), 6);
          graphics_fill_circle(ctx, GPoint(((bounds.size.w) / 2) + 21, (bounds.size.h - 15)), 6);
        }
      }
    #else
      if(currentGameData.outs == 2){
        graphics_fill_circle(ctx, GPoint(((bounds.size.w) / 20) * 19, ((bounds.size.h / 4) * 3 + 23)), 6);
        graphics_fill_circle(ctx, GPoint(((bounds.size.w) / 20) * 19, ((bounds.size.h / 4) * 3 + 1)), 6);
      } else if(currentGameData.outs > 0) {
        graphics_fill_circle(ctx, GPoint(((bounds.size.w) / 20) * 19, (((bounds.size.h / 4) * 3 + 12))), 6);
        if ((strcmp(currentGameData.inning_half, "Middle") == 0) || (strcmp(currentGameData.inning_half, "End") == 0)){
          graphics_fill_circle(ctx, GPoint(((bounds.size.w) / 20) * 19, ((bounds.size.h / 4) * 3 - 7)), 6);
          graphics_fill_circle(ctx, GPoint(((bounds.size.w) / 20) * 19, ((bounds.size.h / 4) * 3 + 31)), 6);
        }
      }
    #endif
  }
}
static void inning_state_update_proc(Layer *layer, GContext *ctx) {
  if (currentGameData.status == 2){
    GRect bounds = layer_get_bounds(layer);
    // Custom drawing happens here!
    if ((strcmp(currentGameData.inning_half, "Top") == 0) || (strcmp(currentGameData.inning_half, "Middle") == 0)) {
      #ifdef PBL_ROUND
        GPoint inning_up_arrow[4] = {
          { .x = ((bounds.size.w / 3) * 2) - 5, .y = ((bounds.size.h / 10) * 6) + 21 },
          { .x = ((bounds.size.w / 3) * 2) - 12, .y = ((bounds.size.h / 10) * 6) + 33 },
          { .x = ((bounds.size.w / 3) * 2) + 2, .y = ((bounds.size.h / 10) * 6) + 33 },
          { .x = ((bounds.size.w / 3) * 2) - 5, .y = ((bounds.size.h / 10) * 6) + 21 }
        };
      #else
        GPoint inning_up_arrow[4] = {
          { .x = ((bounds.size.w / 3) * 2) - 5, .y = ((bounds.size.h / 10) * 7) + 22 },
          { .x = ((bounds.size.w / 3) * 2) - 12, .y = ((bounds.size.h / 10) * 7) + 34 },
          { .x = ((bounds.size.w / 3) * 2) + 2, .y = ((bounds.size.h / 10) * 7) + 34 },
          { .x = ((bounds.size.w / 3) * 2) - 5, .y = ((bounds.size.h / 10) * 7) + 22 }
        };
      #endif
      GPathInfo inning_up_arrowinfo = { .num_points = 4, .points = inning_up_arrow };
      GPath *inning_up_arrow_path = gpath_create(&inning_up_arrowinfo);
      graphics_context_set_fill_color(ctx, userSettings.primary_color);
      gpath_draw_filled(ctx, inning_up_arrow_path);
      gpath_destroy(inning_up_arrow_path);
    } else if ((strcmp(currentGameData.inning_half, "Bottom") == 0) || (strcmp(currentGameData.inning_half, "End") == 0)) {
      #ifdef PBL_ROUND
        GPoint inning_down_arrow[4] = {
          { .x = ((bounds.size.w / 3) * 2) - 5, .y = ((bounds.size.h / 10) * 6) + 33 },
          { .x = ((bounds.size.w / 3) * 2) - 12, .y = ((bounds.size.h / 10) * 6) + 21 },
          { .x = ((bounds.size.w / 3) * 2) + 2, .y = ((bounds.size.h / 10) * 6) + 21 },
          { .x = ((bounds.size.w / 3) * 2) - 5, .y = ((bounds.size.h / 10) * 6) + 33 }
        };
      #else
        GPoint inning_down_arrow[4] = {
          { .x = ((bounds.size.w / 3) * 2) - 5, .y = ((bounds.size.h / 10) * 7) + 34 },
          { .x = ((bounds.size.w / 3) * 2) - 12, .y = ((bounds.size.h / 10) * 7) + 22 },
          { .x = ((bounds.size.w / 3) * 2) + 2, .y = ((bounds.size.h / 10) * 7) + 22 },
          { .x = ((bounds.size.w / 3) * 2) - 5, .y = ((bounds.size.h / 10) * 7) + 34 }
        };
      #endif
      GPathInfo inning_down_arrowinfo = { .num_points = 4, .points = inning_down_arrow };
      GPath *inning_down_arrow_path = gpath_create(&inning_down_arrowinfo);
      graphics_context_set_fill_color(ctx, userSettings.primary_color);
      gpath_draw_filled(ctx, inning_down_arrow_path);
      gpath_destroy(inning_down_arrow_path);
    }
  }
}
static void bases_update_proc(Layer *layer, GContext *ctx) {
  if (currentGameData.status == 2){
    GRect bounds = layer_get_bounds(layer);
    // Custom drawing happens here!
    graphics_context_set_fill_color(ctx, userSettings.secondary_color);
    graphics_context_set_stroke_color(ctx, userSettings.secondary_color);
    graphics_context_set_stroke_width(ctx, 3);
    GPoint first_points[4] = {
      { .x = (bounds.size.w - 20), .y = bounds.size.h / 2 },
      { .x = bounds.size.w, .y = (bounds.size.h / 2) + 20 },
      { .x = bounds.size.w, .y = (bounds.size.h / 2) - 20 },
      { .x = (bounds.size.w - 20), .y = bounds.size.h / 2 }
    };
    GPoint second_points[4] = {
      { .x = (bounds.size.w / 2), .y = 20 },
      { .x = ((bounds.size.w / 2) - 20), .y = 0 },
      { .x = ((bounds.size.w / 2) + 20), .y = 0 },
      { .x = (bounds.size.w / 2), .y = 20 }
    };
    GPoint third_points[4] = {
      { .x = 20, .y = bounds.size.h / 2 },
      { .x = 0, .y = (bounds.size.h / 2) + 20 },
      { .x = 0, .y = (bounds.size.h / 2) - 20 },
      { .x = 20, .y = bounds.size.h / 2 }
    };
    GPathInfo first_pathinfo = { .num_points = 4, .points = first_points };
    GPath *first_path = gpath_create(&first_pathinfo);
    GPathInfo second_pathinfo = { .num_points = 4, .points = second_points };
    GPath *second_path = gpath_create(&second_pathinfo);
    GPathInfo third_pathinfo = { .num_points = 4, .points = third_points };
    GPath *third_path = gpath_create(&third_pathinfo);
    
    graphics_context_set_fill_color(ctx, userSettings.secondary_color);
    if (currentGameData.first == 1) {
      gpath_draw_filled(ctx, first_path);
    } else if (currentGameData.status == 2) {
      gpath_draw_outline(ctx, first_path);
    }
    gpath_destroy(first_path);
    if (currentGameData.second == 1) {
      gpath_draw_filled(ctx, second_path);
    } else if (currentGameData.status == 2) {
      gpath_draw_outline(ctx, second_path);
    }
    gpath_destroy(second_path);
    if (currentGameData.third == 1) {
      gpath_draw_filled(ctx, third_path);
    } else if (currentGameData.status == 2) {
      gpath_draw_outline(ctx, third_path);
    }
    gpath_destroy(third_path);
  }
}

// Window Handlers
static void window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, userSettings.background_color);
  
  // Load team logo
  #ifdef PBL_ROUND
    s_team_logo_layer = bitmap_layer_create(GRect(0, 0, bounds.size.w, 113));
  #else
    #ifdef PBL_COLOR
      s_team_logo_layer = bitmap_layer_create(GRect(-22, -6, bounds.size.w + 22, 119));
    #else
      s_team_logo_layer = bitmap_layer_create(GRect(0, 0, 144, 108));
    #endif
  #endif
  bitmap_layer_set_compositing_mode(s_team_logo_layer, GCompOpSet);
  
  // Load custom fonts
  s_font_mlb_40 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MLB_40));
  s_font_mlb_18 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MLB_18));
  s_font_capital_20 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CAPITAL_20));
  s_font_phillies_22 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PHILLIES_22));
  
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(GRect(0, ((bounds.size.h / 10) * 4) - 6, bounds.size.w, 54));
  #ifdef PBL_ROUND
    s_away_team_layer = sliding_text_layer_create(GRect((bounds.size.w / 5), ((bounds.size.h / 10) * 6) + 5, 50, 25));
    s_home_team_layer = sliding_text_layer_create(GRect((bounds.size.w / 5), ((bounds.size.h / 10) * 7) + 9, 50, 25));
    s_game_time_layer = sliding_text_layer_create(GRect((bounds.size.w / 5) * 2, ((bounds.size.h / 10) * 8) + 11, 50, 25));
    s_away_data_layer = sliding_text_layer_create(GRect(((bounds.size.w / 5) * 2) + 5, ((bounds.size.h / 10) * 6) + 5, ((bounds.size.w / 5) * 3) - 5, 25));
    s_home_data_layer = sliding_text_layer_create(GRect(((bounds.size.w / 5) * 2) + 5, ((bounds.size.h / 10) * 7) + 8, ((bounds.size.w / 5) * 3) - 5, 25));
    s_inning_layer = sliding_text_layer_create(GRect(((bounds.size.w / 5) * 3), ((bounds.size.h / 10) * 6) + 15, ((bounds.size.w / 5) * 3) - 5, 45));
    s_loading_layer = sliding_text_layer_create(GRect(0, ((bounds.size.h / 10) * 6) + 15, bounds.size.w, 45));
  #else
    s_away_team_layer = sliding_text_layer_create(GRect((bounds.size.w / 15), ((bounds.size.h / 10) * 7) + 2, 50, 25));
    s_home_team_layer = sliding_text_layer_create(GRect((bounds.size.w / 15), ((bounds.size.h / 10) * 8) + 10, 50, 25));
    s_game_time_layer = sliding_text_layer_create(GRect(((bounds.size.w / 5) * 4) - 4, ((bounds.size.h / 10) * 7) + 14, 50, 25));
    s_away_data_layer = sliding_text_layer_create(GRect(((bounds.size.w / 15) * 2) + 30, ((bounds.size.h / 10) * 7) + 2, ((bounds.size.w / 5) * 4) - 0, 30));
    s_home_data_layer = sliding_text_layer_create(GRect(((bounds.size.w / 15) * 2) + 30, ((bounds.size.h / 10) * 8) + 10, ((bounds.size.w / 5) * 4) - 0, 30));
    s_inning_layer = sliding_text_layer_create(GRect(((bounds.size.w / 5) * 3), ((bounds.size.h / 10) * 7) + 14, ((bounds.size.w / 5) * 3) - 5, 45));
    s_loading_layer = sliding_text_layer_create(GRect(0, ((bounds.size.h / 10) * 7) + 14, bounds.size.w, 45));
  #endif
  
  // Create the canvas layers
  s_bso_layer = layer_create(bounds);
  s_inning_state_layer = layer_create(bounds);
  s_bases_layer = layer_create(bounds);
  layer_set_update_proc(s_bso_layer, bso_update_proc);
  layer_set_update_proc(s_inning_state_layer, inning_state_update_proc);
  layer_set_update_proc(s_bases_layer, bases_update_proc);
  
  // Improve the layout to be more like a watchface
  text_layer_set_text_color(s_time_layer, userSettings.secondary_color);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, s_font_mlb_40);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  sliding_text_layer_set_text_color(s_away_team_layer, userSettings.primary_color);
  sliding_text_layer_set_text(s_away_team_layer, "");
  sliding_text_layer_set_font(s_away_team_layer, s_font_capital_20);
  sliding_text_layer_set_text_alignment(s_away_team_layer, GTextAlignmentLeft);
  
  sliding_text_layer_set_text_color(s_home_team_layer, userSettings.primary_color);
  sliding_text_layer_set_text(s_home_team_layer, "");
  sliding_text_layer_set_font(s_home_team_layer, s_font_capital_20);
  sliding_text_layer_set_text_alignment(s_home_team_layer, GTextAlignmentLeft);
  
  sliding_text_layer_set_text_color(s_game_time_layer, userSettings.primary_color);
  sliding_text_layer_set_text(s_game_time_layer, "");
  sliding_text_layer_set_font(s_game_time_layer, s_font_mlb_18);
  sliding_text_layer_set_text_alignment(s_game_time_layer, GTextAlignmentLeft);
  
  sliding_text_layer_set_text_color(s_away_data_layer, userSettings.primary_color);
  sliding_text_layer_set_text(s_away_data_layer, "");
  sliding_text_layer_set_font(s_away_data_layer, s_font_phillies_22);
  sliding_text_layer_set_text_alignment(s_away_data_layer, GTextAlignmentLeft);
  
  sliding_text_layer_set_text_color(s_home_data_layer, userSettings.primary_color);
  sliding_text_layer_set_text(s_home_data_layer, "");
  sliding_text_layer_set_font(s_home_data_layer, s_font_phillies_22);
  sliding_text_layer_set_text_alignment(s_home_data_layer, GTextAlignmentLeft);
  
  sliding_text_layer_set_text_color(s_inning_layer, userSettings.primary_color);
  sliding_text_layer_set_text(s_inning_layer, "");
  sliding_text_layer_set_font(s_inning_layer, s_font_phillies_22);
  sliding_text_layer_set_text_alignment(s_inning_layer, GTextAlignmentLeft);
  
  sliding_text_layer_set_text_color(s_loading_layer, userSettings.primary_color);
  sliding_text_layer_set_next_text(s_loading_layer, "LOADING");
  sliding_text_layer_animate_down(s_loading_layer);
  sliding_text_layer_set_font(s_loading_layer, s_font_capital_20);
  sliding_text_layer_set_text_alignment(s_loading_layer, GTextAlignmentCenter);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, bitmap_layer_get_layer(s_team_logo_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  sliding_text_layer_add_to_window(s_home_team_layer, window);
  sliding_text_layer_add_to_window(s_away_team_layer, window);
  sliding_text_layer_add_to_window(s_game_time_layer, window);
  sliding_text_layer_add_to_window(s_away_data_layer, window);
  sliding_text_layer_add_to_window(s_home_data_layer, window);
  sliding_text_layer_add_to_window(s_inning_layer, window);
  sliding_text_layer_add_to_window(s_loading_layer, window);
  layer_add_child(window_layer, s_bso_layer);
  layer_add_child(window_layer, s_inning_state_layer);
  layer_add_child(window_layer, s_bases_layer);
}

static void window_unload(Window *window) {
  fonts_unload_custom_font(s_font_mlb_40);
  fonts_unload_custom_font(s_font_mlb_18);
  fonts_unload_custom_font(s_font_capital_20);
  fonts_unload_custom_font(s_font_phillies_22);
  text_layer_destroy(s_time_layer);
  sliding_text_layer_destroy(s_home_team_layer);
  sliding_text_layer_destroy(s_away_team_layer);
  sliding_text_layer_destroy(s_game_time_layer);
  sliding_text_layer_destroy(s_away_data_layer);
  sliding_text_layer_destroy(s_home_data_layer);
  sliding_text_layer_destroy(s_inning_layer);
  sliding_text_layer_destroy(s_loading_layer);
  gbitmap_destroy(s_team_logo);
  bitmap_layer_destroy(s_team_logo_layer);
  layer_destroy(s_bso_layer);
  layer_destroy(s_inning_state_layer);
  layer_destroy(s_bases_layer);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(s_buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    //Use 12 hour format
    strftime(s_buffer, sizeof("00:00"), "%I:%M", tick_time);
    int n;
    if( ( n = strspn(s_buffer, "0" ) ) != 0 && s_buffer[n] != '\0' ) {
      snprintf(s_buffer, sizeof(s_buffer), "%s", &s_buffer[n]);
    }
    
  }
  // Display this time on the TextLayer if the time changed
  if (strcmp(s_buffer, s_buffer_prev) != 0){
    text_layer_set_text(s_time_layer, s_buffer);
    snprintf(s_buffer_prev, sizeof(s_buffer_prev), "%s", s_buffer);
  }
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  if (slide_number > 0){
    slide_number++;
    if (slide_number == userSettings.shake_time){
      show_pitchers();
      slide_number = 0;
    }
  }
  
  update_number++;
  if (currentGameData.status > 0 && currentGameData.status < 3){
    if (update_number >= userSettings.refresh_time_on){
      // Update
      request_update();
      update_number = 0;
    }
  } else {
    if (update_number >= userSettings.refresh_time_off){
      // Update
      request_update();
      update_number = 0;
    }
  }
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  // A tap event occured
  if(userSettings.shake_enabeled == 1){
    if(currentGameData.status < 2){
      if (slide_number == 0){
        show_broadcasts();
      }
    }
  }
  
}

void init(void) {
  // Populate the initial settings for loading
  initialize_settings();
  currentGameData.status = 99;
	window = window_create();
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
	window_stack_push(window, true);
	update_time();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  accel_tap_service_subscribe(accel_tap_handler);
  
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
		
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
}

void deinit(void) {
	app_message_deregister_callbacks();
  accel_tap_service_unsubscribe();
	window_destroy(window);
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}