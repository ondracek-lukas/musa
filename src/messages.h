// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

// messages module allows asynchronous sending of messages to other modules
// and manages global variables (options);
// it is thread-safe and lock-free.

// Definitions of options and types of the messages are in messagesList.pl,
// which contains more detailed description of the interface.

// Sending a message works this way:
//    1) Any module calls msgSend_NAME(arg0, arg1, ...),
//       it returns immediately (all arguments including strings are copied).
//    2) Appropriate tasks of taskManager module are suspended.
//    3) Registered functions are called with the same arguments as original call.
//    4) The tasks are resumed.

// Global variables can be read directly, writing is a special type of message.

// The module also provides a little capability of introspection,
// which can be used by console module to directly send the messages.



#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdbool.h>

// The type of the message,
// it specifies what functions should be called with what types of arguments, etc.
struct msgType;

// Sending a message using msgType and array of ptrs to args,
// messagesList.pl provides more comfortable interface.
void msgSend(struct msgType *type, void **args);

// Allowed data types of the arguments and options.
enum msgDataType {
	MSG_VOID = 0,  // used for NULL-termination
	MSG_INT,
	MSG_BOOL,
	MSG_FLOAT,
	MSG_DOUBLE,
	MSG_STRING,    // char *
	MSG_PATH       // char *, console can handle it differently to string
};

// String alias used for introspection,
// messagesList.pl defines NULL-terminated array msgAliases.
struct msgAlias {
	char *name;             // beginning of console command
	bool preferred;         // alias preferred to other aliases of the same msgType
	struct msgType *msg;    // type of the alias to be called
	enum msgDataType *args; // NULL-terminated array of data types of the arguments
};

extern struct taskInfo msgTask;

// Packing arguments (including strings) to newly created type-less array.
// argsArr is array of ptrs to variables of types specified by NULL-terminated types
void *msgPackArgs(enum msgDataType *types, void **argsArr);
void msgUnpackArgs(enum msgDataType *types, void **argsArr, void *packedArgs);
void msgFreeArgs(void *packedArgs);


#include "messagesList.gen.h"

#endif
