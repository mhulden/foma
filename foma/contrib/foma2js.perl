#!/usr/bin/perl

###################################################################
# Converts foma file to js array for use with Javascript runtime  # 
# Outputs a js array of all the transitions, indexed in the       #
# input direction. This array can be passed to the js function    #
# foma_apply_down() in foma_apply_down.js for stand-alone         #
# transducer application.                                         #
#                                                                 #
# Usage: foma2js [-n array variable name] [file]                  # 
# MH 20120127                                                     #
###################################################################

use Switch;
use utf8;
use Compress::Zlib;

my $buffer ; my $filein ; my $file; my $jsnetname = 'myNet';

die "Usage: fomatojs [-n name] filename" if $#ARGV < 0;
foreach (my $argnum = 0 ; $argnum <= $#ARGV; $argnum++) {
    if ($ARGV[$argnum] =~ '^-h$') {
	print "Usage: foma2js [-n array variable name] [file]\n";
	exit;
    }
    if ($ARGV[$argnum] =~ '^-n$') {
	if ($argnum+1 <= $#ARGV) {
	    $jsnetname = $ARGV[$argnum+1];
	} else {
	    die;
	}
    } else {
	$file = $ARGV[$argnum];
    }
}

my $gz = gzopen($file, "rb")
    or die "Cannot open $file: $gzerrno\n" ;
while ($gz->gzread($buffer) > 0) {
    $filein .= $buffer;
}

die "Error reading from $file: $gzerrno" . ($gzerrno+0) . "\n"
    if $gzerrno != Z_STREAM_END ;
$gz->gzclose();

my @lines = split '\n', $filein;

my $mode = "none";
my $version; my $numnets = 0; my %pr;
my $line = 0;
my @sigma;
my $longestsymbollength = 0;
foreach (@lines) {
    chomp;
    if ($_ =~ /##foma-net ([0-9]+\.[0-9]+)##/) {
	$version = $1;
	$numnets++;
	if ($numnets > 1) {
	    die "Only one network per file supported"
	}
	next;
    }
    if ($_ =~ /##props##/) {
	$mode = "props";
	next;
    }
    if ($_ =~ /##sigma##/) {
	$mode = "sigma";
	next;
    }
    if ($_ =~ /##states##/) {
	$mode = "states";
	next;
    }
    if ($_ =~ /##end##/) {
	$mode = "none";
	next;
    }
    switch($mode) {
	case "props" {
	    ($pr{"arity"}, $pr{"arccount"},$pr{"statecount"},$pr{"linecount"}, $pr{"finalcount"},$pr{"pathcount"},$pr{"is_deterministic"},$pr{"is_pruned"},$pr{"is_minimized"},$pr{"is_epsilon_free"},$pr{"is_loop_free"},$pr{"extras"},$pr{"name"}) = split ' ';
	}
	case "states" {
	    #state in out target final
	    @transitions = split ' ';
	    if ($transitions[0] == -1) { next; }
	    if ($transitions[1] == -1 && $#transitions == 3) {
		$arrstate = $transitions[0];		
		$arrfinal = $transitions[3];
		if ($arrfinal == 1) {
		    $finals[$arrstate] = 1;
		}
		next;
	    }
	    if ($#transitions == 4) {
		$arrstate = $transitions[0];
		$arrin = $transitions[1];
		$arrout = $transitions[2];
		$arrtarget = $transitions[3];
		$arrfinal = $transitions[4];
		if ($arrfinal == 1) {
		    $finals[$arrstate] = 1;
		}
	    }
	    elsif ($#transitions == 3) {
		$arrstate = $transitions[0];
		$arrin = $transitions[1];
		$arrtarget = $transitions[2];
		$arrfinal = $transitions[3];
		$arrout = $arrin;
		if ($arrfinal == 1) {
		    $finals[$arrstate] = 1;
		}
	    }
	    elsif ($#transitions == 2) {
		$arrin = $transitions[0];
		$arrout = $transitions[1];
		$arrtarget = $transitions[2];
	    }
	    elsif ($#transitions == 1) {
		$arrin = $transitions[0];
		$arrtarget = $transitions[1];
		$arrout = $arrin;
	    }	   
	    push(@{$trans{$arrstate ."|" .$sigma[$arrin]}}, "\{$arrtarget:\'$sigma[$arrout]\'\}");
	}
	case "sigma" {
	    (my $number, my $symbol) = split ' ';
	    $symbol =~ s/^\@_EPSILON_SYMBOL_\@$//g;
	    $symbol =~ s/^\@_IDENTITY_SYMBOL_\@$/\@ID\@/g;
	    $symbol =~ s/^\@_UNKNOWN_SYMBOL_\@$/\@UN\@/g;
	    $symbol =~ s/'/\\'/g;
	    $sigma[$number] = $symbol;
	    if ($number > 2) {
		utf8::decode($symbol);	  
		if (length($symbol) > $longestsymbollength) {
		    $longestsymbollength = length($symbol);
		}
		
	    }
	}
	case "none" {
	    die "Format error";
	}
    }
}

print "var $jsnetname = new Object;\n";
print "$jsnetname.t = Array;\n";
print "$jsnetname.f = Array;\n";
print "$jsnetname.s = Array;\n\n";

foreach $k (keys %trans) {
    ($state, $in) = split /\|/, $k;
    $in =~ s/^\@UN\@$/\@ID\@/;
    print "$jsnetname.t\[$state + '|' + \'$in\'\] = \[";
    print join (',', @{$trans{$k}}) ."\];\n";
}

for ($i = 0; $i <= $pr{'statecount'}; $i++) {
    if (defined $finals[$i] and $finals[$i]) {
	print "$jsnetname.f\[$i\] = 1;\n";
    }
}

for ($i = 3 ; $i <= $#sigma; $i++) {
    if (defined $sigma[$i]) {
	print "$jsnetname.s\['$sigma[$i]'\] = $i;\n";
    }
}

print "$jsnetname.maxlen = $longestsymbollength ;\n";
