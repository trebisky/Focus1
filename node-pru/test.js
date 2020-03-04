var b = require('bonescript');
//var pru = require("pru");
var pru = require("./build/Release/pru");

// See if this sets up a busy loop

function handler (req, res) {
  res.writeHead(200, {'Content-Type': 'text/plain'} );
  res.write("Hello");
  res.end();
  console.log('something requested');
}

var my_port = 2000;
var app = require('http').createServer(handler);
app.listen(my_port);

// End of web server experiment

// Do the bare minimum to setup a pin for digital output
// before we hand it to the PRU
var led_bit = "P8_11";
b.pinMode(led_bit, b.OUTPUT);
// b.digitalWrite(led_bit,0);

pru.init ();

pru.load ( "pru_eblink.bin" );

function next () {
    console.log ( "hey" );
}

// setTimeout ( next, 1000 );
setInterval ( next, 1000 );

console.log ( "enter busy loop" );
for ( ;; )
    ;
console.log ( "done with busy loop" );

for ( ;; ) {
    pru.setDataRAM ( [ 2, 100, 2500, 250000 ] );
    pru.block ();
    setTimeout ( func, 1000 );
}

pru.setDataRAM ( [ 1, 0, 2500, 250000 ] );

// This works (stops continuous motion)
// pru.setDataRAM ( [ 0, 0, 2500, 250000 ] );

// pru.setDataRAM ( [ 2, 1600, 2500, 250000 ] );

//pru.reset ();

pru.run ();

// pru.wait ( function () { console.log ( "done\n" ); } );

// Thinking this is ruby or something, I do this, and it works!
// It works (doing who knows what) for any word I put there.
//pru.block
pru.block ();

// quit without disabling PRU
pru.quit ();

console.log ( "exiting" );

// THE END
