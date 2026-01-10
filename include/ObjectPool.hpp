#ifndef HPP_OBJECTPOOL
#define HPP_OBJECTPOOL

#include <vector>
#include <cstddef>
#include <stdexcept>

/**
 * @brief A simple object pool for efficient memory allocation and reuse
 *
 * This object pool pre-allocates a fixed number of objects and provides
 * fast allocation and deallocation without going to the heap for each request.
 *
 * Features:
 * - Pre-allocation of PoolSize objects at construction
 * - O(1) allocation and deallocation
 * - Thread-unsafe (use external synchronization if needed)
 * - Useful for frequently allocated/deallocated objects
 *
 * Usage:
 *   ObjectPool<MyClass, 1024> pool;
 *   MyClass* obj = pool.allocate();
 *   // ... use obj ...
 *   pool.deallocate(obj);
 */
template<typename T, size_t PoolSize = 1024>
class ObjectPool {
public:
    /**
     * @brief Construct the object pool and pre-allocate all objects
     */
    ObjectPool() : m_nextFree(0) {
        // Pre-allocate storage for all objects
        m_storage.reserve(PoolSize);
        for (size_t i = 0; i < PoolSize; ++i) {
            m_storage.push_back(new T());
            m_freeList.push_back(m_storage[i]);
        }
    }

    /**
     * @brief Destructor - frees all allocated objects
     */
    ~ObjectPool() {
        for (typename std::vector<T*>::iterator it = m_storage.begin(); it != m_storage.end(); ++it) {
            delete *it;
        }
    }

    /**
     * @brief Allocate an object from the pool
     * @return Pointer to an object, or nullptr if pool is exhausted
     *
     * WARNING: Always check the returned pointer for nullptr before use!
     * Example:
     *   MyClass* obj = pool.allocate();
     *   if (obj == nullptr) {
     *       // Handle pool exhaustion
     *       return;
     *   }
     *   // Safe to use obj here
     */
    T* allocate() {
        if (m_nextFree >= m_freeList.size()) {
            return nullptr; // Pool exhausted
        }
        return m_freeList[m_nextFree++];
    }

    /**
     * @brief Return an object to the pool
     * @param obj Pointer to the object to return
     *
     * WARNING: Only pass pointers that were allocated from this pool.
     * This method includes basic validation to prevent corruption.
     *
     * Note: This does not call the destructor - the object is reused as-is.
     * If you need cleanup, call it manually before deallocating.
     *
     * @throws std::runtime_error if the pool is already full or if the pointer
     *         doesn't belong to this pool's storage
     */
    void deallocate(T* obj) {
        if (obj == nullptr) {
            return;
        }

        if (m_nextFree == 0) {
            throw std::runtime_error("ObjectPool: Cannot deallocate to full pool");
        }

#ifndef NDEBUG
        // Validate that the pointer belongs to this pool's storage (debug builds only)
        // Note: This is O(n) but only enabled in debug mode for safety validation
        bool found = false;
        for (typename std::vector<T*>::const_iterator it = m_storage.begin();
             it != m_storage.end(); ++it) {
            if (*it == obj) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("ObjectPool: Pointer does not belong to this pool");
        }
#endif

        m_freeList[--m_nextFree] = obj;
    }

    /**
     * @brief Get the number of available objects in the pool
     */
    size_t available() const {
        return m_freeList.size() - m_nextFree;
    }

    /**
     * @brief Get the total pool size
     */
    size_t size() const {
        return PoolSize;
    }

    /**
     * @brief Check if the pool is empty (all objects allocated)
     */
    bool empty() const {
        return m_nextFree >= m_freeList.size();
    }

    /**
     * @brief Check if the pool is full (all objects available)
     */
    bool full() const {
        return m_nextFree == 0;
    }

    /**
     * @brief Reset the pool to initial state
     *
     * WARNING: This does not call destructors. Make sure all objects
     * are properly cleaned up before calling reset().
     */
    void reset() {
        m_nextFree = 0;
    }

private:
    // Disable copy and assignment
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    std::vector<T*> m_storage;   // Owns all allocated objects
    std::vector<T*> m_freeList;  // Tracks available objects
    size_t m_nextFree;           // Index of next free object
};

#endif /* HPP_OBJECTPOOL */
