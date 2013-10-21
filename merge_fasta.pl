#!/usr/bin/perl

use strict;
use warnings;

use Bio::Perl;
use Try::Tiny;

die "Usage: merge_fasta.pl <bed_file> <seqs_file>" unless defined $ARGV[0] && defined $ARGV[1];

#take the args and open the files so we can process them side by side
my $bed_file = shift;
open( my $bed_fh, "<", $bed_file ) or die "Couldn't open $bed_file: $!";

my $seqs_file = shift;
open( my $seqs_fh, "<", $seqs_file ) or die "Couldn't open $seqs_file: $!";

#loop both files synchronously, as the lines should correspond
while ( my $bed_line = <$bed_fh> ) {
    my $seq_line = <$seqs_fh>;

    my $merged = merge_seqs( $bed_line, $seq_line );

    if ( $merged ) {
        print $merged;
    }
    else {
        #try once more, because sometimes bwa aligns off the end of the chromosome,
        #and bed2fasta skips that entry. If the next line matches then we just ignore.
        #<$bed_fh> will send the next line of the bed file
        my $next_bed_line = <$bed_fh>;
        $merged = merge_seqs( $next_bed_line, $seq_line );

        chomp $bed_line; chomp $seq_line;

        if ( $merged ) {
            print STDERR "WARNING: $seq_line didn't match $bed_line, but the next entry was fine.\n";
            print $merged;

            #if it worked then we'll just continue as normal
            next;
        }

        die "$seq_line doesn't match $bed_line!";
    }
}

sub merge_seqs {
    my ( $bed_line, $seq_line ) = @_;

    my ( $location, $seq ) = split /\s+/, $seq_line;
    #the location needs to be further split to match the bed file format 
    my ( $seq_chr, $seq_start, $seq_end ) = $location =~ /(.+):(\d+)-(\d+)/;

    #parse bed file into vars. unknown should be renamed to what it actually is
    my ( $chr, $start, $end, $name, $unknown, $strand ) = split /\s+/, $bed_line;

    #make sure the locations from each file are the same or everything would be wrong
    if ( $chr eq $seq_chr and $start == $seq_start and $end == $seq_end ) {
        $name .= "-$seq";

        return "$chr\t$start\t$end\t$name\t$unknown\t$strand\n";
    }
    #else {
    #    chomp $bed_line; chomp $seq_line;
    #    print STDERR "$bed_line vs $seq_line failed\n";
    #}

    #return undef if it didn't match.
    return;
}

1;

__END__

=head1 NAME

merge_fasta.pl

=head1 SYNOPSIS

merge_fasta.pl [bed_file] [seqs_file] > merged.bed

Where seqs_file is the product of: 
fastaFromBed -tab -fi GRCm38_68.fa -bed output.bed -fo output.tsv

Example usage based on the previous fastaFromBed command:
merge_fasta.pl output.bed output.tsv > output_merged.bed

=head1 DESCRIPTION

Merges a tsv of sequence data/locations (created by fastaFromBed) and a bed file into a single bed file. 
The sequences are stored in the name column in the bed file like: Name-SEQ
e.g. AR1-CTGTGCTGATGCTCGATCGAGG

We do this so we can easily check the validity of paired crispr sites.

=head AUTHOR

Alex Hodgkins

=cut