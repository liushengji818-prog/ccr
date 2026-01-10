/**
 * Input Validation Functions
 *
 * This file contains validation functions for command-line arguments.
 * Previously contained additional unused utility functions that have been removed.
 */

#ifndef IMPROVEMENTS_HPP
#define IMPROVEMENTS_HPP

#include <iostream>
#include <cstddef>

namespace Improvements {

    // Constants
    const size_t MAX_SCORE = 120;              // Maximum score for quit count
    const size_t MAX_ADDRESS_LENGTH = 34;      // TRON address length

    // Validate quit count parameter
    // Note: Always returns true because any quit-count is valid (0 = unlimited)
    // Just warns if value exceeds maximum possible score
    inline bool validateQuitCount(size_t quitCount) {
        // Allow 0 (unlimited) to MAX_SCORE
        if (quitCount > MAX_SCORE) {
            std::cerr << "warning: quit-count exceeds max score (" << MAX_SCORE
                      << "), will exit immediately" << std::endl;
        }
        return true;
    }

    // Validate prefix/suffix count parameters
    inline bool validateMatchingCounts(size_t prefixCount, size_t suffixCount) {
        if (prefixCount > MAX_ADDRESS_LENGTH) {
            std::cerr << "error: prefix-count cannot exceed " << MAX_ADDRESS_LENGTH << std::endl;
            return false;
        }

        if (suffixCount > MAX_ADDRESS_LENGTH) {
            std::cerr << "error: suffix-count cannot exceed " << MAX_ADDRESS_LENGTH << std::endl;
            return false;
        }

        if (prefixCount == 0 && suffixCount == 0) {
            std::cerr << "warning: both prefix-count and suffix-count are 0, will match all addresses" << std::endl;
        }

        return true;
    }

}  // namespace Improvements

#endif // IMPROVEMENTS_HPP
