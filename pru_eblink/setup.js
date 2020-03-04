// Do the bare minimum to setup a pin for digital output
// before we hand it to the PRU

var b = require('bonescript');
var led_bit = "P8_11";
b.pinMode(led_bit, b.OUTPUT);
// b.digitalWrite(led_bit,0);
