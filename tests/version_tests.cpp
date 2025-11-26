#include "hotplugpp/i_plugin.hpp"

#include <gtest/gtest.h>

namespace hotplugpp {
namespace tests {

// ============================================================================
// Constructor Tests
// ============================================================================

TEST(VersionTest, DefaultConstructor) {
    Version v;
    EXPECT_EQ(v.major, 1);
    EXPECT_EQ(v.minor, 0);
    EXPECT_EQ(v.patch, 0);
}

TEST(VersionTest, ParameterizedConstructor) {
    Version v(2, 3, 4);
    EXPECT_EQ(v.major, 2);
    EXPECT_EQ(v.minor, 3);
    EXPECT_EQ(v.patch, 4);
}

TEST(VersionTest, ConstructorWithMajorOnly) {
    Version v(5);
    EXPECT_EQ(v.major, 5);
    EXPECT_EQ(v.minor, 0);
    EXPECT_EQ(v.patch, 0);
}

TEST(VersionTest, ConstructorWithMajorAndMinor) {
    Version v(5, 6);
    EXPECT_EQ(v.major, 5);
    EXPECT_EQ(v.minor, 6);
    EXPECT_EQ(v.patch, 0);
}

TEST(VersionTest, ZeroVersion) {
    Version v(0, 0, 0);
    EXPECT_EQ(v.major, 0);
    EXPECT_EQ(v.minor, 0);
    EXPECT_EQ(v.patch, 0);
}

TEST(VersionTest, LargeVersionNumbers) {
    Version v(999, 999, 999);
    EXPECT_EQ(v.major, 999);
    EXPECT_EQ(v.minor, 999);
    EXPECT_EQ(v.patch, 999);
}

// ============================================================================
// toString Tests
// ============================================================================

TEST(VersionTest, ToStringDefault) {
    Version v;
    EXPECT_EQ(v.toString(), "1.0.0");
}

TEST(VersionTest, ToStringCustomVersion) {
    Version v(2, 3, 4);
    EXPECT_EQ(v.toString(), "2.3.4");
}

TEST(VersionTest, ToStringZeroVersion) {
    Version v(0, 0, 0);
    EXPECT_EQ(v.toString(), "0.0.0");
}

TEST(VersionTest, ToStringLargeNumbers) {
    Version v(100, 200, 300);
    EXPECT_EQ(v.toString(), "100.200.300");
}

// ============================================================================
// isCompatible Tests
// ============================================================================

TEST(VersionTest, IsCompatibleSameVersion) {
    Version v1(1, 2, 3);
    Version v2(1, 2, 3);
    EXPECT_TRUE(v1.isCompatible(v2));
}

TEST(VersionTest, IsCompatibleHigherMinor) {
    Version v1(1, 3, 0);  // Has higher minor
    Version v2(1, 2, 0);  // Required version
    EXPECT_TRUE(v1.isCompatible(v2));
}

TEST(VersionTest, IsCompatibleLowerMinor) {
    Version v1(1, 1, 0);  // Has lower minor
    Version v2(1, 2, 0);  // Required version
    EXPECT_FALSE(v1.isCompatible(v2));
}

TEST(VersionTest, IsCompatibleDifferentMajor) {
    Version v1(2, 0, 0);
    Version v2(1, 0, 0);
    EXPECT_FALSE(v1.isCompatible(v2));
}

TEST(VersionTest, IsCompatibleSameMajorEqualMinor) {
    Version v1(1, 5, 0);
    Version v2(1, 5, 10);
    EXPECT_TRUE(v1.isCompatible(v2));
}

TEST(VersionTest, IsCompatiblePatchIgnored) {
    Version v1(1, 2, 0);
    Version v2(1, 2, 999);
    EXPECT_TRUE(v1.isCompatible(v2));
}

// ============================================================================
// Equality Operators Tests
// ============================================================================

TEST(VersionTest, EqualityOperatorEqual) {
    Version v1(1, 2, 3);
    Version v2(1, 2, 3);
    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 != v2);
}

TEST(VersionTest, EqualityOperatorDifferentMajor) {
    Version v1(1, 2, 3);
    Version v2(2, 2, 3);
    EXPECT_FALSE(v1 == v2);
    EXPECT_TRUE(v1 != v2);
}

TEST(VersionTest, EqualityOperatorDifferentMinor) {
    Version v1(1, 2, 3);
    Version v2(1, 3, 3);
    EXPECT_FALSE(v1 == v2);
    EXPECT_TRUE(v1 != v2);
}

TEST(VersionTest, EqualityOperatorDifferentPatch) {
    Version v1(1, 2, 3);
    Version v2(1, 2, 4);
    EXPECT_FALSE(v1 == v2);
    EXPECT_TRUE(v1 != v2);
}

// ============================================================================
// Less Than Operator Tests
// ============================================================================

TEST(VersionTest, LessThanMajor) {
    Version v1(1, 0, 0);
    Version v2(2, 0, 0);
    EXPECT_TRUE(v1 < v2);
    EXPECT_FALSE(v2 < v1);
}

TEST(VersionTest, LessThanMinor) {
    Version v1(1, 1, 0);
    Version v2(1, 2, 0);
    EXPECT_TRUE(v1 < v2);
    EXPECT_FALSE(v2 < v1);
}

TEST(VersionTest, LessThanPatch) {
    Version v1(1, 2, 1);
    Version v2(1, 2, 2);
    EXPECT_TRUE(v1 < v2);
    EXPECT_FALSE(v2 < v1);
}

TEST(VersionTest, LessThanEqual) {
    Version v1(1, 2, 3);
    Version v2(1, 2, 3);
    EXPECT_FALSE(v1 < v2);
    EXPECT_FALSE(v2 < v1);
}

TEST(VersionTest, LessThanMajorTakesPriority) {
    Version v1(1, 9, 9);
    Version v2(2, 0, 0);
    EXPECT_TRUE(v1 < v2);
}

TEST(VersionTest, LessThanMinorTakesPriorityOverPatch) {
    Version v1(1, 1, 9);
    Version v2(1, 2, 0);
    EXPECT_TRUE(v1 < v2);
}

// ============================================================================
// Greater Than Operator Tests
// ============================================================================

TEST(VersionTest, GreaterThanMajor) {
    Version v1(2, 0, 0);
    Version v2(1, 0, 0);
    EXPECT_TRUE(v1 > v2);
    EXPECT_FALSE(v2 > v1);
}

TEST(VersionTest, GreaterThanMinor) {
    Version v1(1, 2, 0);
    Version v2(1, 1, 0);
    EXPECT_TRUE(v1 > v2);
    EXPECT_FALSE(v2 > v1);
}

TEST(VersionTest, GreaterThanPatch) {
    Version v1(1, 2, 2);
    Version v2(1, 2, 1);
    EXPECT_TRUE(v1 > v2);
    EXPECT_FALSE(v2 > v1);
}

TEST(VersionTest, GreaterThanEqual) {
    Version v1(1, 2, 3);
    Version v2(1, 2, 3);
    EXPECT_FALSE(v1 > v2);
    EXPECT_FALSE(v2 > v1);
}

// ============================================================================
// Less Than Or Equal Operator Tests
// ============================================================================

TEST(VersionTest, LessThanOrEqualLess) {
    Version v1(1, 0, 0);
    Version v2(2, 0, 0);
    EXPECT_TRUE(v1 <= v2);
    EXPECT_FALSE(v2 <= v1);
}

TEST(VersionTest, LessThanOrEqualEqual) {
    Version v1(1, 2, 3);
    Version v2(1, 2, 3);
    EXPECT_TRUE(v1 <= v2);
    EXPECT_TRUE(v2 <= v1);
}

TEST(VersionTest, LessThanOrEqualGreater) {
    Version v1(2, 0, 0);
    Version v2(1, 0, 0);
    EXPECT_FALSE(v1 <= v2);
}

// ============================================================================
// Greater Than Or Equal Operator Tests
// ============================================================================

TEST(VersionTest, GreaterThanOrEqualGreater) {
    Version v1(2, 0, 0);
    Version v2(1, 0, 0);
    EXPECT_TRUE(v1 >= v2);
    EXPECT_FALSE(v2 >= v1);
}

TEST(VersionTest, GreaterThanOrEqualEqual) {
    Version v1(1, 2, 3);
    Version v2(1, 2, 3);
    EXPECT_TRUE(v1 >= v2);
    EXPECT_TRUE(v2 >= v1);
}

TEST(VersionTest, GreaterThanOrEqualLess) {
    Version v1(1, 0, 0);
    Version v2(2, 0, 0);
    EXPECT_FALSE(v1 >= v2);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(VersionTest, ComparisonWithZeroVersion) {
    Version v1(0, 0, 0);
    Version v2(0, 0, 1);
    EXPECT_TRUE(v1 < v2);
    EXPECT_TRUE(v2 > v1);
    EXPECT_FALSE(v1 == v2);
}

TEST(VersionTest, MaxUint32Version) {
    Version v(UINT32_MAX, UINT32_MAX, UINT32_MAX);
    EXPECT_EQ(v.major, UINT32_MAX);
    EXPECT_EQ(v.minor, UINT32_MAX);
    EXPECT_EQ(v.patch, UINT32_MAX);
}

TEST(VersionTest, CopySemantics) {
    Version v1(1, 2, 3);
    Version v2 = v1;
    EXPECT_EQ(v1, v2);
    
    v2.patch = 4;
    EXPECT_NE(v1, v2);
}

} // namespace tests
} // namespace hotplugpp
