# MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

# This file defines options and types of messages,
# it will be processed by messagesGen.pl.



# === SYNTAX OF THIS FILE ===

# The file will be executed by Perl 5 interpreter and should consist of one hash
# with 'modules', 'messages', and 'options' subhashes.

# Message has these attributes:
# 	args:       Data types of arguments. (default: no arguments)
# 	call:       Functions to be called with arguments of given types.
# 	pause:      Tasks to be suspended before calling the functions.
# 	aliases:    Commands to send the message from console, the first one is preferred.

# Option has these attributes:
# 	type:       Data type of the variable.
# 	default:    Default value of the variable. (default: 0/NULL)
# 	call:       Function to be called with no arguments after changing the value.
# 	pause:      Tasks to be suspended before value change and functions calls.
# 	initCall:   Set to 1 to call the functions even for the default value.
# 	aliases:    Names in the console, the first one is preferred.
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

# Aliases can be accessed via NULL-terminated array
# 	struct msgAlias msgAliases[];



# === CONSOLE INTERFACE ===

# To send message with ALIAS and 3 arguments use:
# 	:ALIAS arg1 arg2 arg3

# To read all aliased values use:
# 	:set
# To read value of option with ALIAS use:
# 	:set ALIAS                  (for non-bool only)
# 	:set ALIAS?
# To change the value use:
# 	:set ALIAS=newValue         (for non-bool only)
# 	:set ALIAS / :set noALIAS   (for bool)



# === DEFINITION OF OPTIONS AND TYPES ===

modules => [qw( main drawer drawerMusicVisualiser logFft consoleOut )],

messages => {
	print => {
		args    => [qw( string )],
		pause   => [qw( drawerConsoleTask )],
		call    => [qw( consolePrint )],
	},
	printErr => {
		args    => [qw( string )],
		pause   => [qw( drawerConsoleTask )],
		call    => [qw( consolePrintErr )],
	},
	printStatus => {
		args    => [qw( string )],
		pause   => [qw( drawerConsoleTask )],
		call    => [qw( consolePrintStatus )],
	},
	consoleClear => {
		pause   => [qw( drawerConsoleTask )],
		call    => [qw( consoleClear )],
	},
	quit => {
		aliases => [qw( quit q )],
		call => [qw( mainExit )],
	},
},

options => {
	columnLen => {
		type       => "int",
		pause      => [qw( drawerMainTask dmvTask )],
		call       => [qw( dmvReset )],
	},
	visibleBefore => {
		type       => "int",
		pause      => [qw( drawerMainTask dmvTask )],
		call       => [qw( dmvReset )],
	},
	visibleAfter => {
		type       => "int",
		pause      => [qw( drawerMainTask dmvTask )],
		call       => [qw( dmvReset )],
	},
	a1Freq => {
		type       => "double",
		pause      => [qw( drawerMainTask dmvTask )],
		call       => [qw( dmvReset )],
		aliases    => [qw( a1freq )],
		default    => 440,
		validCond  => "arg0 > 0",
		unit       => "Hz",
	},
	minFreq => {
		type       => "double",
		pause      => [qw( drawerMainTask dmvTask )],
		call       => [qw( dmvReset )],
		aliases    => [qw( minfreq )],
		default    => 16,
		validCond  => "arg0 > 0",
		unit       => "Hz",
	},
	maxFreq => {
		type       => "double",
		pause      => [qw( drawerMainTask dmvTask )],
		call       => [qw( dmvReset )],
		aliases    => [qw( maxfreq )],
		default    => 22050,
		validCond  => "arg0 > 0",
		unit       => "Hz",
	},
}
