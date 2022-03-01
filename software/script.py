from serial import Serial, PARITY_EVEN, PARITY_ODD
from serial.tools.list_ports import comports
import time

# for port in comports():
#     print(port.name)

COMPORT = "COM12"
driver = Serial(COMPORT, baudrate=19200, parity=PARITY_EVEN, dsrdtr=None, timeout=0.1)

# Opening the serial port causes the board to reset:
# https://forum.arduino.cc/t/soft-reset-on-arduino-through-starting-serial-comm/26755
# Can be addressed by sleeping for some time, so the board can reboot:
# https://stackoverflow.com/questions/23332198/arduino-python-serial-communication-write-not-working
time.sleep(3)
print("Finished sleeping")

for i in range(20):
    driver.write(
        bytearray(
            [
                0x7E,
                1,  # id
                12,  # length
            ]
        )
        + (5000).to_bytes(byteorder="little", length=4) * 3
        + b"0"
    )
    for _ in range(20):
        msg = driver.read(10000).decode(errors="replace")
        if msg != "":
            print(msg)
    driver.write(
        bytearray(
            [
                0x7E,
                1,  # id
                12,  # length
            ]
        )
        + (-5000).to_bytes(byteorder="little", length=4, signed=True) * 3
        + b"0"
    )
    for _ in range(20):
        msg = driver.read(10000).decode(errors="replace")
        if msg != "":
            print(msg)
