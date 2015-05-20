#include <senseboard2015.h>
//#include <extra/configurationmanager.h>

#include <stdio.h>
#include <unistd.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <string.h>
#include <cmath>

bool Senseboard2015::initialize() {
	printf("Init: senseboard2015\n");

    m_sensor_data = datamanager()->writeChannel<Comm::SensorBoard::SensorData>(this,"SENSOR_DATA");
    m_control_data = datamanager()->writeChannel<Comm::SensorBoard::ControlData>(this,"CONTROL_DATA");

    convert_ir_front.config(getConfig("ir_front"));
    convert_ir_rear.config(getConfig("ir_rear"));
    convert_ir_r_front.config(getConfig("ir_r_front"));
    convert_ir_r_rear.config(getConfig("ir_r_rear"));
    convert_ultrasonic.config(getConfig("ultrasonic"));
    convert_gyro.config(getConfig("gyro"));
    convert_acc.config(getConfig("acc"));
    convert_encoder.config(getConfig("encoder"));
    convert_mode.config(getConfig("mode"));
    convert_rc_poti.config(getConfig("rc_poti"));

    convert_servo_front.config(getConfig("servo_front"));
    convert_servo_rear.config(getConfig("servo_rear"));
    convert_velocity.config(getConfig("velocity"));

    usb_path = getConfig()->get<std::string>("usbDevice", std::string("/dev/ttyACM0"));

    bool success = initUSB();
    if(!success) {
        logger.perror("init") <<"failed to open senseboard";
        return false;
    }
    /// Alle Daten die im Empfangsbuffer liegen löschen
    tcflush(usb_fd, TCIFLUSH);
    memset(&up, 0, sizeof(up));
    memset(&down, 0, sizeof(down));
    drivemodeMemory = Comm::SensorBoard::MODE_STOP;

	return true;
}

bool Senseboard2015::deinitialize() {
	printf("Deinit: senseboard2015\n");

	close(usb_fd);

	return true;
}

int i = 0;
bool Senseboard2015::cycle () {
    m_control_data->control.velocity.velocity = 0.5;
    //get senseboard-data
    ///Prepare Down
    prepareDown();

    //tracer.trace("01 prepare Down");
    /// Read data
    bool timeout = false;

    if(!tooMuchBytesAvailable()) {
        timeout = readData();
    }

    /// Achtung Timeout in der Kommunikation
    if(timeout || tooMuchBytesAvailable()) {
        /// Überprüft oft es einen Input/output Fehler gibt
        /// Wenn ja --> versuche den USB neu zu initialisieren
        is_valid_fd(usb_fd);
    }

    //tracer.trace("02 read");
    /// Send data
    writeData();
    //tracer.trace("03 write");

	///Interprete Up
	if(check_up(&up)) {
        interpreteUp();
	} else {
        m_sensor_data->valid = false;
        m_sensor_data->invalid_counter ++;
        printf("Senseboard Checksum Error. Data is invalid! Counter: %i\n", m_sensor_data->invalid_counter);
    }
    //TODO convert received data to sense-link data
    //just for testing
    logger.debug("info") <<"drivemode: " << m_sensor_data->drivemode << " " << m_sensor_data->encoder;

	return true;
}

void Senseboard2015::writeData() {
    finalize_down(&down);
    write(usb_fd, &down, sizeof(down));
}

bool Senseboard2015::readData() {
    size_t cnt = 0;
    int loops = 0;
    int rcv = 0;
    while (cnt < sizeof(up) && loops < 2000) {
        rcv = read(usb_fd, ((char*)(&up)) + cnt, sizeof(up) - cnt);
        if (rcv > 0)
            cnt += rcv;
        usleep(10);
        loops ++;
    }
    /// Es gab einen Timeout
    if(loops >= 2000)
        return true;

    /// Es gab keinen Timeout
    return false;
}

void Senseboard2015::prepareDown() {
    down.state = 0;
    if (m_control_data->led_debug_1) down.state |= DOWN_STATE_LED_DBG1;
    if (m_control_data->led_debug_2) down.state |= DOWN_STATE_LED_DBG2;
    if (m_control_data->led_indicator_left) down.state |= DOWN_STATE_LIGHT_INDICATOR_L;
    if (m_control_data->led_indicator_right) down.state |= DOWN_STATE_LIGHT_INDICATOR_R;
    if (m_control_data->led_front) down.state |= DOWN_STATE_LIGHT_FRONT;

    down.v_mode = (DOWN_MODE)m_control_data->vel_mode;
    switch(down.v_mode) {
    case DOWN_MODE_VELOCITY_CONTROL:
        down.mode.v.vsoll = convert_velocity.convert<float, int16_t>(m_control_data->control.velocity.velocity);
        break;
    case DOWN_MODE_POSITION_CONTROL:
        down.mode.p.vmax = m_control_data->control.position.max_velocity;
        down.v_mode &= ~(DOWN_MASK_SENSOR);
        down.v_mode |= m_control_data->control.position.target_sensor;

        switch(m_control_data->control.position.target_sensor) {
        case DOWN_SENSOR_IR_FRONT:
            down.mode.p.value = convert_ir_front.reverse<uint8_t, float>(m_control_data->control.position.threshold);
            break;
        case DOWN_SENSOR_IR_REAR:
            down.mode.p.value = convert_ir_rear.reverse<uint8_t, float>(m_control_data->control.position.threshold);
            break;
        case DOWN_SENSOR_ENCODER:
            down.mode.p.value = convert_encoder.reverse<uint8_t, float>(m_control_data->control.position.threshold);
            break;
        }
        break;
    }
    logger.error()<<m_control_data->steering_front;
    logger.error()<<m_control_data->steering_rear;
    down.servo_f = convert_servo_front.convert<float, int16_t>(-m_control_data->steering_front); // minus wegen neuen servos
    down.servo_r = convert_servo_rear.convert<float, int16_t>(m_control_data->steering_rear);   // minus wegen neuen servos
    down.v_mode = DOWN_MODE_VELOCITY_CONTROL;
}

void Senseboard2015::interpreteUp() {
    m_sensor_data->rc_on = (up.state & UP_MASK_RC);
    m_sensor_data->drivemode = (Comm::SensorBoard::mode)(up.state >> 1);

//    sensor_data->ir[Comm::SensorBoard::SensorData::IR_FRONT_LEFT] = convert_ir_r_front.convert<uint16_t, float>(up.ir[0]);
//    sensor_data->ir[Comm::SensorBoard::SensorData::IR_FRONT_RIGHT] = convert_ir_front.convert<uint16_t, float>(up.ir[1]);
//    sensor_data->ir[Comm::SensorBoard::SensorData::IR_SIDE_REAR] = convert_ir_r_rear.convert<uint16_t, float>(up.ir[2]);
//    sensor_data->ir[Comm::SensorBoard::SensorData::IR_REAR] = convert_ir_rear.convert<uint16_t, float>(up.ir[3]);
    m_sensor_data->ir[Comm::SensorBoard::SensorData::IR_FRONT_RIGHT] = convertIR(up.ir[0], Comm::SensorBoard::SensorData::IR_FRONT_RIGHT);
    m_sensor_data->ir[Comm::SensorBoard::SensorData::IR_FRONT_LEFT] = convertIR(up.ir[1], Comm::SensorBoard::SensorData::IR_FRONT_LEFT);
    m_sensor_data->ir[Comm::SensorBoard::SensorData::IR_SIDE_REAR] = convertIR(up.ir[2], Comm::SensorBoard::SensorData::IR_SIDE_REAR);
    m_sensor_data->ir[Comm::SensorBoard::SensorData::IR_REAR] =  convertIR(up.ir[3], Comm::SensorBoard::SensorData::IR_REAR);
    m_sensor_data->ultrasonic = convert_ultrasonic.convert<uint16_t, float>(up.ultrasonic);
    m_sensor_data->rc_poti = convert_rc_poti.convert<uint8_t, float>(up.rc_poti);
    m_sensor_data->encoder += up.enc_diff;
    /*sensor_data->velocity = convert_encoder.convert<uint8_t, float>(up.enc_diff);
    sensor_data->gyro[0] = convert_gyro.convert<int16_t, float>(up.gyro[0]);
    sensor_data->gyro[1] = convert_gyro.convert<int16_t, float>(up.gyro[1]);
    sensor_data->gyro[2] = convert_gyro.convert<int16_t, float>(up.gyro[2]);

    sensor_data->acc[0] = convert_acc.convert<int16_t, float>(up.acc[0]);
    sensor_data->acc[1] = convert_acc.convert<int16_t, float>(up.acc[1]);
    sensor_data->acc[2] = convert_acc.convert<int16_t, float>(up.acc[2]);*/
    m_sensor_data->valid = true;
    m_sensor_data->invalid_counter = 0;
}

bool Senseboard2015::tooMuchBytesAvailable() {
    int bytes_available;
    ioctl(usb_fd, FIONREAD, &bytes_available);
    //printf("bytes available %i \n", bytes_available);
    if(bytes_available > sizeof(up)) {
        tcflush(usb_fd, TCIFLUSH);
        logger.warn("\033[031m FLUSHED BUFFER, TOO MUCH BYTES AVAILABLE! \033[0m \n");
        ioctl(usb_fd, FIONREAD, &bytes_available);
        logger.warn("\033[032m Bytes available:") << bytes_available;
        usleep(10);
        return true;
    }
    usleep(10);

    return false;
}

void Senseboard2015::is_valid_fd(int fd) {
    /// Sende Anfrage damit ioctl errno neu setzt
    int rc = ioctl(fd, USBDEVFS_CONNECTINFO, 0);

    /// Wenn rc < 0 gab es einen Fehler
    if (rc > 0)
        return;

    perror("\033[031m Error in ioctl \033[0m");
    /// Haben wir einen Input/Output error? -> usb neu initialisieren
    if(errno == EIO) {
        close(usb_fd);
        while(!initUSB()) {
            usleep(100);
        }
    }

    return;
}

bool Senseboard2015::initUSB() {
    usb_fd = open(usb_path.c_str(), O_RDWR | O_NOCTTY | O_NDELAY); ///TODO is nonblocking useful

    if (usb_fd < 0) {
        return false;
    }

    ///Termios
    //further reading http://en.wikibooks.org/wiki/Serial_Programming/termios
    tcgetattr(usb_fd, &usb_tio);

    //
    // Input flags - Turn off input processing
    // convert break to null byte, no CR to NL translation,
    // no NL to CR translation, don't mark parity errors or breaks
    // no input parity check, don't strip high bit off,
    // no XON/XOFF software flow control
    //
    usb_tio.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
                        INLCR | PARMRK | INPCK | ISTRIP | IXON);
    //
    // Output flags - Turn off output processing
    // no CR to NL translation, no NL to CR-NL translation,
    // no NL to CR translation, no column 0 CR suppression,
    // no Ctrl-D suppression, no fill characters, no case mapping,
    // no local output processing
    //
    // usb_tio.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
    //                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
    usb_tio.c_oflag = 0;
    //
    // No line processing:
    // echo off, echo newline off, canonical mode off,
    // extended input processing off, signal chars off
    //
    usb_tio.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    //
    // Turn off character processing
    // clear current char size mask, no parity checking,
    // no output processing, force 8 bit input
    //
    usb_tio.c_cflag &= ~(CSIZE | PARENB);
    usb_tio.c_cflag |= CS8;
    //
    // One input byte is enough to return from read()
    // Inter-character timer off
    //
    usb_tio.c_cc[VMIN]  = 1;
    usb_tio.c_cc[VTIME] = 0;

    if(tcsetattr(usb_fd, TCSANOW, &usb_tio) < 0) {
        perror("SET ATTR");
    }

    if(cfsetispeed(&usb_tio, B115200) < 0 || cfsetospeed(&usb_tio, B115200) < 0) {
        perror("Baud rate\n");

    }

    return true;
}

float Senseboard2015::convertIR(uint16_t voltage, Comm::SensorBoard::SensorData::ir_position position) {
    // Neue Werte vom 27.01.2015
    int i = 0;
    float f0 = 0.0, f1 = 0.0, x0 = 0.0, x1 = 0.0, dist = 0.0;
    const float ir_front_left[2][33] = {
        {
            0,  10,  20,  30,  40,  50,  60,  70,  80,  90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300, 350, 400
        },
        {
            695, 575, 483, 405, 350, 310, 267, 237, 215, 200, 189, 180, 170, 161, 150, 147, 140, 130, 135, 135, 130, 127, 122, 122, 110, 105, 109, 105, 100, 105, 100,  88,  65
        }
    };
    const float ir_front_right[2][33] = {
        {
             0,  10,  20,  30,  40,  50,  60,  70,  80,  90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300, 350, 400
        },
        {
            685, 555, 463, 388, 349, 291, 253, 231, 200, 182, 155, 141, 131, 120, 115, 105, 100,  96,  96,  90,  85,  90,  85,  80,  60,  61,  67,  65,  62,  67,  63,  70,  52
        }
    };
    const float ir_side_rear[2][17] = {
        {
             0,  50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600, 650, 700, 750, 800
        },
        {
            705, 507, 348, 270, 209, 178, 142, 115, 106,  97,  83,  70,  65,  65,  60,  50,  42
        }
    };
    const float ir_rear[2][12] = {
        {
            0,  10,  20,  30,  40,  50, 100, 150, 200, 250, 300, 350
        },
        {
            660, 659, 630, 520, 430, 380, 247, 180, 150, 122, 113, 102
        }
    };

    switch(position) {
        case Comm::SensorBoard::SensorData::IR_FRONT_LEFT:
        for(i=0;i<33;i++) {
            if(voltage<=ir_front_left[1][i]) {
                f0 = ir_front_left[0][i];
                x0 = ir_front_left[1][i];
                if(i > 0) {
                    f1 = ir_front_left[0][i-1];
                    x1 = ir_front_left[1][i-1];
                }
                else {
                    f1 = dist;
                    x1 = x0;
                }
            }
        }
        break;
        case Comm::SensorBoard::SensorData::IR_FRONT_RIGHT:
        for(i=0;i<33;i++) {
            if(voltage<=ir_front_right[1][i]) {
                f0 = ir_front_right[0][i];
                x0 = ir_front_right[1][i];
                if(i > 0) {
                    f1 = ir_front_right[0][i-1];
                    x1 = ir_front_right[1][i-1];
                }
                else {
                    f1 = dist;
                    x1 = x0;
                }
            }
        }
        break;
        case Comm::SensorBoard::SensorData::IR_SIDE_REAR:
        for(i=0;i<17;i++) {
            if(voltage<=ir_side_rear[1][i]) {
                f0 = ir_side_rear[0][i];
                x0 = ir_side_rear[1][i];
                if(i > 0) {
                    f1 = ir_side_rear[0][i-1];
                    x1 = ir_side_rear[1][i-1];
                }
                else {
                    f1 = dist;
                    x1 = x0;
                }
            }
        }
        break;
        case Comm::SensorBoard::SensorData::IR_REAR:
        for(i=0;i<12;i++) {
            if(voltage<=ir_rear[1][i]) {
                f0 = ir_rear[0][i];
                x0 = ir_rear[1][i];
                if(i > 0) {
                    f1 = ir_rear[0][i-1];
                    x1 = ir_rear[1][i-1];
                }
                else {
                    f1 = dist;
                    x1 = x0;
                }
            }
        }
        break;
    }
    dist = linear_interpolation(f1, f0, x1, x0, voltage);
    return dist;
}



float Senseboard2015::linear_interpolation(float f1, float f0, float x1, float x0, float x) {
    float range = 0.0;
    if( (x1-x0) != 0.0 ) { //Div by 0 protection
        range = ( f0 + ((f1-f0)/(x1-x0))*(x-x0) )/1000.0;
        return range;
    }
    range = f0/1000.0;
    return range;
}
