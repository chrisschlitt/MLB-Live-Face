/*

SlidingTextLayer v1.1

----------------------

The MIT License (MIT)

Copyright © 2015 Matthew Tole

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

lib/sliding-text-layer.h

*/


#include <pebble.h>
#include "sliding-text-layer.h"


#define ANIMATION_DIRECTION_UP 0
#define ANIMATION_DIRECTION_DOWN 1


typedef struct SlidingTextLayerData {
  char* text_next;
  char* text_current;
  GColor color_text;
  GFont font;
  bool is_animating;
  GTextOverflowMode overflow;
  GTextAlignment align;
  Animation* animation;
  AnimationImplementation implementation;
  AnimationImplementation implementation_bounce;
  uint16_t offset;
  uint8_t direction;
  AnimationCurve curve;
  int8_t adjustment;
  uint16_t duration;
} SlidingTextLayerData;


static void animate(SlidingTextLayer* layer, uint8_t direction);
static void animate_bounce(SlidingTextLayer* layer, uint8_t direction);
static void render(Layer* layer, GContext* ctx);
static void animation_started(Animation *anim, void *context);
static void animation_stopped(Animation *anim, bool stopped, void *context);
static void animation_stopped_bounce(Animation *anim, bool stopped, void *context);
static void update(Animation* anim, AnimationProgress dist_normalized);
static void update_bounce(Animation* anim, AnimationProgress dist_normalized);
#define get_layer(layer) (Layer*)layer
#define get_data(layer) (SlidingTextLayerData*) layer_get_data(get_layer(layer))
#define layer_get_height(layer) layer_get_bounds(layer).size.h
#define layer_get_width(layer) layer_get_bounds(layer).size.w

static int getDuration(){
  int multiplierNumber = ((rand() % 6) + 1) * 100;
  return multiplierNumber;
}

SlidingTextLayer* sliding_text_layer_create(GRect rect) {
  SlidingTextLayer* layer = (SlidingTextLayer*)layer_create_with_data(rect, sizeof(SlidingTextLayerData));
  SlidingTextLayerData* data = get_data(layer);
  data->color_text = GColorBlack;
  data->align = GTextAlignmentCenter;
  data->overflow = GTextOverflowModeFill;
  data->offset = 0;
  data->is_animating = false;
  data->implementation.update = update;
  data->implementation_bounce.update = update_bounce;
  data->curve = AnimationCurveEaseInOut;
  data->duration = getDuration();
  layer_set_update_proc(get_layer(layer), render);
  return layer;
}

void sliding_text_layer_destroy(SlidingTextLayer* layer) {
  SlidingTextLayerData* data = get_data(layer);
  if (data->is_animating) {

  }
  layer_destroy(get_layer(layer));
}

void sliding_text_layer_animate_up(SlidingTextLayer* layer) {
  animate(layer, ANIMATION_DIRECTION_UP);
}

void sliding_text_layer_animate_down(SlidingTextLayer* layer) {
  animate(layer, ANIMATION_DIRECTION_DOWN);
}

void sliding_text_layer_animate_bounce_up(SlidingTextLayer* layer) {
  animate_bounce(layer, ANIMATION_DIRECTION_UP);
}

void sliding_text_layer_animate_bounce_down(SlidingTextLayer* layer) {
  animate_bounce(layer, ANIMATION_DIRECTION_DOWN);
}

void sliding_text_layer_set_text(SlidingTextLayer* layer, char* text) {
  if (! layer) {
    return;
  }
  SlidingTextLayerData* data = get_data(layer);
  if (! data) {
    return;
  }
  free(data->text_current);
  data->text_current = text;
  layer_mark_dirty(get_layer(layer));
}

void sliding_text_layer_set_next_text(SlidingTextLayer* layer, char* text) {
  if (! layer) {
    return;
  }
  SlidingTextLayerData* data = get_data(layer);
  if (! data) {
    return;
  }
  data->text_next = text;
}

void sliding_text_layer_set_duration(SlidingTextLayer* layer, uint16_t duration) {
  if (! layer) {
    return;
  }
  SlidingTextLayerData* data = get_data(layer);
  if (! data) {
    return;
  }
  data->duration = duration;
}

void sliding_text_layer_set_animation_curve(SlidingTextLayer* layer, AnimationCurve curve) {
  if (! layer) {
    return;
  }
  SlidingTextLayerData* data = get_data(layer);
  if (! data) {
    return;
  }
  data->curve = curve;
}

void sliding_text_layer_set_font(SlidingTextLayer* layer, GFont font) {
  if (! layer) {
    return;
  }
  SlidingTextLayerData* data = get_data(layer);
  if (! data) {
    return;
  }
  data->font = font;
  layer_mark_dirty(get_layer(layer));
}

void sliding_text_layer_set_text_color(SlidingTextLayer* layer, GColor color) {
  if (! layer) {
    return;
  }
  SlidingTextLayerData* data = get_data(layer);
  if (! data) {
    return;
  }
  data->color_text = color;
  layer_mark_dirty(get_layer(layer));
}

void sliding_text_layer_set_text_alignment(SlidingTextLayer* layer, GTextAlignment alignment) {
  if (! layer) {
    return;
  }
  SlidingTextLayerData* data = get_data(layer);
  if (! data) {
    return;
  }
  data->align = alignment;
}

void sliding_text_layer_set_vertical_adjustment(SlidingTextLayer* layer, int8_t adjustment) {
  if (! layer) {
    return;
  }
  SlidingTextLayerData* data = get_data(layer);
  if (! data) {
    return;
  }
  data->adjustment = adjustment;
}

bool sliding_text_layer_is_animating(SlidingTextLayer* layer) {
  if (! layer) {
    return false;
  }
  SlidingTextLayerData* data = get_data(layer);
  if (! data) {
    return false;
  }
  return data->is_animating;
}


static void animate(SlidingTextLayer* layer, uint8_t direction) {
  if (! layer) {
    return;
  }
  SlidingTextLayerData* data = get_data(layer);
  if (! data) {
    return;
  }
  if (data->is_animating) {
    return;
  }

  data->direction = direction;
  data->offset = 0;
  data->animation = animation_create();
  animation_set_duration(data->animation, data->duration);
  animation_set_curve(data->animation, data->curve);
  animation_set_implementation(data->animation, &data->implementation);
  animation_set_handlers(data->animation, (AnimationHandlers) {
    .started = animation_started,
    .stopped = animation_stopped
  }, (void*)layer);
  animation_schedule(data->animation);
}

static void animate_bounce(SlidingTextLayer* layer, uint8_t direction) {
  if (! layer) {
    return;
  }
  SlidingTextLayerData* data = get_data(layer);
  if (! data) {
    return;
  }
  if (data->is_animating) {
    return;
  }

  data->direction = direction;
  data->offset = 0;
  data->animation = animation_create();
  animation_set_duration(data->animation, data->duration / 2);
  animation_set_curve(data->animation, data->curve);
  animation_set_implementation(data->animation, &data->implementation_bounce);
  animation_set_handlers(data->animation, (AnimationHandlers) {
    .started = animation_started,
    .stopped = animation_stopped_bounce
  }, (void*)layer);
  animation_schedule(data->animation);
}

static void render(Layer* layer, GContext* ctx) {
  SlidingTextLayerData* data = get_data(layer);
  graphics_context_set_text_color(ctx, data->color_text);

#if STL_DEBUG
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
#endif

  if (data->is_animating) {
    graphics_draw_text(ctx,
      data->direction == ANIMATION_DIRECTION_UP ? data->text_next : data->text_current,
      data->font,
      GRect(0, data->adjustment - data->offset, layer_get_width(layer), layer_get_height(layer)),
      data->overflow,
      data->align,
      NULL);
    graphics_draw_text(ctx,
      data->direction == ANIMATION_DIRECTION_UP ? data->text_current : data->text_next,
      data->font,
      GRect(0, layer_get_height(layer) + data->adjustment - data->offset, layer_get_width(layer), layer_get_height(layer)),
      data->overflow,
      data->align,
      NULL);
  }
  else {
    graphics_draw_text(ctx,
      data->text_current,
      data->font,
      GRect(0, data->adjustment, layer_get_width(layer), layer_get_height(layer)),
      data->overflow,
      data->align,
      NULL);
  }
}

static void animation_started(Animation *anim, void *context) {
#if STL_DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", heap_bytes_free());
#endif
  SlidingTextLayerData* data = get_data((SlidingTextLayer*)context);
  data->is_animating = true;
}

static void animation_stopped(Animation *anim, bool stopped, void *context) {
  SlidingTextLayerData* data = get_data((SlidingTextLayer*)context);
  data->is_animating = false;
  data->text_current = data->text_next;
  data->text_next = false;
#ifdef PBL_SDK_2
  animation_destroy(anim);
#endif
}

static void animation_stopped_bounce(Animation *anim, bool stopped, void *context) {
  SlidingTextLayerData* data = get_data((SlidingTextLayer*)context);
  data->is_animating = false;
}

static void update(Animation* anim, AnimationProgress dist_normalized) {
  SlidingTextLayer* layer = (SlidingTextLayer*)animation_get_context(anim);
  SlidingTextLayerData* data = get_data(layer);

  uint16_t percent = (100 * dist_normalized) / (ANIMATION_NORMALIZED_MAX);
  if (data->direction == ANIMATION_DIRECTION_UP) {
    data->offset = layer_get_height(layer) - ((uint16_t)(layer_get_height(layer) * percent) / 100);
  }
  else {
    data->offset = (uint16_t)((layer_get_height(layer) * percent) / 100);
  }
  layer_mark_dirty(get_layer(layer));
}

static void update_bounce(Animation* anim, AnimationProgress dist_normalized) {
  SlidingTextLayer* layer = (SlidingTextLayer*)animation_get_context(anim);
  SlidingTextLayerData* data = get_data(layer);

  uint16_t max_height = (layer_get_height(layer) * 100) / 3;

  uint16_t percent = (dist_normalized * 100) / ANIMATION_NORMALIZED_MAX;
  if (data->direction == ANIMATION_DIRECTION_UP) {
    data->offset = (layer_get_height(layer) - (uint16_t)(max_height * percent) / 10000);
  }
  else {
    data->offset = (uint16_t)((max_height * percent) / 10000);
  }
  layer_mark_dirty(get_layer(layer));
}
