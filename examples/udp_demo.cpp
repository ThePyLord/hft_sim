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
    cfg.local_endpoint = argv[1];
    cfg.remote_endpoint = argv[2];

    auto transport = hsnet::make_udp_reliable_transport(cfg);
    auto pub = transport->create_publication(cfg.remote_endpoint, cfg.stream_id);
    auto sub = transport->create_subscription(cfg.local_endpoint, cfg.stream_id);

    if (argc >= 5 && std::string(argv[3]) == "--send") {
        std::string payload = argv[4];
        auto res = pub->offer(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()), cfg.stream_id, true);
        std::cout << (res == hsnet::PublishResult::OK ? "sent" : "send error") << std::endl;
        return res == hsnet::PublishResult::OK ? 0 : 2;
    }

    std::cout << "Listening on " << cfg.local_endpoint << ", expecting from " << cfg.remote_endpoint << std::endl;
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