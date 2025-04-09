
#pragma once

#include <folly/io/async/AsyncPipe.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include "vsoshlb/lib/EventPipeCallback.h"
#include "vsoshlb/lib/VsoshlbLb.h"
#include "vsoshlb/lib/MonitoringStructs.h"

namespace vsoshlb {
namespace monitoring {

/**
class SubscriptionCallback {
 public:
  virtual ~SubscriptionCallback() = default;

  virtual void onClientCanceled(ClientId cid) = 0;

  /**
  virtual bool onClientSubscribed(
      ClientId cid,
      std::shared_ptr<ClientSubscriptionIf> sub,
      const EventIds& subscribed_events) = 0;
};

/**
class MonitoringServiceCore
    : public SubscriptionCallback,
      public std::enable_shared_from_this<MonitoringServiceCore> {
 public:
  MonitoringServiceCore() {}

  ~MonitoringServiceCore() override {
    if (initialized_) {
      tearDown();
    }
  }

  /**
  static std::shared_ptr<MonitoringServiceCore> make() {
    return std::make_shared<MonitoringServiceCore>();
  }

  /**
  virtual bool initialize(std::shared_ptr<VsoshlbMonitor> monitor);

  /**
  virtual void tearDown();

  /**
  typedef struct SubscriptionResult {
    ResponseStatus status;
    std::optional<ClientId> cid;
    std::optional<EventIds> subscribed_events;
    std::optional<std::shared_ptr<SubscriptionCallback>> sub_cb;

    /**
    explicit SubscriptionResult(ResponseStatus status_in) : status(status_in) {}

    /**
    explicit SubscriptionResult(
        ResponseStatus status_in,
        ClientId cid_in,
        EventIds subscribed_events_in,
        std::shared_ptr<SubscriptionCallback> sub_cb_in)
        : status(status_in),
          cid(cid_in),
          subscribed_events(subscribed_events_in),
          sub_cb(sub_cb_in) {}
  } SubscriptionResult;

  /**
  SubscriptionResult acceptSubscription(const EventIds& requested_events);

  /**
  void onClientCanceled(ClientId cid) override;

  /**
  bool onClientSubscribed(
      ClientId cid,
      std::shared_ptr<ClientSubscriptionIf> sub,
      const EventIds& subscribed_events) override;

  /**
  bool initialized() {
    return initialized_;
  }

  /**
  void set_limit(ClientId limit) {
    size_t size = subscription_map_.rlock()->size();
    if (limit >= size) {
      client_limit_ = limit;
    }
  }

  /**
  bool has_client(ClientId cid) {
    auto subsmap = subscription_map_.rlock();
    return subsmap->find(cid) != subsmap->end();
  }

 protected:
  /**
  ClientSubscriptionMap getSubscriptionMapForEvent(EventId eventId);

  /**
  bool addSubscription(
      ClientId cid,
      std::shared_ptr<ClientSubscriptionIf> sub,
      const EventIds& subscribed_events);

  /**
  void cancelSubscription(ClientId cid);

  /**
  bool initialized_{false};

  /**
  std::shared_ptr<VsoshlbMonitor> monitor_{nullptr};

  /**
  folly::Synchronized<ClientSubscriptionMap> subscription_map_;

  /**
  folly::Synchronized<ClientId> curr_cid_{0};

  /**
  ClientId client_limit_{kDefaultClientLimit};

  /**
  EventIds enabled_events_;

  /**
  std::unordered_map<ClientId, std::set<EventId>> client_to_event_ids_;

  /**
  std::unordered_map<EventId, folly::AsyncPipeReader::UniquePtr> readers_;

  /**
  std::unordered_map<EventId, std::shared_ptr<folly::AsyncPipeWriter>> writers_;

  /**
  std::unordered_map<EventId, std::unique_ptr<EventPipeCallback>>
      event_pipe_cbs_;

  /**
  folly::ScopedEventBaseThread reader_thread_;
};

} // namespace monitoring
} // namespace vsoshlb
