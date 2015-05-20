#ifndef SHARED_COMM_SENSEBOARD_H
#define SHARED_COMM_SENSEBOARD_H

#include <stdint.h>

namespace Comm {
namespace SensorBoard {

enum mode{
    MODE_STOP, MODE_FOH, MODE_FMH, MODE_PARK
};


struct ControlData {
	bool led_debug_1, led_debug_2;
	bool led_indicator_left, led_indicator_right;
	bool led_front;

	float steering_front, steering_rear;

	enum {
        MODE_VELOCITY = 0,
        MODE_POSITION = 1
    } vel_mode;

	union {
		struct {
			float velocity;
		} velocity;
		struct {
			enum {
				IR_FRONT = 0, IR_BACK = 1, ENCODER = 2
			} target_sensor;
			float threshold;
			float max_velocity;
		} position;
	} control;
};

struct Steering{
    float steering_front;
    float steering_back;
};



struct SensorData {
	bool valid;
	unsigned int invalid_counter;

    Comm::SensorBoard::mode drivemode;
    bool rc_on;

    enum ir_position{ IR_FRONT_LEFT = 0, IR_FRONT_RIGHT, IR_SIDE_REAR, IR_REAR };
    float ir[4];
    float ultrasonic;

    //vertex3f gyro;
    //vertex3f acc;

	float rc_poti;

    int32_t encoder;
    //float velocity;
};

}
}

#endif // SHARED_COMM_SENSEBOARD_H
