#!/usr/bin/env perl
use strict;
use warnings;
use autodie;

use JSON::PP qw( decode_json );

my $json = do {
  open my $fh, '<:raw', $ARGV[0];
  local $/ = undef;
  my $data = <$fh>;
  decode_json($data);
};

use Data::Dumper;
print Dumper($json);