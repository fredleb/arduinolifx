/* Source: http://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both */

struct rgb {
    double r;       // percent
    double g;       // percent
    double b;       // percent
};

struct hsv {
    double h;       // angle in degrees
    double s;       // percent
    double v;       // percent
};

static hsv      rgb2hsv(rgb in);

hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if( max > 0.0 ) {
        out.s = (delta / max);                  // s
    } else {
        // r = g = b = 0                        // s = 0, v is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}



/* Convert Kelvin to RGB: based on http://bit.ly/1bc83he */

rgb kelvinToRGB(long kelvin) {
  rgb kelvin_rgb;
  long temperature = kelvin / 100;

  if(temperature <= 66) {
    kelvin_rgb.r = 255;
  } 
  else {
    kelvin_rgb.r = temperature - 60;
    kelvin_rgb.r = 329.698727446 * pow(kelvin_rgb.r, -0.1332047592);
    if(kelvin_rgb.r < 0) kelvin_rgb.r = 0;
    if(kelvin_rgb.r > 255) kelvin_rgb.r = 255;
  }

  if(temperature <= 66) {
    kelvin_rgb.g = temperature;
    kelvin_rgb.g = 99.4708025861 * log(kelvin_rgb.g) - 161.1195681661;
    if(kelvin_rgb.g < 0) kelvin_rgb.g = 0;
    if(kelvin_rgb.g > 255) kelvin_rgb.g = 255;
  } 
  else {
    kelvin_rgb.g = temperature - 60;
    kelvin_rgb.g = 288.1221695283 * pow(kelvin_rgb.g, -0.0755148492);
    if(kelvin_rgb.g < 0) kelvin_rgb.g = 0;
    if(kelvin_rgb.g > 255) kelvin_rgb.g = 255;
  }

  if(temperature >= 66) {
    kelvin_rgb.b = 255;
  } 
  else {
    if(temperature <= 19) {
      kelvin_rgb.b = 0;
    } 
    else {
      kelvin_rgb.b = temperature - 10;
      kelvin_rgb.b = 138.5177312231 * log(kelvin_rgb.b) - 305.0447927307;
      if(kelvin_rgb.b < 0) kelvin_rgb.b = 0;
      if(kelvin_rgb.b > 255) kelvin_rgb.b = 255;
    }
  }

  return kelvin_rgb;
}
