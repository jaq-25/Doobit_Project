#include "WS_Flow.h"

Adafruit_NeoMatrix Matrix = Adafruit_NeoMatrix(
  8, 8, RGB_Control_PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB + NEO_KHZ800
);

void Matrix_Init() {
  Matrix.begin();
  Matrix.setBrightness(5); 
  Matrix.fillScreen(0);
  Matrix.show();
}