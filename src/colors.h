typedef struct {
  float r;
  float g;
  float b;
  float a;
} RGBA;

typedef struct {
  float h;
  float s;
  float v;
} HSV;

HSV rgb_to_hsv(RGBA rgb);
RGBA hsv_to_rgb(HSV hsv);