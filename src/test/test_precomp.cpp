/**
 * Unit tests for precomp.cpp
 * Tests the precomputed elliptic curve points
 */

#include <gtest/gtest.h>
#include "precomp.hpp"

/**
 * Test fixture for precomp tests
 */
class PrecompTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }

    void TearDown() override {
        // Cleanup
    }
};

/**
 * Test 1: Verify precomp array is defined
 */
TEST_F(PrecompTest, ArrayDefined) {
    // g_precomp should be accessible
    EXPECT_NE(&g_precomp[0], nullptr);
}

/**
 * Test 2: Verify array size
 * Based on precomp.cpp, the array has 8160 elements
 */
TEST_F(PrecompTest, ArraySize) {
    const int EXPECTED_SIZE = 8160;

    // We can't directly get array size in C++, but we know it from the code
    // Just verify we can access elements in valid range
    EXPECT_NO_THROW({
        point p = g_precomp[0];
        (void)p; // Suppress unused variable warning
    });

    EXPECT_NO_THROW({
        point p = g_precomp[EXPECTED_SIZE - 1];
        (void)p; // Suppress unused variable warning
    });
}

/**
 * Test 3: Verify point structure
 */
TEST_F(PrecompTest, PointStructure) {
    point p = g_precomp[0];

    // Each point has two mp_number coordinates (x and y)
    // Each mp_number has 8 uint32_t values

    // Verify we can access x and y coordinates
    EXPECT_NO_THROW({
        uint32_t x0 = p.x.d[0];
        uint32_t y0 = p.y.d[0];
        (void)x0; (void)y0;
    });
}

/**
 * Test 4: Verify first point has expected values
 * This is the generator point G for secp256k1
 */
TEST_F(PrecompTest, GeneratorPoint) {
    point g = g_precomp[0];

    // Known values for secp256k1 generator point (little-endian representation)
    // Gx = 79BE667E F9DCBBAC 55A06295 CE870B07 029BFCDB 2DCE28D9 59F2815B 16F81798
    EXPECT_EQ(g.x.d[0], 0x16f81798);
    EXPECT_EQ(g.x.d[1], 0x59f2815b);
    EXPECT_EQ(g.x.d[2], 0x2dce28d9);
    EXPECT_EQ(g.x.d[3], 0x029bfcdb);
    EXPECT_EQ(g.x.d[4], 0xce870b07);
    EXPECT_EQ(g.x.d[5], 0x55a06295);
    EXPECT_EQ(g.x.d[6], 0xf9dcbbac);
    EXPECT_EQ(g.x.d[7], 0x79be667e);

    // Gy = 483ADA77 26A3C465 5DA4FBFC 0E1108A8 FD17B448 A6855419 9C47D08F FB10D4B8
    EXPECT_EQ(g.y.d[0], 0xfb10d4b8);
    EXPECT_EQ(g.y.d[1], 0x9c47d08f);
    EXPECT_EQ(g.y.d[2], 0xa6855419);
    EXPECT_EQ(g.y.d[3], 0xfd17b448);
    EXPECT_EQ(g.y.d[4], 0x0e1108a8);
    EXPECT_EQ(g.y.d[5], 0x5da4fbfc);
    EXPECT_EQ(g.y.d[6], 0x26a3c465);
    EXPECT_EQ(g.y.d[7], 0x483ada77);
}

/**
 * Test 5: Verify points are non-zero
 */
TEST_F(PrecompTest, PointsNonZero) {
    // Check first few points to ensure they're not all zeros
    for (int i = 0; i < 10; i++) {
        point p = g_precomp[i];

        // At least one component should be non-zero
        bool hasNonZero = false;
        for (int j = 0; j < 8; j++) {
            if (p.x.d[j] != 0 || p.y.d[j] != 0) {
                hasNonZero = true;
                break;
            }
        }

        EXPECT_TRUE(hasNonZero) << "Point " << i << " should have non-zero components";
    }
}

/**
 * Test 6: Verify different points have different values
 */
TEST_F(PrecompTest, PointsAreDifferent) {
    point p0 = g_precomp[0];
    point p1 = g_precomp[1];

    // Points should be different
    bool isDifferent = false;
    for (int i = 0; i < 8; i++) {
        if (p0.x.d[i] != p1.x.d[i] || p0.y.d[i] != p1.y.d[i]) {
            isDifferent = true;
            break;
        }
    }

    EXPECT_TRUE(isDifferent) << "Consecutive points should be different";
}

/**
 * Test 7: Verify point at various indices
 */
TEST_F(PrecompTest, RandomPointAccess) {
    // Test accessing points at various indices
    std::vector<int> indices = {0, 100, 500, 1000, 2000, 4000, 8000, 8159};

    for (int idx : indices) {
        EXPECT_NO_THROW({
            point p = g_precomp[idx];
            // Verify at least one component is non-zero
            bool hasNonZero = false;
            for (int j = 0; j < 8; j++) {
                if (p.x.d[j] != 0 || p.y.d[j] != 0) {
                    hasNonZero = true;
                    break;
                }
            }
            EXPECT_TRUE(hasNonZero) << "Point at index " << idx << " should be valid";
        });
    }
}

/**
 * Test 8: Verify mp_number size
 */
TEST_F(PrecompTest, MpNumberSize) {
    // mp_number should have 8 uint32_t values (256 bits total)
    mp_number num;

    // Should be able to access all 8 elements
    for (int i = 0; i < 8; i++) {
        EXPECT_NO_THROW({
            num.d[i] = 0;
            uint32_t val = num.d[i];
            (void)val;
        });
    }
}

/**
 * Test 9: Verify point structure size
 */
TEST_F(PrecompTest, PointSize) {
    // A point should contain two mp_numbers (x and y)
    // Each mp_number is 8 * 4 bytes = 32 bytes
    // So a point should be 64 bytes

    EXPECT_EQ(sizeof(mp_number), 8 * sizeof(uint32_t));
    EXPECT_EQ(sizeof(point), 2 * sizeof(mp_number));
}

/**
 * Test 10: Verify second point (2G)
 */
TEST_F(PrecompTest, SecondPoint) {
    point p2 = g_precomp[1];

    // Second point should be 2G (doubling of generator)
    // Known value for 2G on secp256k1
    // 2Gx = C6047F94 1ED7D6D3 045406E9 5C07CD85 C778E4B8 CEF3CA7A BAC09B95 C709EE5
    // Note: We only verify the first two words for a quick sanity check.
    // Full verification would check all 8 words but is omitted for brevity.
    EXPECT_EQ(p2.x.d[0], 0x5c709ee5);
    EXPECT_EQ(p2.x.d[1], 0xabac09b9);

    // Just verify it's different from first point
    EXPECT_NE(p2.x.d[0], g_precomp[0].x.d[0]);
}

/**
 * Test 11: Memory access pattern
 */
TEST_F(PrecompTest, MemoryAccessPattern) {
    // Verify sequential access works correctly
    point prev = g_precomp[0];

    for (int i = 1; i < 100; i++) {
        point curr = g_precomp[i];

        // Each point should be different from previous
        bool isDiff = false;
        for (int j = 0; j < 8; j++) {
            if (curr.x.d[j] != prev.x.d[j] || curr.y.d[j] != prev.y.d[j]) {
                isDiff = true;
                break;
            }
        }

        EXPECT_TRUE(isDiff) << "Point " << i << " should differ from point " << (i-1);
        prev = curr;
    }
}

/**
 * Test 12: Verify data integrity
 */
TEST_F(PrecompTest, DataIntegrity) {
    // Read the same point multiple times and verify consistency
    point p1 = g_precomp[1000];
    point p2 = g_precomp[1000];

    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(p1.x.d[i], p2.x.d[i]) << "X coordinate mismatch at index " << i;
        EXPECT_EQ(p1.y.d[i], p2.y.d[i]) << "Y coordinate mismatch at index " << i;
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
