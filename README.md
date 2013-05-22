T2K ToF TIC DAQ Software
========================

This program operates the SRS SR620 Time Interval Counter and records
measurements.

Instructions
------------

Edit line 15 of `comSR620.c`.  Set the defined variable `SERIAL_DEVICE` to the
appropriate serial port.

1. `make`  
1. `mkdir TicData`  
1. `./sr620`  
