#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "net/UdpReliable.h"
#include "net/Transport.h"

using namespace std::chrono_literals;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <local_ip:port> <remote_ip:port> [--send <msg>]" << std::endl;
        return 1;
    }

    hsnet::UdpConfig cfg;
    cfg.localEndpoint = argv[1];
    cfg.remoteEndpoint = argv[2];

    auto transport = hsnet::makeUdpReliableTransport(cfg);
    auto pub = transport->createPublication(cfg.remoteEndpoint, cfg.streamId);
    auto sub = transport->createSubscription(cfg.localEndpoint, cfg.streamId);

    if (argc >= 5 && std::string(argv[3]) == "--send") {
        std::string payload = argv[4];
        auto res = pub->offer(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()), cfg.streamId, true);
        std::cout << (res == hsnet::PublishResult::OK ? "sent" : "send error") << std::endl;
        return res == hsnet::PublishResult::OK ? 0 : 2;
    }

    std::cout << "Listening on " << cfg.localEndpoint << ", expecting from " << cfg.remoteEndpoint << std::endl;
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < 5s) {
        int n = sub->poll([&](const hsnet::MessageView& mv){
            std::string s(reinterpret_cast<const char*>(mv.data), mv.length);
            std::cout << "recv: " << s << std::endl;
        }, 32);
        if (n == 0) std::this_thread::sleep_for(1ms);
    }
    return 0;
} 