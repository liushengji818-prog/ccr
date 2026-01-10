#ifndef HPP_SPEEDSAMPLE
#define HPP_SPEEDSAMPLE

#include <chrono>
#include <stdexcept>
#include <vector>

class SpeedSample {
private:
	std::vector<std::chrono::time_point<std::chrono::steady_clock>> m_samples;
	std::vector<size_t> m_sizes;
	size_t m_maxSamples;
	size_t m_currentIndex;
	size_t m_totalSize;

	size_t getOldestIndex() const {
		if (m_samples.size() < m_maxSamples) {
			return 0;
		}
		return m_currentIndex;
	}

	size_t getNewestIndex() const {
		const size_t numSamples = m_samples.size();
		if (numSamples < m_maxSamples) {
			return numSamples - 1;
		}
		// When buffer is full, newest is just before current index
		return (m_currentIndex + m_maxSamples - 1) % m_maxSamples;
	}

public:
	SpeedSample(const size_t maxSamples)
		: m_maxSamples(maxSamples),
		  m_currentIndex(0),
		  m_totalSize(0) {
		if (m_maxSamples == 0) {
			throw std::invalid_argument("maxSamples must be greater than zero");
		}
		m_samples.reserve(maxSamples);
		m_sizes.reserve(maxSamples);
	}

	// Default timestamp is captured at the call site to avoid duplicate clock queries.
	void sample(const size_t size) {
		sample(size, std::chrono::steady_clock::now());
	}

	void sample(const size_t size, const std::chrono::time_point<std::chrono::steady_clock>& now) {
		if (m_samples.size() < m_maxSamples) {
			m_samples.push_back(now);
			m_sizes.push_back(size);
			m_totalSize += size;
		} else {
			m_totalSize -= m_sizes[m_currentIndex];
			m_samples[m_currentIndex] = now;
			m_sizes[m_currentIndex] = size;
			m_totalSize += size;
			m_currentIndex = (m_currentIndex + 1) % m_maxSamples;
		}
	}

	double getSpeed() const {
		if (m_samples.size() < 2) {
			return 0.0;
		}

		const size_t oldestIndex = getOldestIndex();
		const size_t newestIndex = getNewestIndex();

		const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
			m_samples[newestIndex] - m_samples[oldestIndex]).count();

		if (duration == 0) {
			return 0.0;
		}

		// Return hashes per second
		return (m_totalSize * 1000.0) / duration;
	}
};

#endif /* HPP_SPEEDSAMPLE */
