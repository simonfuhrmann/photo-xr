#ifndef SRC_UTIL_STATUS_H_
#define SRC_UTIL_STATUS_H_

#include <string>
#include <string_view>
#include <type_traits>

#include "src/server/util/string_utils.h"

namespace util {

// Status codes to be used with `util::Status`. The codes are compatible with
// `absl::StatusCode` for the canonical error space.
enum class StatusCode {
  OK = 0,                   // Operation was successful (not an error).
  CANCELLED = 1,            // Operation was cancelled.
  UNKNOWN = 2,              // Specific error is unknown.
  INVALID_ARGUMENT = 3,     // Provided argument was invalid.
  DEADLINE_EXCEEDED = 4,    // Some deadline or timeout expired.
  NOT_FOUND = 5,            // Some requested entity was not found.
  ALREADY_EXISTS = 6,       // Some entity already exists.
  PERMISSION_DENIED = 7,    // Insufficient privileges for an operation.
  RESOURCE_EXHAUSTED = 8,   // Some resource is exhausted.
  FAILED_PRECONDITION = 9,  // Preconditions for an operation are not met.
  ABORTED = 10,             // Some operation was aborted.
  OUT_OF_RANGE = 11,        // Operation accesses outside of a valid range.
  UNIMPLEMENTED = 12,       // Operation is not implemented.
  INTERNAL = 13,            // Bugs, things that should not happen.
  UNAVAILABLE = 14,         // Resource temporarily not available.
  DATA_LOSS = 15,           // Data loss or data corruption.
  UNAUTHENTICATED = 16,     // Authentication required.
};

// `util::Status` is used to propagate status (error) messages associated with
// somewhat generic error codes. It is usually returned from a function that
// can either succeed or fail. For example:
//
//   const util::Status status = DoSomethingThatCanFail();
//
// Functions returning `util::Status` should return `util::OkStatus() on
// success, and make use of the constructors for the specific error codes.
// For example:
//
//   util::Status DoSomethingThatCanFail() {
//     if (Check()) {
//       return util::FailedPreconditionError("Check() failed");
//     }
//     return util::OkStatus();
//   }
//
// As a special treat, a `util::Status` can directly be constructed from an
// errno error code, with an appropriate status code and message. For example:
//
//   std::sqrt(-1.0);
//   return util::StatusFromErrno(errno);
//
// This API is inspired by `absl::Status`, but no code has been taken from the
// Abseil library. The error codes are compatible with `absl::Status`. Some
// method names use lower-case style to be somewhat API compatible to ABSL.
class [[nodiscard]] Status {
 public:
  // Constructs a Status that holds OK and no error message.
  Status() = default;

  // Constructs a Status with the given error `code` and `message`.
  Status(StatusCode code, std::string_view message);

  // Copy and move constructors and assignment operators.
  Status(const Status& other) = default;
  Status& operator=(const Status& other) = default;
  Status(Status&& other) noexcept = default;
  Status& operator=(Status&& other) noexcept = default;

  // Comparison operators.
  bool operator==(const Status& other) const;
  bool operator!=(const Status& other) const;

  // Returns true if the status code is OK.
  bool ok() const;
  // Returns the status code.
  StatusCode code() const;
  // Returns the status message. Can be empty, esp. when OK.
  std::string_view message() const;

  // Returns a new status a string prefixed/suffixed to the message.
  // This adds a colon separator, e.g., "prefix: message", or "message: suffix".
  Status WithPrefix(std::string_view prefix) const;
  Status WithSuffix(std::string_view suffix) const;

  // Ignores errors. Does nothing except suppress [[nodiscard]] errors.
  void IgnoreError() const;

 private:
  StatusCode code_ = StatusCode::OK;
  std::string message_;
};

static_assert(std::is_move_constructible<Status>::value,
              "util::Status must be move constructible");
static_assert(std::is_move_assignable<Status>::value,
              "util::Status must be move assignable");

// Constructors for specific error codes.
Status OkStatus();
Status CancelledError(std::string_view message);
Status UnknownError(std::string_view message);
Status InvalidArgumentError(std::string_view message);
Status DeadlineExceededError(std::string_view message);
Status NotFoundError(std::string_view message);
Status AlreadyExistsError(std::string_view message);
Status PermissionDeniedError(std::string_view message);
Status ResourceExhaustedError(std::string_view message);
Status FailedPreconditionError(std::string_view message);
Status AbortedError(std::string_view message);
Status OutOfRangeError(std::string_view message);
Status UnimplementedError(std::string_view message);
Status InternalError(std::string_view message);
Status UnavailableError(std::string_view message);
Status DataLossError(std::string_view message);
Status UnauthenticatedError(std::string_view message);

// Constructor for errno error codes.
Status StatusFromErrno(int error_number);

// Exits the program with a std::exit(EXIT_FAILURE), does not return.
void ExitProgramIfStatusError(const Status& status);

// Evalutes an expression that produces a `util::Status`. If the status code
// is not OK, returns it from the current scope.
#define RETURN_IF_ERROR(expr)              \
  do {                                     \
    const ::util::Status _status = (expr); \
    if (!_status.ok()) return _status;     \
  } while (false)

// Evalutes and expression that produces a `util::Status`. If the status code
// is not OK, terminates the program using std::exit(EXIT_FAILURE).
#define EXIT_IF_ERROR(expr) ExitProgramIfStatusError(expr)

// Inline implementation.

inline Status::Status(StatusCode code, std::string_view message)
    : code_(code), message_(code != StatusCode::OK ? message : "") {}

inline bool Status::operator==(const Status& other) const {
  return code_ == other.code_ && message_ == other.message_;
}

inline bool Status::operator!=(const Status& other) const {
  return !(*this == other);
}

inline bool Status::ok() const { return code_ == StatusCode::OK; }

inline StatusCode Status::code() const { return code_; }

inline std::string_view Status::message() const { return message_; }

inline Status Status::WithPrefix(std::string_view prefix) const {
  if (ok()) return *this;
  return Status(code(), StrCat(prefix, ": ", message()));
}

inline Status Status::WithSuffix(std::string_view suffix) const {
  if (ok()) return *this;
  return Status(code(), StrCat(message(), ": ", suffix));
}

inline void Status::IgnoreError() const {}

inline Status OkStatus() { return Status(); }

inline Status CancelledError(std::string_view message) {
  return Status(StatusCode::CANCELLED, message);
}

inline Status UnknownError(std::string_view message) {
  return Status(StatusCode::UNKNOWN, message);
}

inline Status InvalidArgumentError(std::string_view message) {
  return Status(StatusCode::INVALID_ARGUMENT, message);
}

inline Status DeadlineExceededError(std::string_view message) {
  return Status(StatusCode::DEADLINE_EXCEEDED, message);
}

inline Status NotFoundError(std::string_view message) {
  return Status(StatusCode::NOT_FOUND, message);
}

inline Status AlreadyExistsError(std::string_view message) {
  return Status(StatusCode::ALREADY_EXISTS, message);
}

inline Status PermissionDeniedError(std::string_view message) {
  return Status(StatusCode::PERMISSION_DENIED, message);
}

inline Status ResourceExhaustedError(std::string_view message) {
  return Status(StatusCode::RESOURCE_EXHAUSTED, message);
}

inline Status FailedPreconditionError(std::string_view message) {
  return Status(StatusCode::FAILED_PRECONDITION, message);
}

inline Status AbortedError(std::string_view message) {
  return Status(StatusCode::ABORTED, message);
}

inline Status OutOfRangeError(std::string_view message) {
  return Status(StatusCode::OUT_OF_RANGE, message);
}

inline Status UnimplementedError(std::string_view message) {
  return Status(StatusCode::UNIMPLEMENTED, message);
}

inline Status InternalError(std::string_view message) {
  return Status(StatusCode::INTERNAL, message);
}

inline Status UnavailableError(std::string_view message) {
  return Status(StatusCode::UNAVAILABLE, message);
}

inline Status DataLossError(std::string_view message) {
  return Status(StatusCode::DATA_LOSS, message);
}

inline Status UnauthenticatedError(std::string_view message) {
  return Status(StatusCode::UNAUTHENTICATED, message);
}

}  // namespace util

#endif  // SRC_UTIL_STATUS_H_
