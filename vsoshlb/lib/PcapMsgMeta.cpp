#include <fmt/core.h>

namespace vsoshlb {

using EventId = monitoring::EventId;

    : msg_(std::move(msg)), event_(event) {}

    : msg_(std::move(msg.msg_)),
      event_(msg.event_),
      packetLimit_(msg.packetLimit_),
      restart_(msg.restart_),
      control_(msg.control_),
      stop_(msg.stop_),
      shutdown_(msg.shutdown_) {}

  msg_ = std::move(msg.msg_);
  event_ = msg.event_;
  packetLimit_ = msg.packetLimit_;
  restart_ = msg.restart_;
  control_ = msg.control_;
  stop_ = msg.stop_;
  shutdown_ = msg.shutdown_;
}

  return msg_;
}

  try {
    return static_cast<EventId>(event_);
  } catch (const std::exception& e) {
    LOG(ERROR) << fmt::format("invalid event {}: {}", event_, e.what());
    return EventId::UNKNOWN;
  }
}

} // namespace vsoshlb
