
rem Set the platform name
set PLATFORM=XIAO_ESP32C3

rem Path to your Arduino IDE installation
set ARDUINO_PATH="C:\Program Files\Arduino"

rem Path to your Arduino sketch
set SKETCH_PATH="C:\Users\danie\Documents\GitHub\DayPix\DayPix"

rem Compile the sketch
%ARDUINO_PATH%\arduino-builder -compile -hardware %ARDUINO_PATH%\hardware -hardware %USERPROFILE%\AppData\Local\Arduino15\packages -tools %ARDUINO_PATH%\tools-builder -tools %ARDUINO_PATH%\hardware\tools\avr -tools %USERPROFILE%\AppData\Local\Arduino15\packages -built-in-libraries %ARDUINO_PATH%\libraries -libraries %SKETCH_PATH%\libraries -fqbn <board_fqbn> %SKETCH_PATH%

rem Rename the binary file with the platform name
ren %SKETCH_PATH%\DayPix.ino.bin sketch_name_%PLATFORM%.bin
