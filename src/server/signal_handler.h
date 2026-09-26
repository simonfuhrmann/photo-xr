#ifndef SRC_SERVER_SIGNAL_HANDLER_H_
#define SRC_SERVER_SIGNAL_HANDLER_H_

#include <atomic>
#include <functional>
#include <thread>

namespace server {

class SignalHandler {
 public:
  SignalHandler();
  ~SignalHandler();

  SignalHandler(const SignalHandler&) = delete;
  SignalHandler& operator=(const SignalHandler&) = delete;

  void SetCallback(int signal, std::function<void()> callback);

 private:
  static void HandleSignal(int signal);

  std::atomic<bool> signal_received_{false};
  std::atomic<bool> running_{true};
  std::function<void()> callback_;
  int signal_ = 0;

  std::thread thread_;
  static SignalHandler* instance_;
};

}  // namespace server

#endif  // SRC_SERVER_SIGNAL_HANDLER_H_
