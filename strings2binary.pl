#!/usr/bin/perl
##########################Start Description and Purpose#########################
#
#Created by Micah Bly
#   on: 2020/03/29 (f256jr version 2022/12/07)
#Name: strings2binary.pl
#
#Purpose: convert files with 1 string per row into binary files. 
#         will parse each file individually
#         will eliminate white space, comment lines, braces, etc. 
#		  usage case is managing strings in a text file, and preparing them
#         for use in an 8 bit program. initial use case: C128 minirogue
#         Assumes and requires following format (per line!)
#         id TAB stringlen TAB string EOL
#         id must be 0-255. stringlen must be 0-254 (to allow for null)
#         strings themselves cannot be longer than 254 characters.
#         EOL does not count against that. 
#         Lines that start with # are comments, and will be ignored.
#
#Usage: "perl strings2binary.pl [sourcedirectory]"
#Usage example: "perl strings2binary.pl strings" 
#
##########################End Description and Purpose###########################


#Declare packages to be used always including strict and utf8
use strict;
#use utf8;
use IO::File;
#use open OUT => ':utf8';
#use open IN => ':utf8';
use Encode;
#no warning 'utf8';
use charnames ':full';
#use encoding::warnings;
#use encoding 'iso-8859-1';
use open ':encoding(iso-8859-1)';

#Declare variables to be used including command line variables if needed
my ($targetfile, $sourcefile, $argcount, $sourcedirpath, $targetdirpath);
my (@sourceDir);


# ascii to PETSCII conversion isn't as necessary on F256jr version, but keeping to map some special chars
sub asciiToPETSCII 
{
	my $thisChar = $_[0];
	my $thisCharCode = ord($thisChar);
	
	if ($thisCharCode >= 48 && $thisCharCode <= 63)
	{ 		
		#/* 0 - ? */
		return $thisChar;
	}
	elsif ($thisCharCode >= 65 && $thisCharCode <= 90) 
	{	
		##/* A - Z */
		return $thisChar;
	}
	elsif ($thisCharCode == 64)
	{			
		#/* @ */
		return $thisChar;
	}
	elsif ($thisCharCode == 126)
	{			
		#/* tilde, replace with sort by type downward disclosure triangle */
		return chr(248);
	}
	elsif ($thisCharCode >= 91 && $thisCharCode <= 96) 
	{	
		#/* [ - ` */
		return $thisChar;
	}
	elsif ($thisCharCode >= 97 && $thisCharCode <= 126) 
	{ 	
		#/* a - ~ */
		return $thisChar;
	}
	else
	{
		return $thisChar;
	}
}


sub processFile {
	my $theFile = shift;
	my $petscii_version = '';
	my $char;
	
    open (FILEIN, "<:utf8", $theFile) or die "Can't open file $theFile.\n";

    while (<FILEIN>) {
        my $line = $_;
        chomp($line);
		# print "line: $line\n";
		
		# example line:
		# 0	17	This is a string.
		
		# eliminate comment lines
		$line =~ s#\#.+?##gi;

		my ($the_id, $the_len, $the_string) = $line =~ /^(\d+)\t(\d+)\t(.+?)$/s; 			

		if ($the_id ne "" && $the_len ne "" && $the_string ne "")
		{
			# convert ASCII to PETSCII
			$petscii_version = '';
			
			foreach $char (split //, $the_string)
			{
  				$petscii_version .= asciiToPETSCII($char);
			}
			
			my $binary = pack("CC", $the_id, $the_len);
			$binary = encode("iso-8859-1", $binary);
			$petscii_version = encode("iso-8859-1", $petscii_version);
			
			# now "unfix" any printf format specifiers like %s, %i
			#$petscii_version =~ s/\x25\x09/\x25\x49/g; #turns %i petscii back into %i ascii
			#$petscii_version =~ s/\x25\x13/\x25\x53/g; #turns %s petscii back into %s ascii
			#$petscii_version =~ s/\x25\x15/\x25\x55/g; #turns %u petscii back into %u ascii
			#$petscii_version =~ s/(\x25\d\d)\x04/\1\x44/g; #turns %02d petscii back into %02d ascii
			#$petscii_version =~ s/(\x25\d\d)\x15/\1\x55/g; #turns %02u petscii back into %02u ascii
			
			print DATAOUT $binary . $petscii_version;
		}			
    }
    close FILEIN;
}

#Main body of program
# Grab the number of arguments
$argcount = $#ARGV;
# Display usage command if the number of arguments is not at least 1
if ($argcount < 0)
{
    print "Usage: perl strings2binary.pl INPUT_DIRECTORY\n";
    print "\tINPUT_DIRECTORY - The directory containing the text files that you want converted into binary data files.\n";
    print "\t********** Important Note **********\n";
    print "\tFolder names should include no spaces.\n";
    print "\tIf a Folder name must have spaces, then\n";
    print "\tenclose the file name in double quotes.\n";
    exit();
}
else
{
    # get list of files in source dir
    $sourcedirpath = $ARGV[0];
    opendir MYDIR, $sourcedirpath;
		@sourceDir = readdir MYDIR;
		closedir MYDIR;
		
		# set the out file to binary mode. hopefully this will stop it from changing $FF, $A0, etc. to unicode.
		binmode DATAOUT, ':raw';
		
		# put in $5000 as the first two bytes (little endian), which will be default location to read into memory in B128.
	  	#print DATAOUT "\x00\x50";

		# iterate through source files
		foreach $sourcefile ( @sourceDir )
		{
			if ( $sourcefile =~ /\.(?:txt)$/ )
			{
				$sourcefile = "$sourcedirpath/$sourcefile";

				# create binary file if not already created
				#$targetfile = "$sourcefile.bin";
				$targetfile = $sourcefile;
				$targetfile =~ s/\.txt$/\.bin/;
				
				open(FILEHANDLE,$targetfile);
				# Open the output file for writing.
				open DATAOUT, ">$targetfile" or die "can't open Target file: $targetfile $!";
					
				# process each file
				print "processing file: $sourcefile\n";
				processFile($sourcefile);

				# print some closing 00 to overwrite any junk that might be in memory. really only 1 required, but... 
				#print DATAOUT "\x00";
		
				# Close the new file.
				close (DATAOUT);
			}
		}

}

