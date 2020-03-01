// Tom Trebisky  1-8-2014
// This creates a node server that supports
// a browser hosted GUI to control a motor
// driven by the PRU on the Beaglebone Black.

// Note:  with the screw and focus stage I am currently
// using, a single motor step gives .0005 inches of
// motion, which is 12.7 microns.
//
// The controller can microstep at 2, 4, or 8
// By not grounding the black and brown we get 8x
// microstepping is not yet software controlled

// var limit = 1000;
var limit = 3000;

// -------------------

var app = require('http').createServer(handler);
var io = require('socket.io').listen(app);
var fs = require('fs');
var b = require('bonescript');
var pru = require("pru");

var html_file = 'gui.html';
var my_port = 1776;

// OK
var motor_bit = "P8_11";
b.pinMode(motor_bit, b.OUTPUT);

//var dir_bit = "P8_6"; won't work
// OK
var dir_bit = "P8_8";
b.pinMode(dir_bit, b.OUTPUT);

// OK
var limit_bit = "P8_12";
b.pinMode ( limit_bit, b.INPUT );

// cannot use P8_3 or P8_4 as output bit.
// cannot use P8_5 or P8_6 as output bit.

// OK
var focus_bit = "P8_15";
b.pinMode(focus_bit, b.OUTPUT);

// OK
var shutter_bit = "P8_16";
b.pinMode(shutter_bit, b.OUTPUT);

// socket.io debug level - set 1 for warn, 2 for info, 3 for debug
var io_debug = 2;

var motor_rate;		// set in new_rate();

app.listen(my_port);

io.set('log level', io_debug);
// io.set('browser client minification', true);  // send minified client
io.set('browser client etag', true);  // apply etag caching logic based on version number

console.log('Server running on: http://' + getIPAddress() + ':' + my_port);

var limit_status = get_limit ();

function handler (req, res) {
  if (req.url == "/favicon.ico"){   // handle requests for favico.ico
	  res.writeHead(200, {'Content-Type': 'image/x-icon'} );
	  res.end();
	  // console.log('favicon requested');
	  return;
  }
  fs.readFile ( html_file,
	  function (err, data) {
	    if (err) {
	      res.writeHead(500);
	      return res.end('Error loading index.html');
	    }
	    res.writeHead(200);
	    res.end(data);
	  }
  );
}

// We get a zero when nothing is in the path
// (this is with a 2N7000 inverting and buffering
// the signal)
// This function returns true when we are in a limit.
function get_limit () {
	if ( b.digitalRead ( limit_bit ) == 0 )
		return 'false'
	else
		return 'true'
}

// runs at 1000 Hz
// now serves just to monitor limit detection
function ticker () {

	var new_limit = get_limit ();

	if ( new_limit != limit_status ) {
		limit_status = new_limit;
		// push on change
		io.sockets.emit ( 'limit', limit_status );
		console.log ( "limit " + limit_status );
	}
}

io.sockets.emit ( 'limit', limit_status );
console.log ( "limit " + limit_status );

// start timer running
setInterval ( ticker, 1 );

var nano = 1000000000;

var hi_delay = 2500;
var lo_delay = 250000;
var center = 100000;
var llim = center - limit;
var ulim = center + limit;

var botexp = center;
var topexp = center;
var num_steps = 1;
var exp_step = 10;

pru.init ();
pru.load ( "pru_eblink.bin" );

new_rate ( 400 );
pru.setDataRAM ( [ 0, 0, hi_delay, lo_delay, llim, ulim, center, 0  ] );

// set motor rate
function new_rate ( rate ) {
	motor_rate = rate;
	lo_delay = nano / 10 / rate;
	lo_delay -= hi_delay;
	pru.setDataRAMInt ( 3, lo_delay );
}

// run positive is a 1
function run_motor ( dir ) {
    b.digitalWrite ( dir_bit, dir );
    if ( dir ) {
	pru.setDataRAM ( [ 1, 0 ] );
    } else {
	pru.setDataRAM ( [ 256 + 1, 0 ] );
    }
    pru.run ();
}

function halt_motor () {
    pru.setDataRAM ( [ 0, 0 ] );
}

// Really should mark busy, use wait with callback,
// and queue commands while busy (or something like that)

function jog_motor ( val ) {
    if ( val == 0 ) return;
    if ( val < 0 ) {
	val = -val;
	b.digitalWrite ( dir_bit, 0 );
	pru.setDataRAM ( [ 256 + 2, val ] );
    } else {
	b.digitalWrite ( dir_bit, 1 );
	pru.setDataRAM ( [ 2, val ] );
    }
    pru.run ();
}

function move_motor ( val ) {
      var cur_pos = pru.getDataRAMInt ( 6 );
      jog_motor ( val - cur_pos );
}

// Like Jog, but override limit
function override ( val ) {
    if ( val == 0 ) return;
    if ( val < 0 ) {
	val = -val;
	b.digitalWrite ( dir_bit, 0 );
	pru.setDataRAM ( [ 512 + 256 + 2, val ] );
    } else {
	b.digitalWrite ( dir_bit, 1 );
	pru.setDataRAM ( [ 512 + 2, val ] );
    }
    pru.run ();
}

var fo = 0;
b.digitalWrite ( focus_bit, fo );

var sh = 0;
b.digitalWrite ( shutter_bit, sh );

function focusX () {
	fo = 1 - fo;
	b.digitalWrite ( focus_bit, fo );
	//console.log ( "focus " + fo );
}

function shutterX () {
	sh = 1 - sh;
	b.digitalWrite ( shutter_bit, sh );
	//console.log ( "shutter " + sh );
}

function focus_end () {
	b.digitalWrite ( focus_bit, 0 );
	//console.log ( "focus end" );
}

function focus () {
	b.digitalWrite ( focus_bit, 1 );
	//console.log ( "focus start" );
	setTimeout ( focus_end, 400 );
}

function shutter_end () {
	b.digitalWrite ( shutter_bit, 0 );
	//console.log ( "shutter end" );
}

// 100 ms duration works less than half the time.
// 250 ms duration works almost all the time.
function trip_shutter () {
	console.log ( "trip_shutter" );
	b.digitalWrite ( shutter_bit, 1 );
	setTimeout ( shutter_end, 500 );
}

// These two functions handle a motion loop
// in node.js style

var move_delay = 2000;		// settling time
var mirror_delay = 5000;	// settling time
var shutter_delay = 2000;	// settling time

var move_count;
var next_pos;
var inc_pos;

function next_move () {
    console.log ( "next move " + next_pos );
    move_motor ( next_pos );
    pru.wait ( move_done )
}

// move is done -- no delay really needed
// here if we are raising mirror w/ lockup.
function move_done () {
    console.log ( "move done " + move_count );
    pru.clearInterrupt ();
    setTimeout ( move_mirror, move_delay );
}

// raise mirror
function move_mirror () {
    console.log ( "move mirror " + move_count );
    trip_shutter ();
    setTimeout ( move_shutter, mirror_delay );
}

// capture image
function move_shutter () {
    console.log ( "move shutter " + move_count );
    trip_shutter ();

    --move_count;
    if ( move_count > 0 ) {
	next_pos += inc_pos;
	setTimeout ( next_move, shutter_delay );
    }
}

// ----------------------------------------------
 
io.sockets.on('connection', function (socket) {
  socket.on('run', function (dir) {
      run_motor (dir);
  });
  socket.on('halt', function () {
      halt_motor ();
  });
  socket.on('jog', function ( val ) {
      jog_motor ( val );
  });
  socket.on('over', function ( val ) {
      override ( val );
  });
  socket.on('setRate', function ( val ) {
	  new_rate ( val );
  });
  socket.on('getRate', function () {
	  socket.emit ( 'rate', motor_rate );
  });
  socket.on('getLimit', function () {
	  socket.emit ( 'limit', limit_status );
  });

  socket.on('focus', function () {
      focus ();
  });
  socket.on('shutter', function () {
      trip_shutter ();
  });

  socket.on('getPos', function () {
	  var cur_pos = pru.getDataRAMInt ( 6 );
	  socket.emit ( 'pos', cur_pos );
  });
  socket.on('setzero', function () {
	  pru.setDataRAMInt ( 6, center );
  });
  socket.on('move', function ( val ) {
      move_motor ( val );
  });

  socket.on('stow', function () {
      move_motor ( center );
  });

  socket.on('setnum', function ( val ) {
      //console.log ( "setnum " + val );
      num_steps = +val;
  });
  socket.on('setstep', function ( val ) {
      exp_step = +val;
  });
  socket.on('botexp', function () {
      botexp = pru.getDataRAMInt ( 6 );
  });
  socket.on('topexp', function () {
      topexp = pru.getDataRAMInt ( 6 );
  });

  // User gives counts per step
  socket.on('goexp', function () {
      console.log ( "GO step is " + exp_step );
      // Javascript idiom to convert string to int
      var num = Math.floor ( (topexp - botexp) / exp_step ) + 1;
      if ( exp_step * (num-1) >= (topexp - botexp ) )
	    num = num - 1;

      move_count = num + 1;
      next_pos = botexp;
      inc_pos = exp_step;

      console.log ( "GO count = " + move_count );
      next_move ();
  });

  // User gives number of steps (no what we really want).
  socket.on('goexpX', function () {
      console.log ( "GO " + num_steps );
      // Javascript idiom to convert string to int
      move_count = +num_steps + 1;
      next_pos = botexp;
      inc_pos = ( topexp - botexp) / num_steps;
      console.log ( "GO " + inc_pos );
      console.log ( "GO " + move_count );
      next_move ();
  });

  socket.on('gettop', function () {
      socket.emit ( 'top', topexp );
  });
  socket.on('getbot', function () {
      socket.emit ( 'bot', botexp );
  });
  socket.on('getnum', function () {
      socket.emit ( 'num', num_steps );
  });
  socket.on('getstep', function () {
      socket.emit ( 'step', exp_step );
  });

});

// Get server IP address on LAN
function getIPAddress() {
  var interfaces = require('os').networkInterfaces();
  for (var devName in interfaces) {
    var iface = interfaces[devName];
    for (var i = 0; i < iface.length; i++) {
      var alias = iface[i];
      if (alias.family === 'IPv4' && alias.address !== '127.0.0.1' && !alias.internal)
        return alias.address;
    }
  }
  return '0.0.0.0';
}
 
// THE END
