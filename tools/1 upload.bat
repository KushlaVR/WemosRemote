echo off
echo "Enter port number, and press Enter."

set /p portNumber="COM"

echo "Port number is COM%portNumber%"

esptool.exe --chip esp8266 --port COM%portNumber% write_flash 0 firmware.bin
pause