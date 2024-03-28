// printf over CAN
//
// Copyright (c) 2013-2020 emotas embedded communication GmbH
//
// $Id: printf.js 33089 2020-08-13 08:41:05Z ro $
//

//----------------------------------------------------------------------
// call commands outside of event callbacks
// use example:
//   addGlobalCommand("nmt.resetCommNetwork()");
//
// This commands are called in the main loop.
// You have to add the callcommand there.
var globalCommands = [ "util.print(\"global command queue ready\")" ];

function addGlobalCommand(s)
{
	globalCommands.push(s);
	//util.print("add " + s);
}

function callGlobalCommands()
{
	if (globalCommands.length > 0) {
		//util.print("call " + globalCommands[0]);
		eval(globalCommands[0]);
		globalCommands.shift();	
	}
}

//
//----------------------------------------------------------------------

// helper function
function dateTime()
{
var currentDateTime = new Date();
var hours = currentDateTime.getHours();
var minutes = currentDateTime.getMinutes();
var seconds = currentDateTime.getSeconds();
var milli_seconds = currentDateTime.getMilliseconds();
var day = currentDateTime.getDate();
var month = currentDateTime.getMonth() + 1;
var year = currentDateTime.getFullYear();

var out = "";

	//date
	if (day < 10) {day = "0" + day;}      
	if (month < 10) {month = "0" + month;}
	out = out + "<b>" +"  @ "  + day + "." + month + "." + year + "</b>";

      // add time 
	if (milli_seconds < 10) {milli_seconds = "00" + milli_seconds;} 
      else if (milli_seconds < 100) {milli_seconds = "0" + milli_seconds;} 
	if (seconds < 10) {seconds = "0" + seconds;}
	if (minutes < 10) {minutes = "0" + minutes;}
	if (hours < 10) {hours = "0" + hours;}  

	out = out + " " + "<b>" + hours + ":" + minutes+ ":" + seconds + ","+ milli_seconds+  " " + "</b>";

	return out;       
}

//-------------------------------------------------------------------

// used CAN ID
//var printf_id = 0x001
//var printf_id = 0x002
var printf_id = 0x580

// reset all old setting
can.unregisterAllCanEvents();
util.deleteAllTimers();

// global String (if more than one CAN Message per String)
var output = new String("");
var timeout_timer = 0;

//---------------------------------------------------------
// callback function
function view_data(id, rtrFlag, dlc, d0, d1, d2, d3, d4, d5, d6, d7)
{
	addGlobalCommand("view_data_main(" 
		+ id + ","
		+ rtrFlag + ","
		+ dlc + ","
		+ d0 + ","
		+ d1 + ","
		+ d2 + ","
		+ d3 + ","
		+ d4 + ","
		+ d5 + ","
		+ d6 + ","
		+ d7 + ");");
}

//---------------------------------------------------------
// generate output string
function view_data_main(id, rtrFlag, dlc, d0, d1, d2, d3, d4, d5, d6, d7)
{
var data = new Array( d0, d1, d2, d3, d4, d5, d6, d7);
var nl = new Array(0, 10, 13); //newline chars
var i;
var c;

	if (timeout_timer != 0) {
		util.deleteTimer(timeout_timer);
		timeout_timer = 0;
	}

	for (i = 0; i < dlc; i++) {
		if ( nl.indexOf(data[i]) > -1) {
			util.print(dateTime()+output);
			output = "";
		} else {
			c = String.fromCharCode(data[i]);
			//util.print(c);
			output = output + c;

		}
	}
	if (output != "") {
		//set timeout 1s
		if (timeout_timer == 0) {
			timeout_timer = util.after(1000, "timeout()");
		}
	}
}

//---------------------------------------------------------
// timeout in case of no newline char
function timeout()
{
	util.print(dateTime() + output);
	output = "";

	// simulate one shoot timer
	util.deleteTimer(timeout_timer);
	timeout_timer = 0;

}

//---------------------------------------------------------
// initialization
function init_printf_view()
{
	can.registerCanEvent( printf_id, "view_data" );
}
init_printf_view();

util.print("script loaded");
util.print("!!! Press Stop before closing the CDE !!!");

//---------------------------------------------------------
//main loop
//---------------------------------------------------------
var running = 1;

//init solution to stop the script
button.setName( 1, "Stop" );
button.setCommand( 1, "running = 0;" );
button.setEnabled( 1, true );
util.callOnExit("running = 0;" );

// command loop
while (running == 1) {
	callGlobalCommands();
	util.msleep(10);
}
util.deleteAllTimers();

//---------------------------------------------------------
//end
//---------------------------------------------------------
