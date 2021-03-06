# MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

# This file defines options and types of messages,
# it will be processed by messagesGen.pl.


# === SYNTAX OF THIS FILE ===

# The file will be executed by Perl 5 interpreter and should consist of one hash
# with 'modules', 'messages', and 'options' subhashes.

# Message has these attributes:
# 	args:       Data types of arguments. (default: no arguments)
# 	call:       Functions to be called with arguments of given types.
# 	pause:      Tasks (without 'Task' suffix) to be suspended before calling the functions.
# 	aliases:    Commands to send the message from console, the first one is preferred.
# 	enabled:    Command will be accessible from console if "true" (default), set to "false" otherwise.

# Option has these attributes:
# 	type:       Data type of the variable.
# 	default:    Default value of the variable. (default: 0/NULL)
# 	call:       Function to be called with no arguments after changing the value.
# 	pause:      Tasks (without 'Task' suffix) to be suspended before value change and functions calls.
# 	initCall:   Set to 1 to call the functions even for the default value.
# 	aliases:    Names in the console, the first one is preferred.
# 	enabled:    Option will be writable from console if "true" (default), set to "false" otherwise.
# 	unit:       String printed after the value in the console.
# 	validCond:  C condition verifying validity of value in 'arg0', also async.
# 	invalidStr: Error message to be printed if validCond is false. (default: "Wrong value.")

# All attributes are optional.

# Allowed data types:
# 	bool
# 	int
# 	float
# 	double
# 	string:     Standard C string (char*).
# 	path:       Like string, console provides auto-complete functionality.

# Header files to be included should be specified via modules list.



# === APPLICATION INTERFACE ===

# To send message NAME with arguments of types TYPE0, TYPE1, TYPE2 use:
# 	void msgSend_NAME(TYPE0 arg0, TYPE1 arg1, TYPE2 arg2);
# 		(same signature as functions to be called),
# 	void msgSend(msgType_NAME, void *null_terminated_arr_of_args_ptr[]);
# The call will return immediately.

# To read value of option NAME of type TYPE use directly variable
# 	TYPE msgOption_NAME;    (only from tasks to be paused)
# to change the value use
# 	void msgSet_NAME(TYPE newValue);

# To enable/disable command/option NAME use:
# 	void msgSetEnabled_NAME(bool value);
# 	void msgSetOptionEnabled_NAME(bool value);

# To send message / set option only if enabled use:
# 	void msgSendUser_NAME(TYPE0 arg0, TYPE1 arg1, TYPE2 arg2);
# 	void msgSetUser_NAME(TYPE newValue);

# Aliases can be accessed via NULL-terminated array
# 	struct msgAlias msgAliases[];



# === CONSOLE INTERFACE ===

# To send message with ALIAS and 3 arguments use:
# 	:ALIAS arg1 arg2 arg3

# To read all aliased options use:
# 	:set
# To read value of option with ALIAS use:
# 	:set ALIAS                  (for non-bool only)
# 	:set ALIAS?
# To change the value use:
# 	:set ALIAS=newValue         (for non-bool only)
# 	:set ALIAS / :set noALIAS   (for bool)



# === DEFINITION OF OPTIONS AND TYPES ===

modules => [qw( main drawer drawerScale drawerMusicVisualiser consoleOut player playerFile resampler consoleStatus )],

messages => {
	print => {
		args    => [qw( string )],
		pause   => [qw( drawerConsole )],
		call    => [qw( consolePrint )],
	},
	printErr => {
		args    => [qw( string )],
		pause   => [qw( drawerConsole )],
		call    => [qw( consolePrintErr )],
	},
	consoleClear => {
		pause   => [qw( drawerConsole )],
		call    => [qw( consoleClear )],
	},
	newSource => {
		pause      => [qw( consoleStatus drawerMain dmv rs )],
		call       => [qw( drawerReset dmvReset dsResetTimeScale dsResetToneScale rsResetHard )],
	},
	drawerReset => {
		pause      => [qw( drawerMain )],
		call       => [qw( drawerReset )],
	},
	open => {
		aliases    => [qw( open o )],
		call       => [qw( playerOpen )],
		args       => [qw( path )],
		pause      => [qw( playerFile playerFileThread consoleStatus dmv rs )],
	},
	openDevice => {
		aliases    => [qw( opendevice )],
		call       => [qw( playerOpenDevice )],
		args       => [qw( double )],
		pause      => [qw( playerFile playerFileThread consoleStatus dmv rs )],
	},
	openDeviceDefault => {
		aliases    => [qw( opendevice )],
		call       => [qw( playerOpenDeviceDefault )],
		pause      => [qw( playerFile playerFileThread consoleStatus dmv rs )],
	},
	close => {
		aliases    => [qw( close c )],
		call       => [qw( playerOpenLogo )],
		pause      => [qw( playerFile playerFileThread consoleStatus dmv rs )],
	},
	play => {
		aliases    => [qw( play )],
		call       => [qw( playerPlay )],
		pause      => [qw( playerFile playerFileThread )],
	},
	pause => {
		aliases    => [qw( pause )],
		call       => [qw( playerPause )],
		pause      => [qw( playerFile playerFileThread )],
	},
	seekAbs => {
		aliases    => [qw( seekto )],
		args       => [qw( double )],
		call       => [qw( playerSeekAbs )],
		pause      => [qw( playerFile playerFileThread )],
	},
	seekRel => {
		aliases    => [qw( seek )],
		args       => [qw( double )],
		call       => [qw( playerSeekRel )],
		pause      => [qw( playerFile playerFileThread )],
	},
	quit => {
		aliases => [qw( quit q )],
		call => [qw( mainExit )],
	},
},

options => {
	columnLen => {
		type       => "int",
		pause      => [qw( drawerMain dmv )],
		call       => [qw( dmvReset dsResetToneScale )],
	},
	a1Freq => {
		type       => "double",
		pause      => [qw( drawerMain dmv )],
		call       => [qw( dmvReset dsResetToneScale )],
		aliases    => [qw( a1freq a1 )],
		default    => 440,
		validCond  => "arg0 > 0",
		unit       => "Hz",
	},
	minFreq => {
		type       => "double",
		pause      => [qw( drawerMain dmv rs )],
		call       => [qw( dmvReset dsResetToneScale rsResetSoft )],
		aliases    => [qw( lowesttone lt )],
		default    => 16,
		validCond  => "(arg0 > 0) && (2*arg0 <= msgOption_maxFreq)",
		unit       => "Hz",
	},
	maxFreq => {
		type       => "double",
		pause      => [qw( drawerMain dmv rs )],
		call       => [qw( dmvReset dsResetToneScale rsResetSoft )],
		aliases    => [qw( highesttone ht )],
		default    => 22000,
		validCond  => "arg0 >= 2*msgOption_minFreq",
		unit       => "Hz",
	},
	outputRate => {
		type       => "double",
		pause      => [qw( drawerMain dmv rs )],
		call       => [qw( dmvReset drawerReset dsResetTimeScale dsResetToneScale rsResetSoft )],
		aliases    => [qw( rate rt )],
		default    => 100,
		validCond  => "(arg0 > 0) && (arg0 <= playerSampleRate)",
		unit       => "Hz",
	},
	showGrid => {
		type       => "bool",
		aliases    => [qw( showgrid sg )],
		default    => "true",
		pause      => [qw( drawerMain dmv )],
		call       => [qw( drawerReset )],
	},
	showCursor => {
		type       => "bool",
		aliases    => [qw( showcursor sc )],
		default    => "true",
	},
	cursorPos => {
		type       => "double",
		pause      => [qw( drawerMain dmv )],
		call       => [qw( drawerReset )],
		aliases    => [qw( cursorposition cursorpos cp )],
		default    => 50,
		validCond  => "(arg0 >= 0) && (arg0 <= 100)",
		unit       => "%",
	},
	forceCursorPos => {
		type       => "bool",
		pause      => [qw( drawerMain )],
		call       => [qw( drawerReset )],
		aliases    => [qw( forcecursorposition forcecursorpos fcp )],
		default    => "false",
	},
	reverseDirection => {
		type       => "bool",
		aliases    => [qw( reversedirection rd )],
		pause      => [qw( drawerMain )],
		call       => [qw( drawerReset )],
	},
	swapAxes => {
		type       => "bool",
		aliases    => [qw( swapaxes sa )],
		pause      => [qw( drawerMain )],
		call       => [qw( drawerReset )],
	},
	filterOvertones => {
		type       => "bool",
		pause      => [qw( drawerMain dmv )],
		call       => [qw( dmvReset dsResetToneScale )],
		aliases    => [qw( filterovertones fo )],
		default    => "false",
	},
	overtoneBlur    => {  # XXX  add boundaries
		type       => "int",
		call       => [qw( dmvReset dsResetToneScale )],
		default    => 2,
		unit       => "px",
		pause      => [qw( drawerMain dmv )],
		aliases    => [qw( overtoneblur ob )],
	},
	overtoneThreshold => { # XXX
		type       => "float",
		call       => [qw( dmvReset dsResetToneScale )],
		default    => 900,
		unit       => "%",
		pause      => [qw( drawerMain dmv )],
		aliases    => [qw( overtonethreshold ot )],
	},
	overtoneRatio => { # XXX
		type       => "float",
		call       => [qw( dmvReset dsResetToneScale )],
		default    => 100,
		unit       => "%",
		pause      => [qw( drawerMain dmv )],
		aliases    => [qw( overtoneratio or )],
	},
	overtoneAddition => { # XXX
		type       => "float",
		call       => [qw( dmvReset dsResetToneScale )],
		default    => 0,
		unit       => "%",
		pause      => [qw( drawerMain dmv )],
		aliases    => [qw( overtoneaddition oa )],
	},
	gain => {
		type       => "double",
		pause      => [qw( drawerMain dmv )],
		call       => [qw( dmvReset dsResetToneScale )],
		aliases    => [qw( gain g )],
		unit       => "dB",
	},
	dynamicGain => {
		type       => "bool",
		pause      => [qw( drawerMain dmv )],
		call       => [qw( dmvReset dsResetToneScale )],
		aliases    => [qw( dynamicgain dg )],
	},
	signalToNoise => {
		type       => "double",
		pause      => [qw( drawerMain dmv )],
		call       => [qw( dmvReset dsResetToneScale )],
		aliases    => [qw( signaltonoise stn )],
		unit       => "dB",
		validCond  => "arg0 > 0",
		default    => 40,
	},
}
