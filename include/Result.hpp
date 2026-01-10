#ifndef HPP_RESULT
#define HPP_RESULT

#include <string>
#include <stdexcept>

/**
 * @brief Result type for unified error handling (C++11 compatible)
 *
 * This is a simplified implementation of a Result/Expected type that allows
 * functions to return either a value or an error without using exceptions
 * for error propagation (though accessors may throw if misused).
 *
 * Usage:
 *   Result<int> divide(int a, int b) {
 *       if (b == 0) return Result<int>::Err("Division by zero");
 *       return Result<int>::Ok(a / b);
 *   }
 *
 *   auto result = divide(10, 2);
 *   if (result.is_ok()) {
 *       std::cout << "Result: " << result.value() << std::endl;
 *   } else {
 *       std::cerr << "Error: " << result.error() << std::endl;
 *   }
 *
 * Note: The value() and error() methods throw exceptions if called on the
 * wrong variant. Use is_ok()/is_err() to check first, or use value_or()
 * for a safe non-throwing alternative.
 */
template<typename T, typename E = std::string>
class Result {
public:
    /**
     * @brief Create a successful result with a value
     */
    static Result Ok(const T& val) {
        Result r;
        r.m_isOk = true;
        r.m_value = val;
        return r;
    }

    /**
     * @brief Create an error result with an error value
     */
    static Result Err(const E& err) {
        Result r;
        r.m_isOk = false;
        r.m_error = err;
        return r;
    }

    /**
     * @brief Check if the result is Ok
     */
    bool is_ok() const {
        return m_isOk;
    }

    /**
     * @brief Check if the result is Err
     */
    bool is_err() const {
        return !m_isOk;
    }

    /**
     * @brief Get the value (throws if result is Err)
     * @throws std::runtime_error if called on an Err result
     *
     * WARNING: Always check is_ok() before calling value(), or use value_or()
     */
    T& value() {
        if (!m_isOk) {
            throw std::runtime_error("Called value() on an Err result");
        }
        return m_value;
    }

    /**
     * @brief Get the value (const version, throws if result is Err)
     * @throws std::runtime_error if called on an Err result
     *
     * WARNING: Always check is_ok() before calling value(), or use value_or()
     */
    const T& value() const {
        if (!m_isOk) {
            throw std::runtime_error("Called value() on an Err result");
        }
        return m_value;
    }

    /**
     * @brief Get the error (throws if result is Ok)
     * @throws std::runtime_error if called on an Ok result
     *
     * WARNING: Always check is_err() before calling error()
     */
    E& error() {
        if (m_isOk) {
            throw std::runtime_error("Called error() on an Ok result");
        }
        return m_error;
    }

    /**
     * @brief Get the error (const version, throws if result is Ok)
     * @throws std::runtime_error if called on an Ok result
     *
     * WARNING: Always check is_err() before calling error()
     */
    const E& error() const {
        if (m_isOk) {
            throw std::runtime_error("Called error() on an Ok result");
        }
        return m_error;
    }

    /**
     * @brief Get the value or a default value if Err (non-throwing)
     *
     * This is the safe alternative to value() that never throws.
     */
    T value_or(const T& default_value) const {
        return m_isOk ? m_value : default_value;
    }

private:
    Result() : m_isOk(false), m_value(), m_error() {}

    bool m_isOk;
    T m_value;
    E m_error;
};

#endif /* HPP_RESULT */
