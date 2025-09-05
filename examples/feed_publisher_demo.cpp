#include <iostream>
#include <iomanip>
#include <string>

#include "net/Transport.h"
#include "net/UdpReliable.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <message>" << std::endl;
        return 1;
    }

    std::string payload = argv[1];

    hsnet::FeedPublisherConfig pubCfg;
    pubCfg.multicast_group = "239.1.1.1";  // using hardcoded group
    pubCfg.port = 8170;                    // using hardcoded port

    // hsnet::
    auto publisher = hsnet::make_udp_reliable_publisher(pubCfg);
    std::cout << "Welcome to the HFT SIM Demo Publisher..." << std::endl;
    std::cout << "Here are some helpful commands\n";
    std::cout << "q - quit the program\n";
	auto res = publisher->offer(
		std::span<const uint8_t>(
			reinterpret_cast<const uint8_t*>(payload.data()), payload.size()),
		pubCfg.stream_id,
		true);
    std::cout << "s msg - Send a message (msg)\n";
    // make_udp_reliable_publisher(pubCfg);
    // auto cmd =
    std::cout << "------------- START --------------\n\n";
    std::string prompt{"> "};
    std::string userInput;
    std::cout << prompt;
    std::getline(std::cin, userInput);
    std::cout << std::endl;
    std::cout << userInput << std::endl;

    while (!userInput.empty() && userInput.at(0) != 'q') {
        auto msg = !userInput.empty() ? userInput : "";
        if (!msg.empty() && msg.starts_with("s ")) {
            payload = msg.substr(2);
            std::cout << "sending payload " << payload << std::endl;
            res = publisher->offer(
                std::span<const uint8_t>(
                    reinterpret_cast<const uint8_t*>(payload.data()), payload.size()),
                pubCfg.stream_id,
                true);
			if (res == hsnet::PublishResult::OK) {
				std::cout << "Published: " << payload << std::endl;
			} else {
				std::cerr << "Publish failed" << std::endl;
			}
        } else if (!msg.empty() && msg.starts_with("q"))
            break;
		
        std::cout << prompt;
        std::getline(std::cin, userInput);
    }

    return 0;
}