from serial import Serial, PARITY_EVEN, PARITY_ODD
from serial.tools.list_ports import comports
import time
import numpy as np


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
        # id, length
        data = bytearray([1, 12])
        for i in range(3):
            data += (int(position[i])).to_bytes(
                byteorder="little", length=4, signed=True
            )
        self._send(data)

    def echo(self, msg: bytes):
        """
        :brief: Echo command
        :param msg: Message that controller will echo back to serial port
        """
        data = bytearray([2, len(msg)]) + msg
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
        checksum = 0  # TODO: Actually compute the checksum
        self.serial.write(checksum.to_bytes(1, "little"))


COMPORT = "COM12"
ctller = Controller(COMPORT)
ctller.set_position([0, 100000, 0])

# for i in range(20):
#     # Demonstrate position setting
#     ctller.set_position([500, 1000, 1500])
#     time.sleep(5)
#     ctller.set_position([0, 0, 0])
#     time.sleep(5)

# # Demonstrate echo
# for i in range(32):
#     data = bytearray()
#     for j in range(8):
#         data += (i * 8 + j).to_bytes(1, "little")
#     ctller.echo(data)

#     for _ in range(5):
#         msg = ctller.serial.read(10000)
#         if msg != b"":
#             print(list(msg))

# class SerialFake:
#     def write(self, x):
#         print(list(x))

# ctller.serial = SerialFake()


def drill():
    # Demonstrate drill cycle =======
    start = np.array([0, 0, 0])  # Starting position
    ctller.set_position(start)
    curr = start

    # print("Moving drill down")
    # curr[2] += 50000
    # ctller.set_position(curr)
    # time.sleep(6)

    # Start orbiting
    print("Start orbiting")
    ORBIT_MAX_R = 30000
    ORBIT_COUNTS_PER_REV = 360
    ORBIT_ITER_COUNT = 1800
    center = curr
    last = curr
    for t in range(ORBIT_ITER_COUNT + 1):
        r = t / ORBIT_ITER_COUNT * ORBIT_MAX_R
        theta = t / ORBIT_COUNTS_PER_REV * 2 * np.pi
        x = r * np.cos(theta)
        y = r * np.sin(theta)
        curr = center.copy()
        curr[0] += x
        curr[1] += y
        ctller.set_position(curr)
        sleep_time = 0.0001 * (np.max(np.abs(curr - last)))
        time.sleep(0.0001 * (np.max(np.abs(curr - last))))
        print(sleep_time)
        last = curr

    # TODO: Continue circling

    # Return to center
    print("Returning to center)")
    ctller.set_position(center)
    time.sleep(3)


# # Move drill up
# ctller.set_position(start)
# time.sleep(10)
