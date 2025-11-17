#pragma once
#include "../../../../include/epoch_script/data/database/updates/iwebsocket_manager.h"
#include <epoch_data_sdk/model/asset/asset.hpp>
#include <atomic>
#include <drogon/WebSocketClient.h>
#include <atomic_queue/atomic_queue.h>
#include <memory>
#include <boost/signals2/signal.hpp>
#include <vector>

namespace epoch_script::data {

struct AlpacaWebSocketManagerOptions {
  AssetClass assetClass;
  std::string key;
  std::string secret;
  bool testing{false};
  std::string feed{"iex"};
};

struct SubscriptionRequest {
  const std::string action{"subscribe"};
  std::vector<std::string> bars;
};

struct AuthRequest {
  const std::string action{"auth"};
  std::string key;
  std::string secret;
};

constexpr size_t kSubscriptionRequestQueueSize = 16;

// NEW ───────────────────────────────────────────────────────────────
constexpr size_t kBarOutboxSize = 1024;

// ── connection life‑cycle ──────────────────────────────────────────
enum class ConnectionState : std::uint8_t {
  Idle,
  Connecting,
  Authenticating,
  Streaming,
  Closing
};

class AlpacaWebSocketManager final : public IWebSocketManager {
public:
  explicit AlpacaWebSocketManager(const AlpacaWebSocketManagerOptions &options);
  ~AlpacaWebSocketManager() override;

  void Connect() override;
  void Disconnect() override;

  void HandleNewMessage(const NewMessageObserver &handler) override {
    m_newMessageSignal.connect(handler);
  }

  void Subscribe(const asset::AssetHashSet &assets) override;

private:
  AlpacaWebSocketManagerOptions m_options;
  drogon::WebSocketClientPtr m_client;
  mutable NewMessageSignal m_newMessageSignal;
  atomic_queue::AtomicQueue2<asset::AssetHashSet, kSubscriptionRequestQueueSize>
      m_subQueue;

  std::atomic<ConnectionState> m_state{ConnectionState::Idle};
  std::atomic<std::uint32_t> m_reconnectAttempts{0};
  bool m_manualCloseRequested{false};

  std::string GetPath() const;

  static std::string GetSymbol(const asset::Asset &asset);
  bool ValidateAssets(const asset::AssetHashSet &assets) const;

  void CompleteSubscriptionRequest(SubscriptionRequest const &) const;

  void flushSubscriptions();
  bool handleControlMessage(const std::string &msg,
                            const drogon::WebSocketClientPtr &wsPtr);
  void onClosed();

  void ParseAndDispatch(const std::string &raw) const;
};

} // namespace epoch_script::data
