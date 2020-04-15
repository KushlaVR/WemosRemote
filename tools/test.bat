echo off
echo "Enter port number, and press Enter to upload setup module"

set /p portNumber="COM"

echo "Port number is COM%portNumber%"

esptool.exe --chip esp8266 --port COM%portNumber% write_flash 0x00200000 test.bin
pause