#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "net/Transport.h"
#include "net/UdpReliable.h"

using namespace std::chrono_literals;

int main() {
    hsnet::FeedSubscriberConfig subCfg;
    subCfg.multicast_group = "239.1.1.1";  // hardcoded group
    subCfg.port = 8170;                    // hardcoded port

    auto subscriber = hsnet::make_udp_reliable_subscriber(subCfg);

    std::cout << "Listening on multicast group " << subCfg.multicast_group
              << ":" << subCfg.port << std::endl;

    auto start = std::chrono::steady_clock::now();
    int message_count = 0;

    while (std::chrono::steady_clock::now() - start < 30s) {
        int n = subscriber->poll([&](const hsnet::MessageView& mv) {
            message_count++;
            std::string s(reinterpret_cast<const char*>(mv.data), mv.length);
            std::cout << "=== RECEIVED MESSAGE #" << message_count << " ===\n";
            std::cout << "Data: '" << s << "'\n";
            std::cout << "Length: " << mv.length << " bytes\n";
        },
                                8);

        if (n == 0) {
            std::this_thread::sleep_for(1ms);
        }
    }

    std::cout << "Finished. Total messages: " << message_count << std::endl;
    return 0;
}