T2K ToF TIC DAQ Software
========================

This program operates the SRS SR620 Time Interval Counter and records
measurements.

Instructions
------------

To build, just run make and then make the 'TicData' directory before running
the program.

1. `make`  
1. `mkdir TicData`  
1. `./sr620 [<options>]`  

Typical options would be

    -d /dev/ttyUSB0 -q : query /dev/ttyUSB0 and reports the responce to *ID? command
    -d /dev/ttyUSB0 --tic LSUTIC -a Cs -b PT_Sept -l NU1 : starts DAQ into a
        file with filename made of the given elements.

There are many new options.  See a list with '-h'.

    Output file naming convention: 
    YYYYMMDDTHHMMSS.<location>.<channel-A>-<channel B>.<tic id>[.user_suffix].dat 
    Usage parameters: 
    -d, --device - specify device to open, defaults to:  /dev/ttyS0 
    -p, --datapath - specify data path, deafults to: ./TicData/ 
    -h, --help - print this help 
    -t, --tic - specify valid tic name (from list): TravTIC, NictTIC, LsuTIC, SkTIC, 
    -a, --channel-a - source of the signal for channel a (free string) 
    -b, --channel-b - source of the signal for channel b (free string) 
    -l, --location - specify valid location (from list): NU1, SK, ND280, OTHER, 
    -u, --user - user added suffix for measurement description (free string) 
    -o, --override - override naming convention with this string 
    -q, --query - query the tic 
    -r, --rotate - enable daily file rotation
    -v, --verbose - print data to standard output too
