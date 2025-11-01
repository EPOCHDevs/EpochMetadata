#pragma once
#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace epoch_flow::runtime {
struct Ilogger {
  virtual void log(std::string const &message) = 0;
  virtual std::string str() const = 0;
  virtual void clear() = 0;
  virtual ~Ilogger() = default;
};

struct Logger : Ilogger {
  void log(std::string const &message) override {
    std::lock_guard lck{m_mtx};
    m_messages.push_back(message);
  }

  std::string str() const override {
    std::lock_guard lck{m_mtx};
    return std::ranges::fold_left(
        m_messages, std::string{},
        [](std::string const &acc, std::string const &message) {
          return acc + message + "\n";
        });
  }

  void clear() override {
    std::lock_guard lck{m_mtx};
    m_messages.clear();
  }

private:
  std::vector<std::string> m_messages;
  mutable std::mutex m_mtx;
};

using ILoggerPtr = std::unique_ptr<Ilogger>;
} // namespace epoch_flow::runtime