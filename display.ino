#include "Nextion.h"

/*
 * Declare a number object [page id:0,component id:5, component name: "angle"]. 
 */
NexNumber nex_angle = NexNumber(0, 5, "angle");

int counter;

void setup() {
    /* Set the baudrate which is for debug and communicate with Nextion screen. */
    nexInit();
     
    counter = 0;
}

void loop() {
  nex_angle.setValue(counter);
  counter = (counter + 1) % 91;
  delay(1000);
}
