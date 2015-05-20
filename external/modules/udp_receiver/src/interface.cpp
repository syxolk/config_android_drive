#include "udp_receiver.h"

extern "C" {
void* getInstance() {
    return new UdpReceiver();
}
}
