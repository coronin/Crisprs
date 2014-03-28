#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <array>
#include <stdexcept>
#include <ctime>
#include <climits>
#include <unistd.h>

#include "utils.h"
#include "crisprutil.h"

using namespace std;

/*
should add finding of all crisprs from genome into these options
maybe a method to find an id given a sequence?
*/
int usage() {
    fprintf(stderr, "\n");
    fprintf(stderr, "Program: find_off_targets\n");
    fprintf(stderr, "Contact: Alex Hodgkins <ah19@sanger.ac.uk>\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: find_off_targets <command> [options]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Command: index      Create binary index of all CRISPRs\n");
    fprintf(stderr, "         align      Find potential off targets for CRISPRs\n");
    fprintf(stderr, "\n\n");
    fprintf(stderr, "Example end to end usage:\n");
    fprintf(stderr, "Create index:\n");
    fprintf(stderr, "\tfind_off_targets index -i human_chr1-11.csv -i human_chr12_on.csv -o index.bin\n");
    fprintf(stderr, "Calculate off targets for single CRISPR:\n");
    fprintf(stderr, "\tfind_off_targets align 873245 > crispr_data.tsv\n");

    return 1;
}

int index_usage() {
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: find_off_targets index [options]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options: -o FILE    The file to put the binary index to\n");
    fprintf(stderr, "         -i FILE    Input file of CSV crispr data (multiple -i flags can be specified)\n");
    fprintf(stderr, "         -a TEXT    Assembly (e.g. GRCh37)\n");
    fprintf(stderr, "         -s TEXT    Species (e.g. Human)\n");
    fprintf(stderr, "         -e INT     Species ID - the species ID that your database uses");
    fprintf(stderr, "\n");
    fprintf(stderr, "Example usage:\n");
    fprintf(stderr, "find_off_targets index -i ~/human_chr1-11.csv -i ~/human_chr12_on.csv -o ~/index.bin");
    fprintf(stderr, " -s Human -a GRCh37 -e 1\n");
    "\n"
    fprintf(stderr, "Note: the input to this option is a csv file of CRISPR data"); 
    fprintf(stderr, " generated by get_all_crisprs.cpp\n");

    return 1;
}

int index(int argc, char * argv[]) {
    int c = -1;

    vector<string> infiles;
    uint8_t species_id = CHAR_MAX;
    string outfile = "", assembly = "", species = "";

    while ( (c = getopt(argc, argv, "i:o:a:s:e:")) != -1 ) {
        switch ( c ) {
            case 'i': infiles.push_back( optarg ); break;
            case 'o': outfile = optarg; break;
            case 'a': assembly = optarg; break;
            case 's': species = optarg; break;
            case 'e': species_id = stoi(optarg); break;
            case '?': return index_usage();
        }
    }

    //validate the so called options
    if ( ! infiles.size() ) {
        cerr << "Please provide an infile with the -i option\n";
        return index_usage();
    }

    if ( outfile == "" ) {
        cerr << "Please provide an output file\n";
        return index_usage();
    }

    if ( assembly == "" || species == "" ) {
        cerr << "Please provide a species (-s) and an assembly (-a)\n";
        return index_usage();
    }

    if ( species_id == CHAR_MAX ) {
        cerr << "Please provide a species_id (-e)\n";
        return index_usage();
    }

    cerr << "Outfile:\n\t" << outfile << "\n";
    cerr << "Infiles:\n";
    for ( auto i = infiles.begin(); i < infiles.end(); i++ ) {
        cerr << "\t" << *i << "\n";
    }

    //resize to be the same length as our metadata char array
    //it has to be -1 to allow for the \0
    assembly.resize( MAX_CHAR_SIZE-1 );
    species.resize( MAX_CHAR_SIZE-1 );

    //initialize metadata and send a pointer to the method
    metadata_t data;
    strcpy( data.assembly, assembly.c_str() );
    strcpy( data.species, species.c_str() );
    data.species_id = species_id;
    data.num_seqs = 0;
    data.seq_length = 20;

    CrisprUtil finder = CrisprUtil();
    finder.text_to_binary( infiles, outfile, &data );

    return 0;
}

int align_usage() {
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: find_off_targets align [options] <ids>\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options: -i FILE    The file containing the CRISPR index, (from the index step)\n");
    fprintf(stderr, "         -s int     The CRISPR range start\n");
    fprintf(stderr, "         -n int     How many CRISPRs to compute after start)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Note:\n");
    fprintf(stderr, "You can specify a list of ids, OR specify a range of CRISPRs. ");
    fprintf(stderr, "For example:\n");
    fprintf(stderr, "./find_off_targets align -s 43275 -n 1000\n");
    fprintf(stderr, "  This will calculate off targets for 1000 crisprs,\n");
    fprintf(stderr, "  the ids of which will be 43275-44725\n");
    fprintf(stderr, "\n\n");
    fprintf(stderr, "find_off_targets align 873245 923577 237587 109583\n");
    fprintf(stderr, "  This will calculate off targets for the given 4 CRISPRs\n");

    return 1;
}

int align(int argc, char * argv[]) {
    int c = -1;

    uint64_t start = 0, num = 0;
    vector<uint64_t> ids;
    string index = "";
    bool range_mode = 0;

    while ( (c = getopt(argc, argv, "s:n:i:")) != -1 ) {
        switch ( c ) {
            case 's': start = atol( optarg ); break;
            case 'n': num = atol( optarg ); break;
            case 'i': index = optarg; break;
            case '?': return align_usage();
        }
    }

    if ( index == "" ) {
        cerr << "An index file must be specified with the -i option\n";
        return align_usage();
    }

    //range mode takes precedence over additional ids
    if ( start != 0 && num != 0 ) {
        cerr << "Will take " << num << " ids, starting from " << start << "\n";
        range_mode = 1;
        //c.find_off_targets( start, num );
    }
    else if ( optind < argc ) { //see if we got 1 or more ids at the end
        cerr << "Provided ids:";
        for ( int i = optind; i < argc; i++ ) {
            uint64_t id = atol(argv[i]);
            cerr << " " << id;
            ids.push_back( id );
        }
        cerr << "\n";
    }
    else {
        return align_usage();
    }

    CrisprUtil finder = CrisprUtil();
    finder.load_binary( index );

    if ( range_mode ) {
        finder.find_off_targets( start, num );
    }
    else {
        finder.find_off_targets( ids );
    }

    return 0;
}

int main(int argc, char * argv[]) {
    //at least 1 option is required
    if ( argc < 2 )
        return usage();

    if ( strcmp(argv[1], "index") == 0 ) 
        return index( argc-1, argv+1 );
    else if ( strcmp(argv[1], "align") == 0 )
        return align( argc-1, argv+1 );
    else {
        fprintf(stderr, "Unrecognised command: %s", argv[1] );
        return 1;
    }

    return 0;
}

/*
    Copyright (C) 2014 Genome Research Limited

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 
    Written by Alex Hodgkins (ah19@sanger.ac.uk) in 2014 
    Some code taken from scanham written by German Tischler
*/
