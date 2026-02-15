#include "src/server/util/status_or.h"

#include <iostream>

namespace util {

void StatusOrRaiseBadAccess() {
  std::cerr << "Invalid access to StatusOr::value()" << std::endl;
  std::abort();
}

}  // namespace util
