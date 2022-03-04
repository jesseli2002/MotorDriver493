# Motor control
The parallel port splitter reverses the pinout polarities, but due to optoisolation it still works. After plugging in USB power (provides 5V power line), hold all pins high for normal operation. To drive the motor, pulse a pin from high (normal) to low to high.

A 4us delay between motor pulses seems to be the absolute minimum to avoid skipping. But it can't reverse at that speed (i.e. go at full speed one way, then switch directions). 8us seems mostly reliable, 7us seems to skip some times, 6us definitely skips.
Since we're not in a hurry, we can add plenty of margin - set delay time to 25us to leave plenty of space.

# Axes definitions
Green connected to left/right motion (x-axis)
Blue connected to forward/back motion (y-axis)
Red connected to up/down motion (z-axis)

# Packet format
- 1 byte start ID (0x7E)
- 1 byte ID
- 1 byte length (unsigned char), no more than 32
- N bytes data - see length.
- 1 byte checksum (currently unused)

- Escape byte is 0xA5
- Data should be little-endian

## `set_position`
- ID: 1
- Length: 12
- Data:
    - 4-byte X position
    - 4-byte Y position
    - 4-byte Z position

## `echo`
- ID: 2
- Length: variable
- Data:
    - String which will be printed back to serial port


Problem: In echoing the response, what appears to happen is that the received data (raw, with escape codes) is being put directly into the data buffer. 
