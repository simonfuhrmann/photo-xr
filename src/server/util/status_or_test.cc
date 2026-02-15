#include "src/server/util/status_or.h"

#include <vector>

#include "src/server/test/tinytest.h"
#include "src/server/util/status.h"

namespace util {

// Tracks how often Tester copies, moves, etc.
struct UsageStats {
  int num_copies = 0;
  int num_moves = 0;
  int num_ctors = 0;
  int num_dtors = 0;
};

// Test object used inside StatusOr.
struct Tester {
  Tester(UsageStats* stats) : stats_(stats) { stats_->num_ctors += 1; }
  ~Tester() { stats_->num_dtors += 1; }

  Tester(const Tester& other) : stats_(other.stats_) {
    stats_->num_copies += 1;
  }
  Tester(Tester&& other) noexcept : stats_(other.stats_) {
    stats_->num_moves += 1;
  }

  Tester& operator=(const Tester& other) {
    stats_ = other.stats_;
    stats_->num_copies += 1;
    return *this;
  }
  Tester& operator=(Tester&& other) noexcept {
    stats_ = other.stats_;
    stats_->num_moves += 1;
    return *this;
  }

  int GetNumCtors() const { return stats_->num_ctors; }

  UsageStats* stats_ = nullptr;
};

// Functions to test ASSIGN_OR_RETURN macro.
StatusOr<Tester> GetTester(UsageStats* stats) { return Tester(stats); }
StatusOr<Tester> GetError() { return InternalError("test"); }

Status AssignOrReturnError(UsageStats* stats) {
  ASSIGN_OR_RETURN(const Tester tester1, GetTester(stats));
  ASSIGN_OR_RETURN(const Tester tester2, GetError());
  return OkStatus();
}

Status AssignOrReturnSuccess(UsageStats* stats) {
  ASSIGN_OR_RETURN(const Tester tester1, GetTester(stats));
  ASSIGN_OR_RETURN(const Tester tester2, GetTester(stats));
  return OkStatus();
}

TEST(StatusOrTest, Construction) {
  // Default constructor creates UNKNOWN status code.
  {
    StatusOr<int> status_or;
    EXPECT_NOT_OK(status_or);
    EXPECT_EQ(status_or.status(), UnknownError(""));
  }

  // Explicit initialization from a status.
  {
    StatusOr<int> status_or(InternalError("test"));
    EXPECT_NOT_OK(status_or);
    EXPECT_EQ(status_or.status(), InternalError("test"));
  }

  // Initialization from an OK status is invalid use.
  {
    StatusOr<int> status_or(OkStatus());
    EXPECT_NOT_OK(status_or);
    EXPECT_EQ(status_or.status(), UnknownError(""));
  }

  // Initialization from value.
  {
    StatusOr<int> status_or(1);
    EXPECT_OK(status_or);
    EXPECT_EQ(status_or.status(), OkStatus());
    EXPECT_EQ(status_or.value(), 1);
    EXPECT_EQ(*status_or, 1);
  }

  // Initialization from copied value.
  {
    UsageStats stats;
    Tester obj(&stats);
    StatusOr<Tester> status_or(obj);
    EXPECT_EQ(status_or->GetNumCtors(), 1);
    EXPECT_EQ(stats.num_ctors, 1);
    EXPECT_EQ(stats.num_copies, 1);
    EXPECT_EQ(stats.num_moves, 0);
  }

  // Initialization from moved value.
  {
    UsageStats stats;
    Tester obj(&stats);
    StatusOr<Tester> status_or(std::move(obj));
    EXPECT_EQ(stats.num_ctors, 1);
    EXPECT_EQ(stats.num_copies, 0);
    EXPECT_EQ(stats.num_moves, 1);
  }

  // Initialization from moved value, moving StatusOr.
  {
    UsageStats stats;
    Tester obj(&stats);
    StatusOr<Tester> status_or1(std::move(obj));
    StatusOr<Tester> status_or2 = std::move(status_or1);
    EXPECT_EQ(stats.num_ctors, 1);
    EXPECT_EQ(stats.num_copies, 0);
    EXPECT_EQ(stats.num_moves, 2);

    StatusOr<Tester> status_or3 = status_or2;
    EXPECT_EQ(stats.num_ctors, 1);
    EXPECT_EQ(stats.num_copies, 1);
    EXPECT_EQ(stats.num_moves, 2);

    EXPECT_OK(status_or3);
    EXPECT_EQ(status_or3.status(), OkStatus());
  }

  // Value management calls destructors properly.
  {
    UsageStats stats;
    Tester obj1(&stats);
    Tester obj2(&stats);
    {
      StatusOr<Tester> status_or;
      status_or = std::move(obj1);
      EXPECT_EQ(stats.num_moves, 1);
      EXPECT_EQ(stats.num_dtors, 0);
      status_or = std::move(obj2);
      EXPECT_EQ(stats.num_moves, 2);
      EXPECT_EQ(stats.num_dtors, 1);
    }
    EXPECT_EQ(stats.num_ctors, 2);
    EXPECT_EQ(stats.num_copies, 0);
    EXPECT_EQ(stats.num_dtors, 2);
  }
}

TEST(StatusOrTest, AssignOrReturn) {
  {
    UsageStats stats;
    const Status status = AssignOrReturnError(&stats);
    EXPECT_NOT_OK(status);
    EXPECT_EQ(stats.num_ctors, 1);
  }

  {
    UsageStats stats;
    const Status status = AssignOrReturnSuccess(&stats);
    EXPECT_OK(status);
    EXPECT_EQ(stats.num_ctors, 2);
  }
}

TEST(StatusOrTest, AssignOrExit) {
  // Test the program does not exit on Ok status.
  UsageStats stats;
  ASSIGN_OR_EXIT(const Tester tester1, GetTester(&stats));
  EXPECT_EQ(stats.num_ctors, 1);
}

}  // namespace util
