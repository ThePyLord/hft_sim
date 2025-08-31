#include "net/UdpReliable.h"

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
#include <iostream>

#include "net/Protocol.h"
#include "net/ReorderingBuffer.h"

namespace hsnet {

namespace {
static constexpr std::string MULTICAST_ADDR{"239.1.1.1"};
static constexpr int MULTICAST_PORT{8170};

class UdpPublication : public IPublication {
   public:
    explicit UdpPublication(int sockfd, sockaddr_in dest)
        : sockfd_(sockfd), dest_(dest), next_seq_(0) {}

    // Multicast setup
    UdpPublication(int sockfd) : sockfd_(sockfd), next_seq_{0} {}

    PublishResult offer(std::span<const uint8_t> payload, StreamId streamId, bool endOfMessage) noexcept override {
        (void)streamId;
        (void)endOfMessage;

        // Create header with proper sequence number
        hsnet::proto::Header header{};
        header.sequence_number = next_seq_;
        header.payload_length = static_cast<uint16_t>(payload.size());
        header.stream_id = streamId;

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

class FeedPublication : public IPublication {
   public:
    explicit FeedPublication(const FeedPublisherConfig& cfg) : next_seq_(0) {
        // Multicast setup
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd_ >= 0) {
            int optval = 1;
            // setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
            // setup multicast
            txSock_.sin_family = AF_INET;
            // txSock_.sin_port = htons(cfg.port);
            txSock_.sin_port = htons(MULTICAST_PORT);
            // txSock_.sin_addr.s_addr = inet_addr(cfg.multicast_group.c_str());
            txSock_.sin_addr.s_addr = inet_addr(MULTICAST_ADDR.c_str());
            setsockopt(sockfd_, IPPROTO_IP, IP_MULTICAST_TTL, &optval, sizeof(optval));
        }
    }

    ~FeedPublication() override {
        close(sockfd_);
    }

    PublishResult offer(std::span<const uint8_t> payload, StreamId streamId, bool endOfMessage) noexcept override {
        (void)streamId;
        (void)endOfMessage;

        // Create header with proper sequence number
        hsnet::proto::Header header{};
        header.sequence_number = next_seq_;
        header.payload_length = static_cast<uint16_t>(payload.size());
        header.stream_id = streamId;

        std::vector<uint8_t> buffer(sizeof(header) + payload.size());
        hsnet::proto::write_header(buffer.data(), header);
        std::memcpy(buffer.data() + sizeof(header), payload.data(), payload.size());
        ssize_t sent = sendto(sockfd_, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr*>(&txSock_), sizeof(txSock_));
        if (sent >= 0) {
            next_seq_++;
            return PublishResult::OK;
        }
        return PublishResult::ERROR;
    }

    uint64_t availableWindow() const noexcept override { return UINT64_MAX; }

   private:
    int sockfd_;
    sockaddr_in txSock_{};
    std::atomic<uint64_t> next_seq_{0};
};

class FeedSubscription : public ISubscription {
   public:
    explicit FeedSubscription(const FeedSubscriberConfig& cfg) : addr({}), mreq({}),reorder_buff(1024) {
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd_ >= 0) {
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            addr.sin_port = htons(cfg.port);
            int optval = 1;
            setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
            setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

            if (bind(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
                throw std::runtime_error("Failed to bind or create subscription socket!");

            // Set mx_addr to constant (Might use config value in the future)
            mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR.c_str());
            // Setting to loopback for now, once again ignoring config value
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            int did_set_sock = setsockopt(sockfd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
            if (did_set_sock < 0) {
                throw std::runtime_error("Failed to set socket options for membership!");
            }
        }
    }

    ~FeedSubscription() override { close(sockfd_); }

    int poll(std::function<void(const MessageView&)> handler, int maxMessages) noexcept override {
        int count = 0;
        using namespace std::chrono;
        while (count < maxMessages && reorder_buff.has_ready()) {
            auto data = reorder_buff.get_next();
            if (data) {
                auto now_time_point = steady_clock::now().time_since_epoch();
                uint64_t recv_ts = duration_cast<nanoseconds>(now_time_point).count();
                MessageView v{
                    data->first.data(),
                    static_cast<uint16_t>(data->first.size()),
                    data->second,
                    reorder_buff.next_expected() - 1,
                    recv_ts,
                    true};
                handler(v);
                count++;
            }
        }

        while (count < maxMessages) {
            uint8_t buff[2048];
            sockaddr src{};
            socklen_t socklen{sizeof(src)};

            ssize_t n = recvfrom(sockfd_, &buff, sizeof(buff), MSG_DONTWAIT, reinterpret_cast<sockaddr*>(&src), &socklen);
            if (n <= 0) break;

            hsnet::proto::Header header{};
            auto is_parsed = hsnet::proto::parse_header(buff, header);
            if (!is_parsed) continue;

            auto payload_len = header.payload_length;
            if (n < static_cast<ssize_t>(sizeof(header) + payload_len)) continue;

            // Extract payload data
            std::vector<uint8_t> data(buff + sizeof(header), buff + sizeof(header) + payload_len);
            auto seq = header.sequence_number;
            // Add to reordering buffer
            if (reorder_buff.add(seq, std::move(data), header.stream_id)) {
                // Packet was added successfully, try to deliver if it's in sequence
                while (reorder_buff.has_ready()) {
                    auto ready_data = reorder_buff.get_next();
                    if (ready_data) {
                        MessageView v{
                            ready_data->first.data(),
                            static_cast<uint16_t>(ready_data->first.size()),
                            ready_data->second,                // stream_id from packet
                            reorder_buff.next_expected() - 1,  // sequence number
                            0,                                 // receive time
                            true                               // endOfMessage
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
        return reorder_buff.has_ready();
    }

   private:
    int sockfd_;
    sockaddr_in addr;
    ip_mreq mreq;
    ReorderingBuffer reorder_buff;
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
                    data->first.data(),
                    static_cast<uint16_t>(data->first.size()),
                    data->second,                         // stream_id from packet
                    reorder_buffer_.next_expected() - 1,  // sequence number
                    0,                                    // receive time
                    true                                  // endOfMessage
                };
                handler(v);
                count++;
            }
        }

        // Then try to receive new packets from the network
        while (count < maxMessages) {
            uint8_t buffer[2048];
            sockaddr_in src{};
            socklen_t sl = sizeof(src);
            ssize_t n = recvfrom(sockfd_, buffer, buffer_size, MSG_DONTWAIT, reinterpret_cast<sockaddr*>(&src), &sl);
            if (n <= 0) break;

            // Parse protocol header
            hsnet::proto::Header header{};
            if (n < static_cast<ssize_t>(sizeof(header))) {
                // Packet too small, skip
                continue;
            }

            auto parsed_header = hsnet::proto::parse_header(buffer, header);
            if (!parsed_header) {
                // Invalid header, skip
                continue;
            }

            // (problematic if receiving from multiple sources)
            // (see UdpPublication constructor)
            // uint64_t seq = header.sequence_number;

            // For simplicity, assume in-order for now
            uint64_t seq = reorder_buffer_.next_expected();
            uint16_t payload_len = header.payload_length;

            // Validate payload length
            if (n < static_cast<ssize_t>(sizeof(header) + payload_len)) {
                // Packet truncated, skip
                continue;
            }

            // Extract payload data
            std::vector<uint8_t> data(buffer + sizeof(header), buffer + sizeof(header) + payload_len);
            
            // Add to reordering buffer
            if (reorder_buffer_.add(seq, std::move(data), header.stream_id)) {
                // Packet was added successfully, try to deliver if it's in sequence
                if (reorder_buffer_.has_ready()) {
                    auto ready_data = reorder_buffer_.get_next();
                    if (ready_data) {
                        MessageView v{
                            ready_data->first.data(),
                            static_cast<uint16_t>(ready_data->first.size()),
                            ready_data->second,                   // stream_id from packet
                            reorder_buffer_.next_expected() - 1,  // sequence number
                            0,                                    // receive time
                            true                                  // endOfMessage
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
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(MULTICAST_PORT);
            if (bind(rxSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
                close(rxSock_);
                rxSock_ = -1;  // Mark as invalid
            }
            ip_mreq mreq;
            mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR.c_str());
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            //
            setsockopt(rxSock_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
        }
        if (rxSock_ < 0) {
            throw std::runtime_error("Failed to create or bind RX socket");
        }
        // Set socket options
        int optval = 1;
        setsockopt(rxSock_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        // Create TX socket and set destination
        // Make tx socket non-blocking and multicast
        txSock_ = socket(AF_INET, SOCK_DGRAM, 0);
        sockaddr_in tx_addr{};
        if (txSock_ >= 0) {
            // setup multicast
            tx_addr.sin_family = AF_INET;
            if (cfg_.remote_endpoint == "multicast") {
                tx_addr.sin_port = htons(MULTICAST_PORT);
                tx_addr.sin_addr.s_addr = inet_addr(MULTICAST_ADDR.c_str());
                dest_ = tx_addr;
                setsockopt(txSock_, IPPROTO_IP, IP_MULTICAST_TTL, &optval, sizeof(optval));
            } else {
                auto colon = cfg.remote_endpoint.find(':');
                std::string ip = cfg.remote_endpoint.substr(0, colon);
                uint16_t port = static_cast<uint16_t>(std::stoi(cfg.remote_endpoint.substr(colon + 1)));
                dest_.sin_family = AF_INET;
                dest_.sin_port = htons(port);
                inet_pton(AF_INET, ip.c_str(), &dest_.sin_addr);
            }
        }
    }

    ~UdpReliableTransport() override {
        if (rxSock_ >= 0) close(rxSock_);
        if (txSock_ >= 0) close(txSock_);
    }

    std::unique_ptr<IPublication> create_publication(std::string_view endpoint, StreamId stream) override {
        (void)endpoint;
        (void)stream;
        return std::make_unique<UdpPublication>(txSock_, dest_);
    }

    std::unique_ptr<ISubscription> create_subscription(std::string_view endpoint, StreamId stream) override {
        (void)endpoint;
        (void)stream;
        return std::make_unique<UdpSubscription>(rxSock_);
    }

   private:
    UdpConfig cfg_{};
    int rxSock_{-1};
    int txSock_{-1};
    sockaddr_in dest_{};
};

}  // namespace

std::unique_ptr<IPublication> make_udp_reliable_publisher(const FeedPublisherConfig& cfg) {
    return std::make_unique<FeedPublication>(cfg);
}

std::unique_ptr<ISubscription> make_udp_reliable_subscriber(const FeedSubscriberConfig& cfg) {
    return std::make_unique<FeedSubscription>(cfg);
}

std::unique_ptr<ITransport> make_udp_reliable_transport(const UdpConfig& cfg) {
    return std::make_unique<UdpReliableTransport>(cfg);
}

}  // namespace hsnet