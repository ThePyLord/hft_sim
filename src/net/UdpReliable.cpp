#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "net/UdpReliable.h"
#include "net/Protocol.h"

namespace hsnet {

namespace {

class UdpPublication : public IPublication {
public:
    explicit UdpPublication(int sockfd, sockaddr_in dest)
        : sockfd_(sockfd), dest_(dest) {}

    PublishResult offer(std::span<const uint8_t> payload, StreamId streamId, bool endOfMessage) noexcept override {
        (void)streamId; (void)endOfMessage;
        ssize_t sent = sendto(sockfd_, payload.data(), payload.size(), 0, reinterpret_cast<sockaddr*>(&dest_), sizeof(dest_));
        return sent >= 0 ? PublishResult::OK : PublishResult::ERROR;
    }

    uint64_t availableWindow() const noexcept override { return UINT64_MAX; }

private:
    int sockfd_;
    sockaddr_in dest_{};
};

class UdpSubscription : public ISubscription {
public:
    explicit UdpSubscription(int sockfd) : sockfd_(sockfd) {}

    int poll(std::function<void(const MessageView&)> handler, int maxMessages) noexcept override {
        int count = 0;
        while (count < maxMessages) {
            uint8_t buffer[2048];
            sockaddr_in src{}; socklen_t sl = sizeof(src);
            ssize_t n = recvfrom(sockfd_, buffer, sizeof(buffer), MSG_DONTWAIT, reinterpret_cast<sockaddr*>(&src), &sl);
            if (n <= 0) break;
            MessageView v{buffer, static_cast<uint16_t>(n), 0, 0, 0, true};
            handler(v);
            ++count;
        }
        return count;
    }

    bool hasData() const noexcept override { return false; }

private:
    int sockfd_;
};

class UdpReliableTransport : public ITransport {
public:
    explicit UdpReliableTransport(const UdpConfig& cfg) : cfg_(cfg) {
        // Create RX socket and bind
        rxSock_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (rxSock_ >= 0) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            auto colon = cfg.localEndpoint.find(':');
            std::string ip = cfg.localEndpoint.substr(0, colon);
            uint16_t port = static_cast<uint16_t>(std::stoi(cfg.localEndpoint.substr(colon + 1)));
            addr.sin_addr.s_addr = ip.empty() ? INADDR_ANY : inet_addr(ip.c_str());
            addr.sin_port = htons(port);
            bind(rxSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }
        // Create TX socket and set destination
        txSock_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (txSock_ >= 0) {
            auto colon = cfg.remoteEndpoint.find(':');
            std::string ip = cfg.remoteEndpoint.substr(0, colon);
            uint16_t port = static_cast<uint16_t>(std::stoi(cfg.remoteEndpoint.substr(colon + 1)));
            dest_.sin_family = AF_INET;
            dest_.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &dest_.sin_addr);
        }
    }

    ~UdpReliableTransport() override {
        if (rxSock_ >= 0) close(rxSock_);
        if (txSock_ >= 0) close(txSock_);
    }

    std::unique_ptr<IPublication> createPublication(std::string_view endpoint, StreamId stream) override {
        (void)endpoint; (void)stream;
        return std::make_unique<UdpPublication>(txSock_, dest_);
    }

    std::unique_ptr<ISubscription> createSubscription(std::string_view endpoint, StreamId stream) override {
        (void)endpoint; (void)stream;
        return std::make_unique<UdpSubscription>(rxSock_);
    }

private:
    UdpConfig cfg_{};
    int rxSock_{-1};
    int txSock_{-1};
    sockaddr_in dest_{};
};

} // namespace

std::unique_ptr<ITransport> makeUdpReliableTransport(const UdpConfig& cfg) {
    return std::make_unique<UdpReliableTransport>(cfg);
}

} // namespace hsnet 