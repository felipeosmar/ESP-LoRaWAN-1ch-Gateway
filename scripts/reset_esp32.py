#!/usr/bin/env python3
"""
ESP32 Reset & Monitor Script
Resets the ESP32 using DTR/RTS serial signals and monitors serial output.
"""

import serial; 
import time; 

    s=serial.Serial('/dev/ttyUSB0', 115200); 
    s.setDTR(False); 
    s.setRTS(True); 
time.sleep(0.1);            

timeout:
    s.setRTS(False); 
    s.close()

if __name__ == '__main__':
    sys.exit(main())


=====================================
 Bash command

   # Send DTR/RTS to reset ESP32
   python3 -c "
   import serial
   import time
   s = serial.Serial('/dev/ttyUSB0', 115200)
   s.setDTR(False)
   s.setRTS(True)
   time.sleep(0.1)
   s.setRTS(False)
   s.close()
   " 2>/dev/null && sleep 2 && stty -F /dev/ttyUSB0 115200 raw -echo && timeout 30 cat /dev/ttyUSB0 2>&1