#!/usr/bin/perl
#
# This script finds all non-system shared objects that are used
# my an executable.  For example
# % ldd test
#	linux-gate.so.1 =>  (0x0092d000)
#	mylib.so => ./mylib.so (0x00e6b000)
#	libc.so.6 => /lib/tls/i686/cmov/libc.so.6 (0x0074a000)
#	/lib/ld-linux.so.2 (0x0059d000)
# The only non-system so needed is ./mylib.so


use strict;

my $exec = shift @ARGV;
if ($exec eq "")
{
  print "Usage: findso.pl <execname>\n";
  exit(1);
}

open PIPE, "ldd $exec |";
while (my $line = <PIPE>)
{
  chop $line;
  $line =~ s/\(.*$//; # remove hex address
  $line =~ s/ //g;   # remove whitespace
  my ($sym, $file) = split "=>", $line;
  # print if non-empty and (doesn't start with "/" or has peasoup or workspace in name)
  if ($file ne "" && $file !~ /notfound/ && (substr($file,0,1) ne "/" || $file =~ /peasoup/ || $file =~ /stonesoup/))
  {
    printf "$file\n";
  }
}
close PIPE;

