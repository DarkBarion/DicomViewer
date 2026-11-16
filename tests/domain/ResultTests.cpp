#include "domain/Result.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace domain::test {

  TEST(Result, OkCarriesValue) {
    auto result = ResultOk<int>(42);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 42);
  }

  TEST(Result, FailCarriesError) {
    auto result = ResultFail<int>("boom");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message, "boom");
  }

  TEST(Result, ValueAccessOnFailureThrows) {
    auto result = ResultFail<int>("boom");
    EXPECT_THROW(result.value(), std::bad_expected_access<Error>);
  }

  TEST(Result, MoveConstructionPreservesState) {
    auto source = ResultOk<std::string>(std::string("hello"));
    auto destination = std::move(source);
    EXPECT_EQ(destination.value(), "hello");
  }

  TEST(Result, BoolConversionMatchesHasValue) {
    EXPECT_TRUE(static_cast<bool>(ResultOk<int>(0)));
    EXPECT_FALSE(static_cast<bool>(ResultFail<int>("x")));
  }

  TEST(Result, WorksWithComplexTypes) {
    auto result = ResultOk<std::vector<std::uint8_t>>({1, 2, 3});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 3u);
    EXPECT_EQ(result.value()[1], 2);
  }

  TEST(Result, ErrorMessagePreservedThroughMove) {
    auto result = ResultFail<int>("specific error message");
    auto moved = std::move(result);
    EXPECT_EQ(moved.error().message, "specific error message");
  }

  TEST(Result, StacktraceCaptured) {
    auto result = ResultFail<int>("test trace");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message, "test trace");
    EXPECT_FALSE(result.error().trace.empty());
  }

  TEST(Status, SuccessIsTruthy) {
    auto status = Status::Success();
    EXPECT_TRUE(status);
    EXPECT_TRUE(status.ok);
    EXPECT_TRUE(status.error.empty());
  }

  TEST(Status, FailCarriesMessage) {
    auto status = Status::Fail("bad thing");
    EXPECT_FALSE(status);
    EXPECT_EQ(status.error, "bad thing");
  }

}  // namespace domain::test
