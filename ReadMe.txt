garmin-ida-loader
This is an IDA loader module for Garmin GPS firmwares. 
Tested on IDA v5.5 and v6.1, but should work on all versions from v4.9.
Compilation requires Visual Studio 2005 or newer.
To compile the project put idasdk55 directory near the project directory or set up 'C/C++ --> General --> Additional include directories' and 'Linker --> Input --> Additional dependencies' to point to '<sdkdir>\include' and '<sdkdir>\libvc.w32\ida.lib' respectively.
