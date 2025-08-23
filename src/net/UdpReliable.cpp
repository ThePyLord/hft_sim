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
#include "net/ReorderingBuffer.h"

namespace hsnet {

namespace {

class UdpPublication : public IPublication {
public:
    explicit UdpPublication(int sockfd, sockaddr_in dest)
        : sockfd_(sockfd), dest_(dest), next_seq_(0) {}

    PublishResult offer(std::span<const uint8_t> payload, StreamId streamId, bool endOfMessage) noexcept override {
        (void)streamId; (void)endOfMessage;
        
        // For now, just send the payload directly
        hsnet::proto::Header header{};

        std::vector<uint8_t> buffer(sizeof(header) + payload.size());
        hsnet::proto::write_header(buffer.data(), header);
        std::memcpy(buffer.data() + sizeof(header), payload.data(), payload.size());
        ssize_t sent = sendto(sockfd_, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr*>(&dest_), sizeof(dest_));
        if (sent >= 0) {
            next_seq_++;
            return PublishResult::OK;
        }
        return PublishResult::ERROR;
    }

    uint64_t availableWindow() const noexcept override { return UINT64_MAX; }

private:
    int sockfd_;
    sockaddr_in dest_{};
    std::atomic<uint64_t> next_seq_{0};
};

class UdpSubscription : public ISubscription {
public:
    explicit UdpSubscription(int sockfd) : sockfd_(sockfd), reorder_buffer_(1024) {}

    int poll(std::function<void(const MessageView&)> handler, int maxMessages) noexcept override {
        int count = 0;
        
        // First, try to deliver any in-sequence packets from the reordering buffer
        while (count < maxMessages && reorder_buffer_.has_ready()) {
            auto data = reorder_buffer_.get_next();
            if (data) {
                MessageView v{
                    data->data(),
                    static_cast<uint16_t>(data->size()),
                    0, // streamId
                    reorder_buffer_.next_expected() - 1, // sequence number
                    0, // receive time
                    true // endOfMessage
                };
                handler(v);
                count++;
            }
        }
        
        // Then try to receive new packets from the network
        while (count < maxMessages) {
            uint8_t buffer[2048];
            sockaddr_in src{}; socklen_t sl = sizeof(src);
            ssize_t n = recvfrom(sockfd_, buffer, buffer_size, MSG_DONTWAIT, reinterpret_cast<sockaddr*>(&src), &sl);
            if (n <= 0) break;
            
            // For now, assume simple payload without protocol header
            uint64_t seq = reorder_buffer_.next_expected(); // Placeholder

            // Add to reordering buffer
            hsnet::proto::Header header{};
            auto parsed_header = proto::parse_header(buffer, header);
            if (n >= sizeof(header) && parsed_header) {
                seq = ntohll(header.sequence_number);
            } else {
                // Invalid packet, skip
                continue;
            }
            std::vector<uint8_t> data(buffer + sizeof(header), buffer + n);
            if (reorder_buffer_.add(seq, std::move(data))) {
                // Packet was added successfully, try to deliver if it's in sequence
                if (reorder_buffer_.has_ready()) {
                    auto ready_data = reorder_buffer_.get_next();
                    if (ready_data) {
                        MessageView v{
                            ready_data->data(),
                            static_cast<uint16_t>(ready_data->size()),
                            0, // streamId
                            reorder_buffer_.next_expected() - 1, // sequence number
                            0, // receive time
                            true // endOfMessage
                        };
                        handler(v);
                        count++;
                    }
                }
            }
        }
        
        return count;
    }

    bool hasData() const noexcept override { 
        return reorder_buffer_.has_ready(); 
    }

private:
    int sockfd_;
    ReorderingBuffer reorder_buffer_;
    static constexpr size_t buffer_size = 2048;
};

class UdpReliableTransport : public ITransport {
public:
    explicit UdpReliableTransport(const UdpConfig& cfg) : cfg_(cfg) {
        // Create RX socket and bind
        rxSock_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (rxSock_ >= 0) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            auto colon = cfg.local_endpoint.find(':');
            std::string ip = cfg.local_endpoint.substr(0, colon);
            uint16_t port = static_cast<uint16_t>(std::stoi(cfg.local_endpoint.substr(colon + 1)));
            addr.sin_addr.s_addr = ip.empty() ? INADDR_ANY : inet_addr(ip.c_str());
            addr.sin_port = htons(port);
            if (bind(rxSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
                close(rxSock_);
                rxSock_ = -1; // Mark as invalid
            }
        }
        if (rxSock_ < 0) {
            throw std::runtime_error("Failed to create or bind RX socket");
        }
        // Set socket options
        int optval = 1;
        setsockopt(rxSock_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        // Create TX socket and set destination
        txSock_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (txSock_ >= 0) {
            auto colon = cfg.remote_endpoint.find(':');
            std::string ip = cfg.remote_endpoint.substr(0, colon);
            uint16_t port = static_cast<uint16_t>(std::stoi(cfg.remote_endpoint.substr(colon + 1)));
            dest_.sin_family = AF_INET;
            dest_.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &dest_.sin_addr);
        }
    }

    ~UdpReliableTransport() override {
        if (rxSock_ >= 0) close(rxSock_);
        if (txSock_ >= 0) close(txSock_);
    }

    std::unique_ptr<IPublication> create_publication(std::string_view endpoint, StreamId stream) override {
        (void)endpoint; (void)stream;
        return std::make_unique<UdpPublication>(txSock_, dest_);
    }

    std::unique_ptr<ISubscription> create_subscription(std::string_view endpoint, StreamId stream) override {
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

std::unique_ptr<ITransport> make_udp_reliable_transport(const UdpConfig& cfg) {
    return std::make_unique<UdpReliableTransport>(cfg);
}

} // namespace hsnet 