#ifndef SRC_UTIL_STATUS_OR_H_
#define SRC_UTIL_STATUS_OR_H_

#include <cstddef>
#include <new>
#include <type_traits>

#include "src/server/util/status.h"

namespace util {

// The `util::StatusOr<T>` template either holds a value of type `T`, or an
// error of type `util::Status` indicating why a value is not present. This
// type is typically returned from a function, and is a suitable alternative
// to exceptions. For example:
//
//   const util::StatusOr<int> number = GetValueOrFail();
//
// The `util::StatusOr<T>` can never hold a value and a non-OK status at the
// same time. Assigning a value will clear the previously held status. It can
// not be initialized with an OK status, as an OK status indicates a value.
//
// When used in a function that returns `util::Status` or `util::StatusOr`, the
// helper macro ASSIGN_OR_RETURN can be used, which either assigns the value
// of the `util::StatusOr<T>`, or returns the error. For example:
//
//   util::Status DoSomeOperation() {
//     ASSIGN_OR_RETURN(const int number, GetValueOrFail());
//     return util::OkStatus();
//   }
//
// There are several options to access the value of `util::StatusOr<T>`. The
// value() family of functions will abort the program if the status is non-OK.
// Alternatively, `operator*` and `operator->` can be used to dereference or
// access members of the object.
template <typename T>
class [[nodiscard]] StatusOr {
 public:
  // Destructs the `StatusOr`. If it holds a value, T's destructor is called.
  ~StatusOr();

  // Constructs a `StatusOr` instance with an UNKNOWN status code.
  explicit StatusOr();

  // Constructs a `StatusOr` instance from a non-OK status code. If the passed
  // status is OK, then UNKNOWN is used instead as fallback.
  StatusOr(const Status& status);

  // Constructs a `StatusOr` from a value.
  StatusOr(const T& value);
  StatusOr(T&& value);

  // Copy and move construction and assignment.
  StatusOr(const StatusOr& other);
  StatusOr(StatusOr&& other) noexcept;
  StatusOr& operator=(const StatusOr& other);
  StatusOr& operator=(StatusOr&& other) noexcept;
  StatusOr& operator=(const T& value);
  StatusOr& operator=(T&& value);

  // Returns the status part. Returns an OK status if it has a value.
  const Status& status() const;

  // Returns true if the StatusOr contains a value.
  bool ok() const;

  // Returns the value part. Aborts the program if status is non-OK.
  const T& value() const&;
  T& value() &;
  T&& value() &&;

  // Access operators. Aborts the program if status is non-OK.
  T& operator*();
  const T& operator*() const;
  T* operator->();
  const T* operator->() const;

  // Ignores errors. Does nothing except suppress [[nodiscard]] errors.
  void IgnoreError() const;

  // Prevent construction, assignment, and taking from const rvalues. Instead,
  // the compiler will flag std::move operations on constant types. In these
  // cases the object is actually copied, not moved, thus inefficient. However,
  // this also does not allow returning, e.g., const int values. To get around
  // this, avoid const on return values, or static cast to const& explicitly.
  StatusOr(const T&& value) = delete;
  StatusOr(const StatusOr&& other) = delete;
  StatusOr& operator=(const StatusOr&& other) = delete;
  StatusOr& operator=(const T&& value) = delete;
  const T&& value() const&& = delete;

 private:
  void Construct(const T& value);
  void Construct(T&& value);
  void Destruct();

  Status status_;
  alignas(T) std::byte storage_[sizeof(T)];
  T* value_ptr_ = nullptr;
};

static_assert(std::is_move_constructible<StatusOr<int>>::value,
              "util::StatusOr must be move constructible");
static_assert(std::is_move_assignable<StatusOr<int>>::value,
              "util::StatusOr must be move assignable");

// Executes an expression `rexpr` that returns a StatusOr. If the status is OK,
// extracts the value into into the variable defined by `lhs`. If the status is
// not OK, returns the status. For example:
//
//   ASSIGN_OR_RETURN(const int value, MaybeGetIntValue());
//
// The current macro implementation leaves a moved-from StatusOr variable with
// a "random name" in the current scope.
#define ASSIGN_OR_RETURN(lhs, rexpr) \
  ASSIGN_OR_RETURN_IMPL(STATUSOR_CONCAT(_tmp, __COUNTER__), lhs, rexpr)

#define ASSIGN_OR_RETURN_IMPL(varname, lhs, rexpr) \
  auto varname = (rexpr);                          \
  RETURN_IF_ERROR(varname.status());               \
  lhs = std::move(varname).value()

// Executes an expression `rexpr` that returns a StatusOr. If the status is OK,
// extracts the value into into the variable defined by `lhs`. If the status is
// not OK, exits the program using EXIT_IF_ERROR.
#define ASSIGN_OR_EXIT(lhs, rexpr) \
  ASSIGN_OR_EXIT_IMPL(STATUSOR_CONCAT(_tmp, __COUNTER__), lhs, rexpr)

#define ASSIGN_OR_EXIT_IMPL(varname, lhs, rexpr) \
  auto varname = (rexpr);                        \
  EXIT_IF_ERROR(varname.status());               \
  lhs = std::move(varname).value()

#define STATUSOR_CONCAT_INNER(a, b) a##b
#define STATUSOR_CONCAT(a, b) STATUSOR_CONCAT_INNER(a, b)

// Internal functions.

void StatusOrRaiseBadAccess();

// Inline implementation.

template <typename T>
inline StatusOr<T>::~StatusOr() {
  Destruct();
}

template <typename T>
inline StatusOr<T>::StatusOr() : status_(StatusCode::UNKNOWN, "") {}

template <typename T>
inline StatusOr<T>::StatusOr(const Status& status) : status_(status) {
  if (ok()) {
    status_ = Status(StatusCode::UNKNOWN, "Invalid OK status for StatusOr");
  }
}

template <typename T>
inline StatusOr<T>::StatusOr(const T& value) {
  Construct(value);
}

template <typename T>
inline StatusOr<T>::StatusOr(T&& value) {
  Construct(std::move(value));
}

template <typename T>
inline StatusOr<T>::StatusOr(const StatusOr& other) : status_(other.status_) {
  if (!ok()) return;
  Construct(*other.value_ptr_);
}

template <typename T>
inline StatusOr<T>::StatusOr(StatusOr&& other) noexcept
    : status_(std::move(other.status_)) {
  if (!ok()) return;
  Construct(std::move(*other.value_ptr_));
}

template <typename T>
inline StatusOr<T>& StatusOr<T>::operator=(const StatusOr& other) {
  if (this == &other) return *this;
  Destruct();
  status_ = other.status_;
  if (ok()) Construct(*other.value_ptr_);
  return *this;
}

template <typename T>
inline StatusOr<T>& StatusOr<T>::operator=(StatusOr&& other) noexcept {
  if (this == &other) return *this;
  Destruct();
  status_ = std::move(other.status_);
  if (ok()) Construct(std::move(*other.value_ptr_));
  return *this;
}

template <typename T>
inline StatusOr<T>& StatusOr<T>::operator=(const T& value) {
  if (value_ptr_ == &value) return *this;
  Destruct();
  status_ = OkStatus();
  Construct(value);
  return *this;
}

template <typename T>
inline StatusOr<T>& StatusOr<T>::operator=(T&& value) {
  if (value_ptr_ == &value) return *this;
  Destruct();
  status_ = OkStatus();
  Construct(std::move(value));
  return *this;
}

template <typename T>
inline const Status& StatusOr<T>::status() const {
  return status_;
}

template <typename T>
inline bool StatusOr<T>::ok() const {
  return status_.ok();
}

template <typename T>
inline const T& StatusOr<T>::value() const& {
  if (!ok()) StatusOrRaiseBadAccess();
  return *value_ptr_;
}

template <typename T>
inline T& StatusOr<T>::value() & {
  if (!ok()) StatusOrRaiseBadAccess();
  return *value_ptr_;
}

template <typename T>
inline T&& StatusOr<T>::value() && {
  if (!ok()) StatusOrRaiseBadAccess();
  return std::move(*value_ptr_);
}

template <typename T>
inline T& StatusOr<T>::operator*() {
  return value();
}

template <typename T>
inline const T& StatusOr<T>::operator*() const {
  return value();
}

template <typename T>
inline T* StatusOr<T>::operator->() {
  return &value();
}

template <typename T>
inline const T* StatusOr<T>::operator->() const {
  return &value();
}

template <typename T>
inline void StatusOr<T>::IgnoreError() const {}

template <typename T>
inline void StatusOr<T>::Construct(const T& value) {
  new (&storage_) T(value);
  value_ptr_ = std::launder(reinterpret_cast<T*>(&storage_));
}

template <typename T>
inline void StatusOr<T>::Construct(T&& value) {
  new (&storage_) T(std::move(value));
  value_ptr_ = std::launder(reinterpret_cast<T*>(&storage_));
}

template <typename T>
inline void StatusOr<T>::Destruct() {
  if (!ok()) return;
  value_ptr_->~T();
  value_ptr_ = nullptr;
}

}  // namespace util

#endif  // SRC_UTIL_STATUS_OR_H_
