var b = require('bonescript');
//var pru = require("pru");
var pru = require("./build/Release/pru");

// Do the bare minimum to setup a pin for digital output
// before we hand it to the PRU
var led_bit = "P8_11";
b.pinMode(led_bit, b.OUTPUT);
// b.digitalWrite(led_bit,0);

pru.init ();

pru.load ( "pru_eblink.bin" );
pru.setDataRAM ( [ 2, 25, 2500, 250000 ] );

function next () {
    console.log ( "hey" );
    pru.reset ();
    pru.run ();
    pru.block ();
}

// setTimeout ( next, 1000 );
setInterval ( next, 250 );
