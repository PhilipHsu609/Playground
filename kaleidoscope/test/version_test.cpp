#include "kaleidoscope/version.hpp"

#include <gtest/gtest.h>

TEST(Version, IsNonEmpty) { EXPECT_FALSE(kaleidoscope::version().empty()); }
