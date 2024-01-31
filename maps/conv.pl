#!/usr/bin/env perl
use strict;
use warnings;
use autodie;

my $buf;

open my $fp, '<:raw', $ARGV[0];
open my $fpo, '>:raw', $ARGV[0] . '.lvl';

# map size
print $fpo chr(100);
print $fpo chr(100);
# map tiles
for (my $y = 0; $y < 100; $y ++) {
  for (my $x = 0; $x < 50; $x ++) {
	read $fp, $buf, 1;
	$buf = ord($buf);
    print $fpo chr($buf >> 4);
    print $fpo chr($buf & 0x0F);
  }
}
# hq locations, capture points
for (my $i = 0; $i < 5; $i ++) {
  read $fp, $buf, 3;
  my @c = unpack 'C3', $buf;
  
  print $fpo chr($c[1]);
  print $fpo chr($c[2]);
}

# additional building count
print $fpo chr(10);
for (my $i = 5; $i < 15; $i ++) {
  read $fp, $buf, 3;

  my @c = unpack 'C3', $buf;
  
  print $fpo chr($c[0] - ($c[0] > 7 ? 2 : 0));
  print $fpo chr($c[1]);
  print $fpo chr($c[2]);
}

close $fpo;
close $fp;