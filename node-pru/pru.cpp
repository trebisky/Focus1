//System headers
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <pthread.h>

// tjt adds to get debug
#include <iostream>
using namespace std;

/* define this to use some new API calls
 *  added by T. Trebisky at the MMT
 */
#define NEWLIB

//PRU Driver headers
#include <prussdrv.h>
#include <pruss_intc_mapping.h>	 

#define OFFSET_SHAREDRAM 2048

#define PRU_NUM 	 0

//Node.js addon headers
#include <node.h>
#include <v8.h>

using namespace v8;

//shared memory pointer
static unsigned int* sharedMem_int;

// data memory pointer
static unsigned int* dataMem_int;

/* Initialise the PRU
 *	Initialise the PRU driver and static memory
 *	Takes no arguments and returns nothing
 */
Handle<Value> InitPRU(const Arguments& args) {
	HandleScope scope;
	static void *mp;
	
	cerr << "InitPRU" << endl;

#ifdef NEWLIB
	prussdrv_initialize ();
#else
	//Initialise driver
	prussdrv_init ();
	
	//Open interrupt
	unsigned int ret = prussdrv_open(PRU_EVTOUT_0);
	if (ret) {
		printf("prussdrv_open open failed\n");
		return scope.Close(Undefined());
	}
	
	//Initialise interrupt
	tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;	
	prussdrv_pruintc_init(&pruss_intc_initdata);
#endif
	
	// Allocate shared PRU memory
	prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, &mp);
	sharedMem_int = (unsigned int*) mp;

	// Allocate data PRU memory (NEW)
	prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, &mp);
	dataMem_int = (unsigned int*) mp;
	
	//Return nothing
	return scope.Close(Undefined());
}

/* Load PRU program
 *	Takes a single string argument, the filename of the .bin
 */
Handle<Value> loadProgram(const Arguments& args) {
	HandleScope scope;
	
	cerr << "load" << endl;
	//Check we have a single argument
	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	//Check that it's a string
	if (!args[0]->IsString()) {
		ThrowException(Exception::TypeError(String::New("Argument must be a string")));
		return scope.Close(Undefined());
	}
	
	//Get a C++ string
	String::Utf8Value program(args[0]->ToString());
	std::string programS = std::string(*program);
	
	//Execute the program
	prussdrv_load_program (PRU_NUM, (char*)programS.c_str());
	
	//Return nothing
	return scope.Close(Undefined());
};

/* NEW */
Handle<Value> enablePRU(const Arguments& args) {
	HandleScope scope;

	cerr << "enable" << endl;
	prussdrv_pru_enable ( PRU_NUM );
	
	//Return nothing
	return scope.Close(Undefined());
}

/* NEW */
Handle<Value> resetPRU(const Arguments& args) {
	HandleScope scope;

	cerr << "reset" << endl;
	prussdrv_pru_reset ( PRU_NUM );
	
	//Return nothing
	return scope.Close(Undefined());
}


/* Execute PRU program
 *	Takes a single string argument, the filename of the .bin
 */
Handle<Value> executeProgram(const Arguments& args) {
	HandleScope scope;
	
	//Check we have a single argument
	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	//Check that it's a string
	if (!args[0]->IsString()) {
		ThrowException(Exception::TypeError(String::New("Argument must be a string")));
		return scope.Close(Undefined());
	}
	
	//Get a C++ string
	String::Utf8Value program(args[0]->ToString());
	std::string programS = std::string(*program);
	
	//Execute the program
	prussdrv_exec_program (PRU_NUM, (char*)programS.c_str());
	
	//Return nothing
	return scope.Close(Undefined());
};


/* Set the shared PRU RAM to an input array
 *	Takes an integer array as input, writes it to PRU shared memory
 *	Not much error checking here, don't pass in large arrays or seg faults will happen
 *	TODO: error checking and allow user to select range to set
 */
Handle<Value> setSharedRAM(const Arguments& args) {
	HandleScope scope;
	
	//Check we have a single argument
	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	//Check that it's an array
	if (!args[0]->IsArray()) {
		ThrowException(Exception::TypeError(String::New("Argument must be array")));
		return scope.Close(Undefined());
	}
	
	//Get array
	Local<Array> a = Array::Cast(*args[0]);
	
	//Iterate over array
	for (unsigned int i = 0; i<a->Length(); i++) {
		//Get element and check it's numeric
		Local<Value> element = a->Get(i);
		if (!element->IsNumber()) {
			ThrowException(Exception::TypeError(String::New("Array must be integer")));
			return scope.Close(Undefined());
		}
		
		//Set corresponding memory bytes
		sharedMem_int[OFFSET_SHAREDRAM + i] = (unsigned int) element->NumberValue();
	}
	
	//Return nothing
	return scope.Close(Undefined());
};

// NEW
Handle<Value> setDataRAM(const Arguments& args) {
	HandleScope scope;
	
	//Check we have a single argument
	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	//Check that it's an array
	if (!args[0]->IsArray()) {
		ThrowException(Exception::TypeError(String::New("Argument must be array")));
		return scope.Close(Undefined());
	}
	
	//Get array
	Local<Array> a = Array::Cast(*args[0]);
	
	//Iterate over array
	for (unsigned int i = 0; i<a->Length(); i++) {
		//Get element and check it's numeric
		Local<Value> element = a->Get(i);
		if (!element->IsNumber()) {
			ThrowException(Exception::TypeError(String::New("Array must be integer")));
			return scope.Close(Undefined());
		}
		
		//Set corresponding memory bytes
		dataMem_int[i] = (unsigned int) element->NumberValue();
	}
	
	//Return nothing
	return scope.Close(Undefined());
};

/* Set a single integer value in shared RAM
 *	Takes two integer arguments, index and value
 */
Handle<Value> setSharedRAMInt(const Arguments& args) {	//array
	HandleScope scope;
	
	//Check that we have 2 arguments
	if (args.Length() != 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	//Check they are both numbers
	if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Arguments must be Integer")));
		return scope.Close(Undefined());
	}
	
	//Get the numbers
	unsigned short index = (unsigned short)Array::Cast(*args[0])->NumberValue();
	unsigned int val = (unsigned int)Array::Cast(*args[1])->NumberValue();
	
	//Set the memory date
	sharedMem_int[OFFSET_SHAREDRAM + index] = val;
	
	//Return nothing
	return scope.Close(Undefined());
};

// NEW
Handle<Value> setDataRAMInt(const Arguments& args) {
	HandleScope scope;
	
	//Check that we have 2 arguments
	if (args.Length() != 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	//Check they are both numbers
	if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Arguments must be Integer")));
		return scope.Close(Undefined());
	}
	
	//Get the numbers
	unsigned short index = (unsigned short)Array::Cast(*args[0])->NumberValue();
	unsigned int val = (unsigned int)Array::Cast(*args[1])->NumberValue();
	
	//Set the memory date
	dataMem_int[index] = val;
	
	//Return nothing
	return scope.Close(Undefined());
};

/* Get array from shared memory
 *	Returns first 16 integers from shared memory
 *	TODO: should take start and size integers as input to let user select array size
 */
Handle<Value> getSharedRAM(const Arguments& args) {	//array
	HandleScope scope;
	
	//Create output array
	Local<Array> a = Array::New(16);
	
	//Iterate over output array and fill it with shared memory data
	for (unsigned int i = 0; i<a->Length(); i++) {
		a->Set(i,Number::New(sharedMem_int[OFFSET_SHAREDRAM + i]));
	}
	
	//Return array
	return scope.Close(a);
};

/* Get single integer from shared memory
 *	Takes integer index argument, returns array at that index
 */
Handle<Value> getSharedRAMInt(const Arguments& args) {	//array
	HandleScope scope;
	
	//Check we have single argument
	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	//Check it's a number
	if (!args[0]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Argument must be Integer")));
		return scope.Close(Undefined());
	}
	
	//Get index value
	unsigned short index = (unsigned short)Array::Cast(*args[0])->NumberValue();
	
	//Return memory
	return scope.Close(Number::New(sharedMem_int[OFFSET_SHAREDRAM + index]));
};

// NEW
Handle<Value> getDataRAM(const Arguments& args) {
	HandleScope scope;
	
	//Create output array
	Local<Array> a = Array::New(16);
	
	//Iterate over output array and fill it with shared memory data
	for (unsigned int i = 0; i<a->Length(); i++) {
		a->Set(i,Number::New(dataMem_int[i]));
	}
	
	//Return array
	return scope.Close(a);
};

// NEW
Handle<Value> getDataRAMInt(const Arguments& args) {
	HandleScope scope;
	
	//Check we have single argument
	if (args.Length() != 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}
	
	//Check it's a number
	if (!args[0]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Argument must be Integer")));
		return scope.Close(Undefined());
	}
	
	//Get index value
	unsigned short index = (unsigned short)Array::Cast(*args[0])->NumberValue();
	
	//Return memory
	return scope.Close(Number::New(dataMem_int[index]));
};


/*-------------------This is mostly copy/pasted from here: ---------------------*/
/*----------------http://kkaefer.github.io/node-cpp-modules/--------------------*/
struct Baton {
    uv_work_t request;
    Persistent<Function> callback;
    int error_code;
    std::string error_message;
    int32_t result;
};

void AsyncWork(uv_work_t* req) {
//    Baton* baton = static_cast<Baton*>(req->data);

	prussdrv_pru_wait_event(PRU_EVTOUT_0);
}

void AsyncAfter(uv_work_t* req, int status) {
    HandleScope scope;
    Baton* baton = static_cast<Baton*>(req->data);
	baton->callback->Call(Context::GetCurrent()->Global(), 0, 0);
    baton->callback.Dispose();
    delete baton;
}

Handle<Value> waitForInterrupt(const Arguments& args) {
	HandleScope scope;
	Local<Function> callback = Local<Function>::Cast(args[0]);

	Baton* baton = new Baton();
	baton->request.data = baton;
	baton->callback = Persistent<Function>::New(callback);
	
	uv_queue_work(uv_default_loop(), &baton->request, AsyncWork,
		(uv_after_work_cb)AsyncAfter);

	return scope.Close(Undefined());
}

/*---------------------------Here ends the copy/pasting----------------------------*/

/* Clear Interrupt */
Handle<Value> clearInterrupt(const Arguments& args) {
	HandleScope scope;

	prussdrv_pru_clear_event(PRU0_ARM_INTERRUPT,PRU_EVTOUT_0);

	return scope.Close(Undefined());
};

Handle<Value> interruptPRU(const Arguments& args) {
	HandleScope scope;

	prussdrv_pru_send_event(ARM_PRU0_INTERRUPT);

	return scope.Close(Undefined());
};


/* NEW */
Handle<Value> disablePRU(const Arguments& args) {
	HandleScope scope;

	prussdrv_pru_disable(PRU_NUM); 

	return scope.Close(Undefined());
};

/* NEW */
Handle<Value> quitPRU(const Arguments& args) {
	HandleScope scope;

	cerr << "quit" << endl;
	prussdrv_exit ();

	return scope.Close(Undefined());
};

/* NEW and changed --
 * Force the PRU code to terminate
 *  Used to be called "exit"
 */
Handle<Value> donePRU (const Arguments& args) {
	HandleScope scope;

	cerr << "done" << endl;
	prussdrv_pru_disable(PRU_NUM); 
	prussdrv_exit ();

	return scope.Close(Undefined());
};

/* NEW and evil */
Handle<Value> blockPRU(const Arguments& args) {
	HandleScope scope;

	cerr << "block" << endl;
	prussdrv_pru_wait_event(PRU_EVTOUT_0);
	prussdrv_pru_clear_event(PRU0_ARM_INTERRUPT,PRU_EVTOUT_0);

	return scope.Close(Undefined());
};


/* Initialise the module */
// void Init(Handle<Object> exports, Handle<Object> module) {
void Init(Handle<Object> exports) {
	//	pru.init();
	exports->Set(String::NewSymbol("init"), FunctionTemplate::New(InitPRU)->GetFunction());
	
	//	pru.execute("mycode.bin");
	exports->Set(String::NewSymbol("execute"), FunctionTemplate::New(executeProgram)->GetFunction());

	// NEW	pru.load("mycode.bin");
	exports->Set(String::NewSymbol("load"), FunctionTemplate::New(loadProgram)->GetFunction());

	// NEW	pru.run();
	exports->Set(String::NewSymbol("run"), FunctionTemplate::New(enablePRU)->GetFunction());
	// NEW	pru.enable();  - same as above
	exports->Set(String::NewSymbol("enable"), FunctionTemplate::New(enablePRU)->GetFunction());

	// NEW	pru.disable();
	exports->Set(String::NewSymbol("disable"), FunctionTemplate::New(disablePRU)->GetFunction());

	// NEW	pru.reset();
	exports->Set(String::NewSymbol("reset"), FunctionTemplate::New(resetPRU)->GetFunction());
	
	//	pru.setSharedRAM ([0x1, 0x2, 0x3]);
	exports->Set(String::NewSymbol("setSharedRAM"), FunctionTemplate::New(setSharedRAM)->GetFunction());
	
	//	pru.setSharedRAMInt (4, 0xff);
	exports->Set(String::NewSymbol("setSharedRAMInt"), FunctionTemplate::New(setSharedRAMInt)->GetFunction());

	//	var intArray = pru.getSharedRAM();  -- array of 16
	exports->Set(String::NewSymbol("getSharedRAM"), FunctionTemplate::New(getSharedRAM)->GetFunction());

	//	var intVal = pru.getSharedRAMInt (3);
	exports->Set(String::NewSymbol("getSharedRAMInt"), FunctionTemplate::New(getSharedRAMInt)->GetFunction());

	// NEW	pru.setDataRAM([0x1, 0x2, 0x3]);
	exports->Set(String::NewSymbol("setDataRAM"), FunctionTemplate::New(setDataRAM)->GetFunction());
	
	// NEW	pru.setDataRAMInt (4, 0xff);
	exports->Set(String::NewSymbol("setDataRAMInt"), FunctionTemplate::New(setDataRAMInt)->GetFunction());

	// NEW	var intArray = pru.getDataRAM();
	exports->Set(String::NewSymbol("getDataRAM"), FunctionTemplate::New(getDataRAM)->GetFunction());
	
	// NEW	var intVal = pru.getDataRAMInt (3);
	exports->Set(String::NewSymbol("getDataRAMInt"), FunctionTemplate::New(getDataRAMInt)->GetFunction());
	
	//	pru.waitForInterrupt(function() { console.log("Interrupted by PRU");});
	exports->Set(String::NewSymbol("waitForInterrupt"), FunctionTemplate::New(waitForInterrupt)->GetFunction());
	exports->Set(String::NewSymbol("wait"), FunctionTemplate::New(waitForInterrupt)->GetFunction());

	//	pru.clearInterrupt();
	exports->Set(String::NewSymbol("clearInterrupt"), FunctionTemplate::New(clearInterrupt)->GetFunction());
	
	//	pru.interrupt();
	exports->Set(String::NewSymbol("interrupt"), FunctionTemplate::New(interruptPRU)->GetFunction());	

	// NEW	pru.block();  XXX
	// for test and experimentation only -- you don't really want to do this
	// to node (i.e. block waiting for an event), but I found this useful for testing.
	// especially when "wait" was not working as I expected.

	exports->Set(String::NewSymbol("block"), FunctionTemplate::New(blockPRU)->GetFunction());
	
	//	pru.quit(); -- exit without disabling
	exports->Set(String::NewSymbol("quit"), FunctionTemplate::New(quitPRU)->GetFunction());

	//	pru.done(); -- used to be called exit
	exports->Set(String::NewSymbol("done"), FunctionTemplate::New(donePRU)->GetFunction());
}

NODE_MODULE(pru, Init)
