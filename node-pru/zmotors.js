// Creates a websocket with socket.io

var app = require('http').createServer(handler);
var io = require('socket.io').listen(app);
var fs = require('fs');
var b = require('bonescript');
var html_file = 'motors.html';
var led_status = 'off';

// socket.io debug level - set 1 for warn, 2 for info, 3 for debug
var io_debug = 2;
var my_port = 1776;

var led_bit = "P8_11";
//var led_bit = "P8_19";

// The following don't work
//var led_bit = "P8_4";

// Something odd is up with P8_21, not working for general output,
// but it is changing state in some strange way.
// var led_bit = "P8_21"; 
var limit_bit = "P8_17";

app.listen(my_port);

io.set('log level', io_debug);
// io.set('browser client minification', true);  // send minified client
io.set('browser client etag', true);  // apply etag caching logic based on version number

console.log('Server running on: http://' + getIPAddress() + ':' + my_port);

b.pinMode(led_bit, b.OUTPUT);
//b.analogWrite(led_bit,1);

b.digitalWrite(led_bit,0);
led_status = 'off';

//b.digitalWrite(led_bit,1);
//led_status = 'on';

b.pinMode ( limit_bit, b.INPUT );
var limit_status = 'false';

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

// This is all about step pule generation for the R208 stepper driver.
// I began generating perfect square waves, but this is dumb.
// The R208 will be happy with a 20 microsecond pulse.
// It has a 30 kHz maximum step rate.
// Also, the Beagle has all kinds of fancy modes for the pins.
// I can almost certainly set up a one-shot mode to spit out the pulse.
// There may well be special modes ideal for driving stepper motors.
// XXX - watch out for an off by one error on the delay count down.

// var blink_flag = false;
var blink_flag = true;
var blink_rate;		// set in new_rate();
var blink_idelay;	// set in new_rate();
var blink_delay;	// set in new_rate();
var blink_state = 0;

function push_status ( arg ) {
	if ( blink_rate > 10 ) return;
	if ( arg == 0 )
		io.sockets.emit ( 'status', 'off' );
	else
		io.sockets.emit ( 'status', 'on' );
}

function get_limit () {
	if ( b.digitalRead ( limit_bit ) == 0 )
		return 'true'
	else
		return 'false'
}

// actually runs at about 300 Hz
function tickerX () {

	b.digitalWrite ( led_bit, 1 );
	b.digitalWrite ( led_bit, 0 );
}

// runs at 1000 Hz
function ticker () {

	var new_limit = get_limit ();

	if ( new_limit != limit_status ) {
		limit_status = new_limit;
		// push on change
		io.sockets.emit ( 'limit', limit_status );
	}

	if ( blink_flag == false ) return;

	if ( blink_delay > 0 ) {
		blink_delay--;
		return;
	}

	blink_state = 1 - blink_state;
	//console.log ( "blink " + blink_state );
	b.digitalWrite ( led_bit, blink_state );
	push_status ( blink_state );
	blink_delay = blink_idelay;
}

function new_rate ( rate ) {
	blink_idelay = Math.floor ( 1000 / (2*rate) );
	blink_rate = 1000 / (2*blink_idelay);
	blink_delay = blink_idelay;
}

// set the default 1 Hz rate
new_rate ( 1 );

// start timer running
blink_delay = blink_idelay;
setInterval ( ticker, 1 );
 
io.sockets.on('connection', function (socket) {
  socket.on('ledOn', function (data) {
        b.digitalWrite(led_bit, 1);
	led_status = 'on';
	socket.emit ( 'status', led_status );
    // console.log('On: ' + data);
  });
  socket.on('ledOff', function (data) {
        b.digitalWrite(led_bit, 0);
	led_status = 'off';
	socket.emit ( 'status', led_status );
    // console.log('Off: ' + data);
  });
  socket.on('startBlink', function () {
      blink_flag = true;
  });
  socket.on('stopBlink', function () {
      blink_flag = false;
  });
  socket.on('setRate', function ( val ) {
	  new_rate ( val );
  });
  socket.on('getRate', function () {
	  socket.emit ( 'rate', blink_rate );
  });
  socket.on('getStatus', function () {
	  socket.emit ( 'status', led_status );
  });
  socket.on('getLimit', function () {
	  socket.emit ( 'limit', limit_status );
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
