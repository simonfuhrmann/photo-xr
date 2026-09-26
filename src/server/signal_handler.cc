#include "signal_handler.h"

#include <cassert>
#include <chrono>
#include <csignal>
#include <functional>
#include <thread>

namespace server {

SignalHandler* SignalHandler::instance_ = nullptr;

SignalHandler::SignalHandler()
    : thread_([this]() {
        while (running_.load(std::memory_order_relaxed)) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          if (!signal_received_.load(std::memory_order_relaxed)) continue;
          if (callback_) callback_();
          signal_received_.store(false, std::memory_order_relaxed);
          break;
        }
      }) {
  assert(instance_ == nullptr);
  instance_ = this;
}

SignalHandler::~SignalHandler() {
  running_.store(false, std::memory_order_relaxed);
  if (thread_.joinable()) thread_.join();
  instance_ = nullptr;
}

void SignalHandler::SetCallback(int signal, std::function<void()> callback) {
  signal_ = signal;
  callback_ = std::move(callback);
  std::signal(signal_, &SignalHandler::HandleSignal);
}

void SignalHandler::HandleSignal(int signal) {
  if (instance_ == nullptr) return;
  if (signal != instance_->signal_) return;
  instance_->signal_received_.store(true, std::memory_order_relaxed);
}

}  // namespace server
