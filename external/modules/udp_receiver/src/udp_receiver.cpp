#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>

#include "udp_receiver.h"

bool UdpReceiver::initialize() {
    controlData = datamanager()->writeChannel<Comm::SensorBoard::ControlData>(this,"CONTROL_DATA");
    config = getConfig();

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(-1 == sockfd) {
        logger.perror("init") << "Could not invoke socket()";
        return false;
    }

    struct sockaddr_in servAddr;
    std::memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(config->get<int>("port", 7213));
    if(-1 == bind(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr))) {
        logger.perror("init") << "Could not invoke bind()";
        return false;
    }

    lastUpdate = lms::extra::PrecisionTime::now();

    return true;
}

bool UdpReceiver::deinitialize() {
    close(sockfd);

    return true;
}

bool UdpReceiver::cycle() {
    while(true) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int retval = select(sockfd + 1, &rfds, NULL, NULL, &tv);

        if(retval > 0 && FD_ISSET(sockfd, &rfds)) {
            logger.info("cycle") << "received";
            struct sockaddr_in clientAddr;

            unsigned int clientAddrLen = sizeof (clientAddr);
            memset(buffer, 0, BUF_SIZE);
            size_t received = recvfrom(sockfd, buffer, BUF_SIZE, 0,
                                       (struct sockaddr *) &clientAddr, &clientAddrLen);

            logger.info("cycle") << received << " bytes";

            Message *msg;

            if(received == 12) {
                msg = reinterpret_cast<Message*>(buffer);
                decodeMessage(*msg);

                logger.debug() << msg->steeringFront << " "
                               << msg->steeringRear << " " << msg->velocity;

                controlData->vel_mode = Comm::SensorBoard::ControlData::MODE_VELOCITY;
                controlData->steering_front = msg->steeringFront;
                controlData->steering_rear = - msg->steeringRear;
                controlData->control.velocity.velocity = -3 * msg->velocity;

    //            buffer[0] = 1;
    //            sendto(sockfd, buffer, 1, 0, (struct sockaddr *) &clientAddr, clientAddrLen);

                lastUpdate = lms::extra::PrecisionTime::now();
            }
        } else {
            break;
        }
    }

    if(lms::extra::PrecisionTime::since(lastUpdate) > lms::extra::PrecisionTime::fromMillis(500)) {
        controlData->vel_mode = Comm::SensorBoard::ControlData::MODE_VELOCITY;
        controlData->steering_front = 0;
        controlData->steering_rear = 0;
        controlData->control.velocity.velocity = 0;
        logger.warn() << "Failsafe";
    }

    return true;
}

void UdpReceiver::decodeMessage(Message &m) {
    m.steeringFront = swapFloat(m.steeringFront);
    m.steeringRear = swapFloat(m.steeringRear);
    m.velocity = swapFloat(m.velocity);
}

float UdpReceiver::swapFloat(float n) {
    std::uint32_t *ptr = reinterpret_cast<std::uint32_t*>(&n);
    std::uint32_t result = ntohl(*ptr);
    return * reinterpret_cast<float*>(&result);
}
