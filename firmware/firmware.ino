#include <string.h>
#include "packet.h"

#define MOTOR_IDLE 0xFF

const unsigned char MOTOR_PULSE[3] = {
    0b11111110,
    0b11111011,
    0b11101111,
};
const unsigned char MOTOR_DIR[3] = {
   0b11111101,
   0b11110111,
   0b11011111
};

// Time between steps (needed to avoid skipping), in microseconds
#define MOTOR_DELAY_TIME 25
// Time to wait for signal to settle, to avoid race conditions in direction
#define MOTOR_SIGNAL_SETTLE_TIME 5

// #define MOTOR_X_PULSE 0b11111110
// #define MOTOR_Y_PULSE 0b11111011
// #define MOTOR_Z_PULSE 0b11101111
// #define MOTOR_X_DIR   0b11111101
// #define MOTOR_Y_DIR   0b11110111
// #define MOTOR_Z_DIR   0b11011111

/*
Port D maps to Arduino digital pins 0 to 7; 0, 1 are used by UART
Port B maps to Arduino digital pins 8 to 13; the high bits 6, 7 map to crystal pins => inaccessible
DDRn is direction (high for output)
PORTn is port value

Use pins 2 thru 9 inclusive. Pin 2 => LSB, pin 9 => MSB.
-> lower 6 bits sent to port D, upper 2 bits sent to port B
*/
#define MOTOR_PORTD_BITS 0b00111111
#define MOTOR_PORTB_BITS 0b11000000

/**
 * @brief Writes a command to the motor driver parallel port.
*/
void Motor_command(char cmd){
    PORTD = ((cmd & MOTOR_PORTD_BITS) << 2) | (PORTD & 0b00000011);
    PORTB = ((cmd & MOTOR_PORTB_BITS) >> 6) | (PORTB & 0b11111100);
}

Packet packet;

// Reference (target) position
long position_ref[3]; // XYZ array
// Actual (current) position
long position[3]; // XYZ array

#define MAX_ITERS 100

/**
 * Protocol (planned):
 * SW sends a query command on startup/periodically to ask if further commands can be sent.
 * FW will respond to query indicating yes/no, as well as how many commands are remaining in queue.
 *
 * SW must send query before every control command; otherwise there is no guarantee that the FW will not drop the command.
 *
 * Escape byte and start bytes must be escaped, by first putting the escape byte then printing the next value XOR the escape byte.
*/
void setup() {
    DDRD = 0xFF;
    DDRB = 0xFF;
    Serial.begin(19200, SERIAL_8E1); // 8 bits, even parity, 1 stop bit
    Packet_init(&packet);
    Motor_command(MOTOR_IDLE);

    for (int i = 0; i < 3; ++i){
        position_ref[i] = 0;
        position[i] = 0;
    }
}

inline void checkCommands() {
    while (true) {

        int nextChar = Serial.read();
        if (nextChar == -1) {
            return;
        }

        // else data read
        if (!Packet_parse(&packet, static_cast<char>(nextChar))) {
            continue;
        }
        // else packet is parsed

        switch(packet.id) {
        case 0:
            break; // reserved for E-stop

        case 1:
            // Provides absolute target update, little endian
            for(int i = 0; i < 3; ++i) {
                memcpy(&(position_ref[i]), &(packet.dat[i * 4]), 4);
            }
            break;

        case 2:
            // Echo
            for (int i = 0; i < packet.len; ++i) {
                Serial.write(packet.dat[i]);
            }
            break;
        }
    }
}

void loop() {
    // int steps = 2500;

    checkCommands();

    unsigned char idle_val = MOTOR_IDLE;
    unsigned char motor_val = MOTOR_IDLE;

    int iters = MAX_ITERS;
    int error_count = 3;
    while (error_count != 0 && --iters != 0)
    {
        error_count = 3;
        for (int i = 0; i < 3; ++i) {
            if (position[i] < position_ref[i]) {
                motor_val &= MOTOR_PULSE[i];
                ++position[i];
            } else if (position[i] > position_ref[i]){
                idle_val &= MOTOR_DIR[i];
                motor_val &= MOTOR_PULSE[i] & MOTOR_DIR[i];
                --position[i];
            } else {
                --error_count;
            }
        }

        Motor_command(idle_val);
        delayMicroseconds(MOTOR_DELAY_TIME);
        Motor_command(motor_val);
        delayMicroseconds(MOTOR_DELAY_TIME);
        Motor_command(idle_val);
        delayMicroseconds(MOTOR_DELAY_TIME);
    }
}
