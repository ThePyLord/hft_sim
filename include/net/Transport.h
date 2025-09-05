#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string_view>

namespace hsnet {

using StreamId = uint32_t;

struct MessageView {
    const uint8_t* data;
    uint16_t       length;
    StreamId       streamId;
    uint64_t       sequenceNumber;
    uint64_t       receiveTimeNs;
    bool           endOfMessage;
};

enum class PublishResult : uint8_t {
    OK,
    BACKPRESSURED,
    NOTCONNECTED,
    CLOSED,
    ERROR
};

class ISubscription {
public:
    virtual ~ISubscription() = default;
    virtual int poll(std::function<void(const MessageView&)> handler, int maxMessages) noexcept = 0;
    virtual bool hasData() const noexcept = 0;
};

class IPublication {
public:
    virtual ~IPublication() = default;
    virtual PublishResult offer(std::span<const uint8_t> payload,
                                StreamId streamId,
                                bool endOfMessage = true) noexcept = 0;
    virtual uint64_t availableWindow() const noexcept = 0;
};

class ITransport {
public:
    virtual ~ITransport() = default;
    virtual std::unique_ptr<IPublication> create_publication(std::string_view endpoint, StreamId stream) = 0;
    virtual std::unique_ptr<ISubscription> create_subscription(std::string_view endpoint, StreamId stream) = 0;
};

} // namespace hsnet 