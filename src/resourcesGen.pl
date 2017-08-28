#!/usr/bin/perl
# MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

# This script generates src/resources.gen.h and src/resources.gen.c from files given as arguments

use v5.14;
say "Generating resources...";

open my $cOut, ">", "src/resources.gen.c" or die "Cannot write to resources.gen.c.";
open my $hOut, ">", "src/resources.gen.h" or die "Cannot write to resources.gen.h.";

while ($_ = shift @ARGV) {
	open my $file, "<", $_ or die "Cannot open file.";
	my $name = s=^.*[\\/]==gr;
	$name =~ s=[-./\\]=_=g;
	printf $cOut "const char resources_$name"."[] = \"";
	my $size = 0;
	while (read $file, $_, 1) {
		printf $cOut "\\x%x", ord;
		$size++;
	}
	printf $cOut "\";\n";
	printf $hOut "extern const char resources_$name"."[$size];\n"; # XXX maybe $size+1 ?
	close $file;
}

close $cOut;
close $hOut;
