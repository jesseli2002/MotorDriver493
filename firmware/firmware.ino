#include <cstring>
#include "packet.h"

#define MOTOR_IDLE 0xFF
#define MOTOR_X_PULSE 0b11111110
#define MOTOR_X_DIR   0b11111101
#define MOTOR_Y_PULSE 0b11111011
#define MOTOR_Y_DIR   0b11110111
#define MOTOR_Z_PULSE 0b11101111
#define MOTOR_Z_DIR   0b11011111

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
void Motor_commmand(char cmd){
    PORTD = ((cmd & MOTOR_PORTD_BITS) << 2) | (PORTD & 0b00000011);
    PORTB = ((cmd & MOTOR_PORTB_BITS) >> 6) | (PORTB & 0b11111100) ;
}

Packet packet;
/**
 * Protocol:
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
    Motor_commmand(MOTOR_IDLE);
}

// Reference (target) position
long position_ref[3]; // XYZ array
// Actual (current) position
long position[3]; // XYZ array


// Time between steps (needed to avoid skipping), in us. TODO: Tune
#define DELAY_TIME 25
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
                std::memcpy(&(position_ref[i]), &(packet.dat[i * 4]), 4);
            }
        }
    }
}

void loop() {
    checkCommands();
    Serial.write("Target position, XYZ: ");
    Serial.print(position_ref[0]);
    Serial.write(", ");
    Serial.print(position_ref[1]);
    Serial.write(", ");
    Serial.println(position_ref[2]);
    delay(500);
    return;


    int steps = 2500;

    Motor_commmand(MOTOR_IDLE);
    delayMicroseconds(DELAY_TIME);
    for (int i = 0; i < steps; ++i) {
        Motor_commmand(MOTOR_X_PULSE);
        delayMicroseconds(DELAY_TIME);
        Motor_commmand(MOTOR_IDLE);
        delayMicroseconds(DELAY_TIME);
    }
    delayMicroseconds(DELAY_TIME);
    for (int i = 0; i < steps; ++i) {
        Motor_commmand(MOTOR_X_PULSE & MOTOR_X_DIR);
        delayMicroseconds(DELAY_TIME);
        Motor_commmand(MOTOR_X_DIR);
        delayMicroseconds(DELAY_TIME);
    }
    delay(500);
}
