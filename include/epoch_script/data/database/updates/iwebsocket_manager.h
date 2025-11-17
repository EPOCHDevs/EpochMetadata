#pragma once
#include <epoch_data_sdk/model/asset/asset.hpp>
#include <memory>
#include <boost/signals2/signal.hpp>

// ── NEW: standard containers & functional ──────────────────────────
#include <functional>
#include <vector>

namespace epoch_script::data {
// ── forward declarations & convenience aliases ────────────────────
namespace asset = data_sdk::asset;
struct BarMessage {
  std::string s;
  double o;
  double h;
  double l;
  double c;
  double v;
  int64_t t_utc;
};

using BarList = std::vector<BarMessage>;

using NewMessageSignal = boost::signals2::signal<void(const BarList &)>;
using NewMessageObserver =
    NewMessageSignal::slot_type;

struct IWebSocketManager {
  virtual ~IWebSocketManager() = default;

  virtual void Connect() = 0;
  virtual void Disconnect() = 0;
  virtual void HandleNewMessage(const NewMessageObserver &handler) = 0;
  virtual void Subscribe(const asset::AssetHashSet &assets) = 0;
};

using IWebSocketManagerPtr = std::shared_ptr<IWebSocketManager>;
} // namespace epoch_script::data
