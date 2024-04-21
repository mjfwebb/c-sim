
#include "colors.h"

#include <math.h>
#include <stdbool.h>

#include "defs.h"

HSV rgb_to_hsv(RGBA rgb) {
  HSV color = {0};
  float rd = rgb.r;
  float gd = rgb.g;
  float bd = rgb.b;
  float tmax = max(rd, max(gd, bd));
  float tmin = min(rd, min(gd, bd));
  float dt = tmax - tmin;

  if (floats_equal(dt, 0)) {
    color.h = 0;
  } else if (floats_equal(tmax, rd)) {
    color.h = fmodf((gd - bd) / dt, 6);
  } else if (floats_equal(tmax, gd)) {
    color.h = (bd - rd) / dt + 2;
  } else {
    color.h = (rd - gd) / dt + 4;
  }

  color.h *= 60;
  if (color.h < 0) {
    color.h += 360;
  }

  color.s = (floats_equal(tmax, 0)) ? 0 : dt / tmax;
  color.v = tmax;
  return color;
}

RGBA hsv_to_rgb(HSV hsv) {
  RGBA color;

  // Red channel
  float k = fmodf((5.0f + hsv.h / 60.0f), 6);
  float t = 4.0f - k;
  k = (t < k) ? t : k;
  k = (k < 1) ? k : 1;
  k = (k > 0) ? k : 0;
  color.r = (hsv.v - hsv.v * hsv.s * k);

  // Green channel
  k = fmodf((3.0f + hsv.h / 60.0f), 6);
  t = 4.0f - k;
  k = (t < k) ? t : k;
  k = (k < 1) ? k : 1;
  k = (k > 0) ? k : 0;
  color.g = (hsv.v - hsv.v * hsv.s * k);

  // Blue channel
  k = fmodf((1.0f + hsv.h / 60.0f), 6);
  t = 4.0f - k;
  k = (t < k) ? t : k;
  k = (k < 1) ? k : 1;
  k = (k > 0) ? k : 0;
  color.b = (hsv.v - hsv.v * hsv.s * k);
  color.a = 1;

  return color;
}