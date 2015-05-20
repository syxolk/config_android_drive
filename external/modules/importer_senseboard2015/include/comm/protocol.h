#ifndef COMM_PROTOCOL_H
#define COMM_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
///############################# DOWN COMM ##########################

enum DOWN_MASKS {
    DOWN_MASK_LED = 0x03,
    DOWN_MASK_LIGHT = 0x1c,
};

enum DOWN_STATE {
    DOWN_STATE_LED_DBG1 = 0x01,
    DOWN_STATE_LED_DBG2 = 0x02,
    DOWN_STATE_LIGHT_FRONT = 0x04,
    DOWN_STATE_LIGHT_INDICATOR_L = 0x08,
    DOWN_STATE_LIGHT_INDICATOR_R = 0x10,
};

enum DOWN_MODE {
    DOWN_MODE_VELOCITY_CONTROL = 0,
    DOWN_MODE_POSITION_CONTROL = 1
};

enum DOWN_POSITION_SENSOR {
	DOWN_SENSOR_IR_FRONT = 0x04,
	DOWN_SENSOR_IR_REAR = 0x08,
	DOWN_SENSOR_ENCODER = 0x0b
};

enum DOWN_MASK {
    DOWN_MASK_MODE = 0x03,
    DOWN_MASK_SENSOR = 0x0b
};

struct down {
	int8_t state;
    int16_t servo_f;
    int16_t servo_r;
    uint8_t v_mode; //enum DOWN_MODE und DOWN_POSITION_SENSOR
    union {
        struct {
            int16_t vsoll;
        } v;
        struct {
            int16_t value;
            int16_t vmax;
        } p;
    } mode;
    uint8_t checksum;
} __attribute__ ((packed));

///############################### UP COMM ##########################

enum UP_IR_POSITIONS {
    UP_IR_FRONT = 0,
    UP_IR_SIDE_FRONT = 1,
    UP_IR_SIDE_REAR = 2,
    UP_IR_REAR = 3,
};

enum UP_STATEMASK {
    UP_MASK_RC = 0x01,
};

enum UP_STATEBYTE {
    UP_STATE_RC = 0x01,
    UP_STATE_STOP = 0x00,
    UP_STATE_FOH = 0x02,
    UP_STATE_FMH = 0x04,
    UP_STATE_PARKING = 0x06,
};

struct up {
    uint8_t state;
    //uint32_t timestamp;
    uint16_t ir[4];
    uint16_t ultrasonic;
    //int16_t gyro[3];
    //int16_t acc[3];

    int16_t enc_diff;
    uint8_t rc_poti;

    uint8_t checksum;
} __attribute__ ((packed));

#ifdef __cplusplus
}
#endif
#endif
