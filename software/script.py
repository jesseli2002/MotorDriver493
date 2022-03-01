from msilib.schema import Control
from serial import Serial, PARITY_EVEN, PARITY_ODD
from serial.tools.list_ports import comports
import time


class Controller:
    START_BYTE = b"\x7E"
    ESCAPE_BYTE = b"\xA5"
    ESCAPE_BYTE_NUM = 0xA5
    ESCAPE_VALS = [0x7E, 0xA5]

    def __init__(self, comport: str):
        """
        :brief: Constructs the controller.
        :param comport: What Windows COM port to use, e.g. "COM1"
        """
        self.serial = Serial(
            comport, baudrate=19200, parity=PARITY_EVEN, dsrdtr=None, timeout=0.1
        )

        # Opening the serial port causes the board to reset:
        # https://forum.arduino.cc/t/soft-reset-on-arduino-through-starting-serial-comm/26755
        # Can be addressed by sleeping for some time, so the board can reboot:
        # https://stackoverflow.com/questions/23332198/arduino-python-serial-communication-write-not-working
        time.sleep(3)

    def set_position(self, position):
        """
        :brief: Commands the machine to a particular position.
        :param position: Array of 3 absolute positions, representing X,Y,Z coordinates for the machine.
        """
        data = bytearray(
            [
                1,  # id
                12,  # length
            ]
        )
        for i in range(3):
            data += (position[i]).to_bytes(byteorder="little", length=4, signed=True)
        self._send(data)

    def _send(self, data):
        """
        :brief: Sends a command over the serial port.
        """
        self.serial.write(self.START_BYTE)
        for byte in data:
            if byte in self.ESCAPE_VALS:
                self.serial.write(self.ESCAPE_BYTE)
                byte ^= self.ESCAPE_BYTE_NUM
            self.serial.write(byte.to_bytes(1, "little"))
        checksum = 0 # TODO: Actually compute the checksum
        self.serial.write(checksum.to_bytes(1, "little"))


COMPORT = "COM12"
ctller = Controller(COMPORT)
for i in range(20):
    ctller.set_position([5000, 10000, 15000])
    time.sleep(5)
    ctller.set_position([-5000, -10000, -15000])
    time.sleep(5)


# for _ in range(20):
#     msg = ctller.serial.read(10000).decode(errors="replace")
#     if msg != "":
#         print(msg)
