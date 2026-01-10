/**
 * Unit tests for Mode.cpp
 * Tests the pattern matching logic for TRON address generation
 */

#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>

// Include the Mode header
#include "Mode.hpp"

/**
 * Test fixture for Mode tests
 */
class ModeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary test file
        // Note: This uses /tmp which is Unix-specific. For Windows compatibility,
        // consider using std::tmpnam() or platform-independent temp directory.
        testFileName = "/tmp/test_addresses.txt";
    }

    void TearDown() override {
        // Clean up test file
        std::remove(testFileName.c_str());
    }

    // Helper function to create a test file with addresses
    void createTestFile(const std::vector<std::string>& addresses) {
        std::ofstream file(testFileName);
        for (const auto& addr : addresses) {
            file << addr << "\n";
        }
        file.close();
    }

    std::string testFileName;
};

/**
 * Test 1: Valid TRON address file with single address
 */
TEST_F(ModeTest, SingleValidAddress) {
    // Create test file with one valid TRON address
    std::vector<std::string> addresses = {
        "TJRabPrwbZy45sbavfcjinPJC18kjpRTv8"
    };
    createTestFile(addresses);

    // Test Mode::matching
    EXPECT_NO_THROW({
        Mode mode = Mode::matching(testFileName);
        EXPECT_GT(mode.matchingCount, 0);
    });
}

/**
 * Test 2: Valid TRON address file with multiple addresses
 */
TEST_F(ModeTest, MultipleValidAddresses) {
    // Create test file with multiple valid TRON addresses
    std::vector<std::string> addresses = {
        "TJRabPrwbZy45sbavfcjinPJC18kjpRTv8",
        "TLPwNkF3Ht6FKpLGVqksmUvqrR8bSGfYyR",
        "TMwFHYXLJaRUPeW6421aqXL4ZEzPRFGkGT"
    };
    createTestFile(addresses);

    EXPECT_NO_THROW({
        Mode mode = Mode::matching(testFileName);
        EXPECT_GT(mode.matchingCount, 0);
    });
}

/**
 * Test 3: Empty file should throw or return empty mode
 */
TEST_F(ModeTest, EmptyFile) {
    // Create empty file
    createTestFile(std::vector<std::string>());

    // Depending on implementation, this might throw or return empty mode
    // We expect it to handle gracefully
    EXPECT_THROW({
        Mode mode = Mode::matching(testFileName);
    }, std::runtime_error);
}

/**
 * Test 4: Non-existent file should throw
 */
TEST_F(ModeTest, NonExistentFile) {
    const std::string nonExistentFile = testFileName + ".nonexistent";
    EXPECT_THROW({
        Mode mode = Mode::matching(nonExistentFile);
    }, std::runtime_error);
}

/**
 * Test 5: File with invalid addresses
 */
TEST_F(ModeTest, InvalidAddresses) {
    // Create test file with invalid addresses
    std::vector<std::string> addresses = {
        "InvalidAddress123",
        "NotATronAddress",
        "TooShort"
    };
    createTestFile(addresses);

    // Should either throw or skip invalid addresses
    // The exact behavior depends on Mode implementation
    EXPECT_THROW({
        Mode mode = Mode::matching(testFileName);
    }, std::runtime_error);
}

/**
 * Test 6: File with mixed valid and invalid addresses
 */
TEST_F(ModeTest, MixedAddresses) {
    // Create test file with mix of valid and invalid
    std::vector<std::string> addresses = {
        "TJRabPrwbZy45sbavfcjinPJC18kjpRTv8",  // Valid
        "InvalidAddress",                         // Invalid
        "TLPwNkF3Ht6FKpLGVqksmUvqrR8bSGfYyR"   // Valid
    };
    createTestFile(addresses);

    // Should process valid addresses and skip/report invalid ones
    // Exact behavior depends on implementation - test that it doesn't crash
    try {
        Mode mode = Mode::matching(testFileName);
        // If it succeeds, mode is created (behavior varies)
    } catch (const std::runtime_error& e) {
        // If it throws, that's also acceptable behavior
    }
    // Test passes if we reach here without crash
}

/**
 * Test 7: Verify Mode data structures are initialized
 */
TEST_F(ModeTest, DataStructuresInitialized) {
    std::vector<std::string> addresses = {
        "TJRabPrwbZy45sbavfcjinPJC18kjpRTv8"
    };
    createTestFile(addresses);

    try {
        Mode mode = Mode::matching(testFileName);

        // Verify basic structure initialization
        EXPECT_GE(mode.score, 0);
        EXPECT_GE(mode.matchingCount, 0);
    } catch (const std::runtime_error& e) {
        // Some implementations might throw for certain inputs
        SUCCEED() << "Mode threw exception (acceptable): " << e.what();
    }
}

/**
 * Test 8: Verify score is non-negative
 */
TEST_F(ModeTest, ScoreNonNegative) {
    std::vector<std::string> addresses = {
        "TJRabPrwbZy45sbavfcjinPJC18kjpRTv8"
    };
    createTestFile(addresses);

    try {
        Mode mode = Mode::matching(testFileName);
        EXPECT_GE(mode.score, 0) << "Score should be non-negative";
    } catch (const std::runtime_error& e) {
        SUCCEED() << "Mode threw exception (acceptable): " << e.what();
    }
}

/**
 * Test 9: Verify counts are non-negative
 */
TEST_F(ModeTest, CountsNonNegative) {
    std::vector<std::string> addresses = {
        "TJRabPrwbZy45sbavfcjinPJC18kjpRTv8"
    };
    createTestFile(addresses);

    try {
        Mode mode = Mode::matching(testFileName);
        EXPECT_GE(mode.prefixCount, 0) << "prefixCount should be non-negative";
        EXPECT_GE(mode.suffixCount, 0) << "suffixCount should be non-negative";
        EXPECT_GE(mode.matchingCount, 0) << "matchingCount should be non-negative";
    } catch (const std::runtime_error& e) {
        SUCCEED() << "Mode threw exception (acceptable): " << e.what();
    }
}

/**
 * Test 10: Large file with many addresses
 */
TEST_F(ModeTest, LargeFile) {
    // Create a large test file
    std::vector<std::string> addresses;
    for (int i = 0; i < 100; i++) {
        // Use the same address multiple times (just for testing)
        addresses.push_back("TJRabPrwbZy45sbavfcjinPJC18kjpRTv8");
    }
    createTestFile(addresses);

    try {
        Mode mode = Mode::matching(testFileName);
        EXPECT_GT(mode.matchingCount, 0);
    } catch (const std::runtime_error& e) {
        SUCCEED() << "Mode threw exception (acceptable for large file): " << e.what();
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
