#!/usr/bin/perl
# MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

# This script generates src/messagesList.gen.h and src/messagesList.gen.c from src/messagesList.pl.

use v5.14;
say "Generating messages list...";



# === LOAD MESSAGES' AND OPTIONS' DESCRIPTION ===


my %messagesList = do './src/messagesList.pl';
my @modules  = @{$messagesList{modules}};
my %messages = %{$messagesList{messages}};
my %options  = %{$messagesList{options}};




# === UTILITIES TO BE USED MAINLY BY PRIMITIVES ===


# ---------- formatting and conversions ----------

my %cTypes = (
	bool   => "bool",
	int    => "int",
	float  => "float",
	double => "double",
	string => "char *",
	path   => "char *");
my %cTypeConsts;
$cTypeConsts{$_} = "MSG_".uc for keys %cTypes;
my %cPrintfConsts = (
	bool   => "%u",
	int    => "%d",
	float  => "%g",
	double => "%lg",
	string => "%s",
	path   => "%s");

our (@args, $argI, $argCType, $argCTypeConst);

sub formatArgs(&;$) {
	local $argI = 0;
	my ($format,$delim) = @_;
	$delim //= "";
	return join $delim, map {
		local $argCType = $cTypes{$_};
		local $argCTypeConst = $cTypeConsts{$_};
		my $ret=&$format;
		$argI++;
		$ret;
	} @args;
}


sub initializer {   # (type, components...)   -->  "(type){comp1, comp2, ...}"
	my $type = shift;
	$type = "($type)" if $type;
	return "$type\{" . join(", ", @_) . "}";
}

sub quote($) {
	my $str = shift;
	#return qq("$str");
	return '"'. ($str =~ s=(["])=\\$1=rg) .'"';
}


sub getMsgTypeDName    {shift =~ s/([^_]*)(_.*)?/$1D$2/r}
sub getMsgTypeArgsName {shift =~ s/([^_]*)(_.*)?/$1Args$2/r}




# ---------- opening and closing blocks ----------

my $lp = "";     # line prefix
my @blockDelims; # (noBlock, open, close)  when  block can be open
                 # (close)                 when  block is open
                 # ()                      when  block cannot be open
sub openBlock {
	die "No block can be open" if not @blockDelims;
	if (@blockDelims >= 2) {
		shift @blockDelims;
		say shift @blockDelims;
		$lp = "	";
	}
}
sub closeBlock {
	say shift @blockDelims if @blockDelims;
	@blockDelims = ();
	$lp = "";
}




# === PRIMITIVES FOR DEFINING GENERATED FILES' STRUCTURE ===


# ---------- top level primitives ----------


sub blankLine {  # should be contained at the end to close opened block
	closeBlock;
	say;
}

sub include {
	closeBlock;
	say qq(#include "$_.h") for @_;
}

# following functions can generate either declaration or definition statements

sub msgType {  # (msgTypeName) OR (msgTypeName, [args], [pause], callbackName, msgOptionName)
	closeBlock;
	my ($msgTypeName, $args, $pauseTasks, $callbackName, $msgOptionName) = @_;
	if (!$callbackName) {  # declaration

		say "struct msgType *$msgTypeName;";

	} else {               # definition

		my $msgTypeDName    = getMsgTypeDName    $msgTypeName;
		my $msgTypeArgsName = getMsgTypeArgsName $msgTypeName;
		@args = @$args;
		say "static enum msgDataType $msgTypeArgsName"."[] = ".
			initializer("", formatArgs {"$argCTypeConst, "} . "MSG_VOID") . ";";
		say "static struct msgType $msgTypeDName = ". initializer("",
			$msgTypeArgsName,
			initializer("struct taskInfo *[]", map({"&$_"} @$pauseTasks), "NULL"),
			$callbackName,
			($msgOptionName ? "&$msgOptionName" : "NULL")),";";
		say "struct msgType *$msgTypeName = &$msgTypeDName;";

	}
}

sub msgOption {  # (type, name) OR (type, name, value)
	closeBlock;
	my ($type, $name, $value) = @_;
	if (!defined $value) {
		say "extern $cTypes{$type} $name;";
	} else {
		say "$cTypes{$type} $name = $value;";
	}
}


sub msgAliases {  # followed by 'aliases' calls serves as definition
	closeBlock;
	print "struct msgAlias *msgAliases";
	@blockDelims = (";", " = (struct msgAlias[]){", "	{NULL, NULL}\n};");
}


sub func {  # (name_with_modifiers, args...); definition if func. statements follows, sets @args
	closeBlock;
	my $name_with_modifiers = shift;
	@args = @_;
	print "$name_with_modifiers(" . (formatArgs {"$argCType arg$argI"} "," or "void") . ")";
	@blockDelims = (";", " {", "}");
}

sub packedFunc {  # (name_with_modifiers, args...)
	closeBlock;
	my $name_with_modifiers = shift;
	@args = @_;
	print "$name_with_modifiers(void *packedData)";
	@blockDelims = (";", " {", "}");
}




# ---------- function statements ----------

# these functions should follow 'func' or 'packedFunc' call,
# and can use its @args

sub msgSend {  # (typeName)
	openBlock;
	my $typeName = shift;
	#say "	msgSend($typeName, (void*[]){".(formatArgs {"&arg$argI"} ", ")."});";
	say "${lp}msgSend($typeName, ". initializer("void*[]", formatArgs {"&arg$argI"} ", ") .");";
}

sub unpackArgs {
	openBlock;
	if (@args) {
		say "${lp}". formatArgs {"$argCType arg$argI;"} " ";
		say "${lp}msgUnpackArgs((enum msgDataType[]){". (formatArgs {$argCTypeConst} ", ") .", MSG_VOID}, ".
			"(void *[]){" . (formatArgs {"&arg$argI"} ", ") . "}, packedData);";
	}
}

sub condIf($) {
	openBlock;
	my $cond = shift;
	say "${lp}if ($cond) {";
	$lp .= "	";
}
sub condElse() {
	$lp = substr $lp, 0, -1;
	say "${lp}} else {";
	$lp .= "	";
}
sub condEnd() {
	$lp = substr $lp, 0, -1;
	say "${lp}};";
}

sub assign {  # (type, lvalue, rvalue)
	openBlock;
	my ($type, $lvalue, $rvalue) = @_;
	if ($cTypes{$type} eq  "char *") {
		say "${lp}utilStrRealloc(&$lvalue, NULL, strlen($rvalue)+1);";
		say "${lp}strcpy($lvalue, $rvalue);";
	} else {
		say "${lp}$lvalue = $rvalue;";
	}
}

sub call {  # (funcName) OR (funcName, args...) to override @args
	openBlock;
	my $name = shift;
	my $args;
	if (@_) {
		$args = join ", ", @{$_[0]};
	} else {
		$args = formatArgs {"arg$argI"} ", ";
	}
	say "${lp}$name($args);";
}

sub callFormatted {  # (funcName, argFormatStr, varTypes..., vars...); types only for %* in format
	openBlock;
	my $name   = shift;
	my $format = shift;
	if (!@_) {
		say "${lp}$name(", quote $format, ");";
	} else {
		$format =~ s/%\*/$cPrintfConsts{(shift)}/ while $format =~ /%\*/;
		say "${lp}{";
		say "${lp}	char *str = NULL;";
		say "${lp}	asprintf(&str, ". join(", ", quote $format, @_) .");";
		say "${lp}	$name(str);";
		say "${lp}	free(str);";
		say "${lp}}";
	}
}

sub callPrint {  # (formatStr, varTypes..., vars...); like printf except %* with type in arg
	callFormatted "msgSend_print", @_;
}
sub callPrintErr {  # (formatStr, varTypes..., vars...)
	callFormatted "msgSend_printErr", @_;
}

# ---------- aliases ----------

# this function should follow 'msgAliases' call

sub aliases {  # (msgTypeName, preferredAlias, otherAliases...)
	openBlock;
	my $msgTypeName     = shift;
	my $msgTypeDName    = getMsgTypeDName    $msgTypeName;
	my $msgTypeArgsName = getMsgTypeArgsName $msgTypeName;
	my $preferred = "true";
	for (@_) {
		say "${lp}". initializer("",
			quote $_,
			$preferred,
			"&$msgTypeDName",
			$msgTypeArgsName) .",";
		$preferred = "false";
	}
}





# === GENERATING FILES ===


# ---------- messagesInit.gen.h ----------

open my $hFile, ">", "src/messagesList.gen.h" or die "Cannot write.";
select $hFile;

say "// This file was generated by messagesGen.pl from messagesList.pl.";

while (my ($name,$msg) = each %messages) {
	blankLine;

	msgType "msgType_$name";
	func "inline void msgSend_$name", @{$msg->{args}};
		msgSend "msgType_$name";
}

while (my ($name,$option) = each %options) {
	blankLine;

	msgOption $option->{type}, "msgOption_$name";

	msgType "msgTypeSet_$name";
	func "inline void msgSet_$name", $option->{type};
		msgSend "msgTypeSet_$name";

	msgType "msgTypeSetPrint_$name";
	func "inline void msgSetPrint_$name";
		msgSend "msgTypeSetPrint_$name";

	if ($option->{type} eq "bool") {

		msgType "msgTypeSetTrue_$name";
		func "inline void msgSetTrue_$name";
			msgSend "msgTypeSetTrue_$name";

		msgType "msgTypeSetFalse_$name";
		func "inline void msgSetFalse_$name";
			msgSend "msgTypeSetFalse_$name";

	}
}

msgType "msgTypeSetPrint";
func "inline void msgSetPrint";
	msgSend "msgTypeSetPrint";

blankLine;

msgAliases;

blankLine;

close $hFile;




# ---------- messagesInit.gen.c ----------

open my $cFile, ">", "src/messagesList.gen.c" or die "Cannot write.";
select $cFile;

say "// This file was generated by messagesGen.pl from messagesList.pl.";

include @modules;

while (my ($name,$msg) = each %messages) {
	blankLine;

	func "void msgSend_$name", @{$msg->{args}};
	packedFunc "static void callback_$name", @{$msg->{args}};
		unpackArgs;
		call $_ for @{$msg->{call}//[]};
	
	msgType "msgType_$name", $msg->{args}, $msg->{pause}, "callback_$name";
}

while (my ($name,$option) = each %options) {
	blankLine;

	msgOption $option->{type}, "msgOption_$name", 0;

	func "void msgSet_$name", $option->{type};
	packedFunc "static void callbackSet_$name", $option->{type};
		unpackArgs;
		condIf $option->{validCond} if $option->{validCond};
			assign $option->{type}, "msgOption_$name", "arg0";
			call $_, [] for @{$option->{call}//[]};
		if ($option->{validCond}) {
			condElse;
				callPrintErr $option->{invalidStr} // "Wrong value.";
			condEnd;
		}

	msgType "msgTypeSet_$name", [$option->{type}], $option->{pause}, "callbackSet_$name", "msgOption_$name";

	func "void msgSetPrint_$name";
	packedFunc "static void callbackSetPrint_$name";
		if ($option->{type} eq "bool") {
			callPrint "%2s$option->{aliases}[0]" . ($option->{unit} ? " $option->{unit}" : ""), qq((msgOption_$name ? "" : "no"));
		} else {
			callPrint "  $option->{aliases}[0]=%*" . ($option->{unit} ? " $option->{unit}" : ""), $option->{type}, "msgOption_$name";
		}
	msgType "msgTypeSetPrint_$name", [], [], "callbackSetPrint_$name";

	if ($option->{type} eq "bool") {
		func "void msgSetTrue_$name";
		packedFunc "static void callbackSetTrue_$name";
			condIf $option->{validCond} if $option->{validCond};
				assign "bool", "msgOption_$name", "true";
				call $_ for @{$option->{call}//[]};
			if ($option->{validCond}) {
				condElse;
					callPrintErr $option->{invalidStr} // "Wrong value.";
				condEnd;
			}
		
		msgType "msgTypeSetTrue_$name", [], $option->{pause}, "callbackSetTrue_$name", "msgOption_$name";

		func "void msgSetFalse_$name";
		packedFunc "static void callbackSetFalse_$name";
			condIf $option->{validCond} if $option->{validCond};
				assign "bool", "msgOption_$name", "false";
				call $_ for @{$option->{call}//[]};
			if ($option->{validCond}) {
				condElse;
					callPrintErr $option->{invalidStr} // "Wrong value.";
				condEnd;
			}
		
		msgType "msgTypeSetFalse_$name", [], $option->{pause}, "callbackSetFalse_$name", "msgOption_$name";
	}
}

{
	blankLine;
	func "void msgSetPrint";
	packedFunc "static void callbackSetPrint";
		my @printableOptions =
			sort {$options{$a}->{aliases}[0] cmp $options{$b}->{aliases}[0]}
			grep {$options{$_}->{aliases}}
			keys %options;
		my (@format, @types, @vars);
		for (@printableOptions) {
			my ($name,$option) = ($_, $options{$_});
				#while (my ($name,$option) = each %options) {
			next if not $option->{aliases} or not @{$option->{aliases}};
			if ($option->{type} eq "bool") {
				push @format, "%2s$option->{aliases}[0]" . ($option->{unit} ? " $option->{unit}" : "");
				push @vars,   qq((msgOption_$name ? "" : "no"));
			} else {
				push @format, "  $option->{aliases}[0]=%*" . ($option->{unit} ? " $option->{unit}" : "");
				push @types,  $option->{type};
				push @vars,   "msgOption_$name";
			}
		}
		callPrint join('\n', @format), @types, @vars;
	msgType "msgTypeSetPrint", [], [], "callbackSetPrint";
}

blankLine;
func "static __attribute__((constructor)) void initGen";
	while (my ($name,$option) = each %options) {
		if ($option->{default}) {
			my $value = $option->{default};
			$value = quote $value if $cTypes{$option->{type}} eq "char *";
			assign $option->{type}, "msgOption_$name", $value;
		}
	}
	while (my ($name,$option) = each %options) {
		call "msgSet_$name", ["msgOption_$name"] if $option->{initCall};
	}

blankLine;

msgAliases;
while (my ($name,$msg) = each %messages) {
	my $delim = "";
	$delim = " " if @{$msg->{args}};
	aliases "msgType_$name", map {"$_$delim"} @{$msg->{aliases}};
}
while (my ($name,$option) = each %options) {
	if ($option->{type} eq "bool") {
		aliases "msgTypeSetTrue_$name",  map {"set $_"}   @{$option->{aliases}};
		aliases "msgTypeSetFalse_$name", map {"set no$_"} @{$option->{aliases}};
		aliases "msgTypeSetPrint_$name", map {"set $_?"}  @{$option->{aliases}};
	} else {
		aliases "msgTypeSet_$name",      map {"set $_="}  @{$option->{aliases}};
		aliases "msgTypeSetPrint_$name", map {"set $_"}   @{$option->{aliases}};
		aliases "msgTypeSetPrint_$name", map {"set $_?"}  @{$option->{aliases}};
	}
}
aliases "msgTypeSetPrint", "set";

blankLine;

close $cFile;
