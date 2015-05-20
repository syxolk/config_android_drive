#ifndef UDP_RECEIVER_H
#define UDP_RECEIVER_H

#include "lms/module.h"
#include "lms/datamanager.h"
#include "lms/type/module_config.h"
#include "comm/senseboard.h"
#include "lms/extra/time.h"

/**
 * @brief This module is intended to be used in conjunction with the Android App.
 */
class UdpReceiver : lms::Module {
public:
    bool initialize() override;
    bool deinitialize() override;
    bool cycle() override;
private:
    lms::extra::PrecisionTime lastUpdate;

    struct Message {
        float steeringFront;
        float steeringRear;
        float velocity;
    };

    void decodeMessage(Message &m);
    float swapFloat(float n);

    Comm::SensorBoard::ControlData *controlData;

    static constexpr size_t BUF_SIZE = 100;

    const lms::type::ModuleConfig *config;
    int sockfd;
    char buffer[BUF_SIZE];
};

#endif /* UDP_RECEIVER_H */
