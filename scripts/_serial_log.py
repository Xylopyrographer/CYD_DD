"""Reset ESP32 via RTS and capture short boot log"""
import time

import serial

PORT = "COM13"
BAUD = 115200
DURATION = 30
LOG = r"e:\Desktop\GitTemp\ESP32\CYD_DD\serial.log"

ser = serial.Serial(PORT, BAUD, timeout=1)
ser.setDTR(False)
ser.setRTS(True)
time.sleep(0.15)
ser.setRTS(False)
time.sleep(0.1)
ser.flushInput()

end = time.time() + DURATION
with open(LOG, "w", encoding="utf-8", errors="replace") as f:
    while time.time() < end:
        data = ser.readline()
        if data:
            ts = time.strftime("%H:%M:%S")
            f.write(f"[{ts}] {data.decode('utf-8', 'replace').rstrip()}\n")
            f.flush()
ser.close()
print("capture done")
