#include "kvstore/version.hpp"

#include <gtest/gtest.h>

TEST(Version, IsNonEmpty) { EXPECT_FALSE(kvstore::version().empty()); }
