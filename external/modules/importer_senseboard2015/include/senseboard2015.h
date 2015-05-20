#ifndef IMPORTER_SENSEBOARD2015_IMPORTER_H
#define IMPORTER_SENSEBOARD2015_IMPORTER_H

#include <termios.h>

#include <lms/datamanager.h>
#include <converter.h>

#include <comm/senseboard.h>
//#include <extra/tracer.h>

#include <comm/protocol.h>
#include <comm/comm.h>

//#include "../../core/include/executionmanager.h"


class Senseboard2015 : public lms::Module {

public:
    //Senseboard2015() : tracer(false) {}
	bool initialize();
	bool deinitialize();

	bool cycle();

protected:
    Comm::SensorBoard::ControlData *m_control_data;
    Comm::SensorBoard::SensorData *m_sensor_data;

    //Tracer tracer;

	Converter convert_ir_front;
	Converter convert_ir_rear;
	Converter convert_ir_r_front;
	Converter convert_ir_r_rear;
	Converter convert_ultrasonic;
	Converter convert_gyro;
	Converter convert_acc;
	Converter convert_encoder;
	Converter convert_rc_poti;
	Converter convert_mode;

    Converter convert_servo_front;
	Converter convert_servo_rear;
	Converter convert_velocity;

    struct up up;
    struct down down;

	struct termios usb_tio;
    int usb_fd;
    std::string usb_path;

    Comm::SensorBoard::mode drivemodeMemory;
    //returns false on success
    bool readData();
    void writeData();
    void prepareDown();
    void interpreteUp();
    bool initUSB();
    void is_valid_fd(int fd);
    bool tooMuchBytesAvailable();

    float convertIR(uint16_t voltage, Comm::SensorBoard::SensorData::ir_position position);
    float linear_interpolation(float f1, float f0, float x1, float x0, float x);
};

#endif
