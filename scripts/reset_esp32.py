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
