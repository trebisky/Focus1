var b = require('bonescript');
var pru = require("pru");

// Do the bare minimum to setup a pin for digital output
// before we hand it to the PRU
var led_bit = "P8_11";
b.pinMode(led_bit, b.OUTPUT);
// b.digitalWrite(led_bit,0);

pru.init ();

pru.load ( "pru_eblink.bin" );

//pru.setDataRAM ( [ 1, 0, 2500, 250000 ] );
//pru.enable ();

// This works (stops continuous motion)
// pru.setDataRAM ( [ 0, 0, 2500, 250000 ] );

// start continual motion
pru.setDataRAM ( [ 1, 0, 2500, 250000 ] );

//pru.reset ();

pru.run ();

// quit without disabling PRU
pru.quit ();
