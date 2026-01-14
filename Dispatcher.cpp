#include "Dispatcher.hpp"

// Includes
#include <stdexcept>
#include <iostream>
#include <thread>
#include <sstream>
#include <iomanip>
#include <random>
#include <thread>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#if defined(__APPLE__) || defined(__MACOSX)
#include <machine/endian.h>
#else
#include <winsock2.h>
#endif

#include "precomp.hpp"

#ifndef htonll
#define htonll(x) ((((uint64_t)htonl(x)) << 32) | htonl((x) >> 32))
#endif

static std::string::size_type fromHex(char c) {
	if (c >= 'A' && c <= 'F') {
		c += 'a' - 'A';
	}

	const std::string hex = "0123456789abcdef";
	const std::string::size_type ret = hex.find(c);
	return ret;
}

static cl_ulong4 fromHex(const std::string & strHex) {
	uint8_t data[32];
	std::fill(data, data + sizeof(data), cl_uchar(0));

	auto index = 0;
	for(size_t i = 0; i < strHex.size(); i += 2) {
		const auto indexHi = fromHex(strHex[i]);
		const auto indexLo = i + 1 < strHex.size() ? fromHex(strHex[i+1]) : std::string::npos;

		const auto valHi = (indexHi == std::string::npos) ? 0 : indexHi << 4;
		const auto valLo = (indexLo == std::string::npos) ? 0 : indexLo;

		data[index] = valHi | valLo;
		++index;
	}

	cl_ulong4 res = {
		.s = {
			htonll(*(uint64_t *)(data + 24)),
			htonll(*(uint64_t *)(data + 16)),
			htonll(*(uint64_t *)(data + 8)),
			htonll(*(uint64_t *)(data + 0)),
		}
	};
	return res;
}

static std::string toHex(const uint8_t * const s, const size_t len) {
	std::string b("0123456789abcdef");
	std::string r;

	for (size_t i = 0; i < len; ++i) {
		const unsigned char h = s[i] / 16;
		const unsigned char l = s[i] % 16;

		r = r + b.substr(h, 1) + b.substr(l, 1);
	}

	return r;
}

struct Sha256State {
	uint32_t h[8];
	uint8_t buffer[64];
	uint64_t bitlen;
	size_t bufferLen;
};

static constexpr uint32_t sha256K[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static uint32_t rotr32(const uint32_t x, const uint32_t n) {
	return (x >> n) | (x << (32 - n));
}

static void sha256Init(Sha256State & state) {
	state.h[0] = 0x6a09e667;
	state.h[1] = 0xbb67ae85;
	state.h[2] = 0x3c6ef372;
	state.h[3] = 0xa54ff53a;
	state.h[4] = 0x510e527f;
	state.h[5] = 0x9b05688c;
	state.h[6] = 0x1f83d9ab;
	state.h[7] = 0x5be0cd19;
	state.bitlen = 0;
	state.bufferLen = 0;
}

static void sha256Transform(Sha256State & state, const uint8_t block[64]) {
	uint32_t w[64];
	for (size_t i = 0; i < 16; ++i) {
		w[i] = (static_cast<uint32_t>(block[i * 4]) << 24)
			| (static_cast<uint32_t>(block[i * 4 + 1]) << 16)
			| (static_cast<uint32_t>(block[i * 4 + 2]) << 8)
			| (static_cast<uint32_t>(block[i * 4 + 3]));
	}

	for (size_t i = 16; i < 64; ++i) {
		const uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
		const uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}

	uint32_t a = state.h[0];
	uint32_t b = state.h[1];
	uint32_t c = state.h[2];
	uint32_t d = state.h[3];
	uint32_t e = state.h[4];
	uint32_t f = state.h[5];
	uint32_t g = state.h[6];
	uint32_t h = state.h[7];

	for (size_t i = 0; i < 64; ++i) {
		const uint32_t s1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
		const uint32_t ch = (e & f) ^ (~e & g);
		const uint32_t temp1 = h + s1 + ch + sha256K[i] + w[i];
		const uint32_t s0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
		const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
		const uint32_t temp2 = s0 + maj;

		h = g;
		g = f;
		f = e;
		e = d + temp1;
		d = c;
		c = b;
		b = a;
		a = temp1 + temp2;
	}

	state.h[0] += a;
	state.h[1] += b;
	state.h[2] += c;
	state.h[3] += d;
	state.h[4] += e;
	state.h[5] += f;
	state.h[6] += g;
	state.h[7] += h;
}

static void sha256Update(Sha256State & state, const uint8_t * data, const size_t len) {
	for (size_t i = 0; i < len; ++i) {
		state.buffer[state.bufferLen++] = data[i];
		if (state.bufferLen == 64) {
			sha256Transform(state, state.buffer);
			state.bitlen += 512;
			state.bufferLen = 0;
		}
	}
}

static void sha256Final(Sha256State & state, uint8_t out[32]) {
	size_t i = state.bufferLen;
	state.buffer[i++] = 0x80;

	if (i > 56) {
		while (i < 64) {
			state.buffer[i++] = 0x00;
		}
		sha256Transform(state, state.buffer);
		i = 0;
	}

	while (i < 56) {
		state.buffer[i++] = 0x00;
	}

	state.bitlen += state.bufferLen * 8;
	const uint64_t bitlen = state.bitlen;
	for (size_t j = 0; j < 8; ++j) {
		state.buffer[63 - j] = static_cast<uint8_t>(bitlen >> (j * 8));
	}

	sha256Transform(state, state.buffer);

	for (size_t j = 0; j < 8; ++j) {
		out[j * 4] = static_cast<uint8_t>(state.h[j] >> 24);
		out[j * 4 + 1] = static_cast<uint8_t>(state.h[j] >> 16);
		out[j * 4 + 2] = static_cast<uint8_t>(state.h[j] >> 8);
		out[j * 4 + 3] = static_cast<uint8_t>(state.h[j]);
	}
}

static void sha256(const uint8_t * data, const size_t len, uint8_t out[32]) {
	Sha256State state;
	sha256Init(state);
	sha256Update(state, data, len);
	sha256Final(state, out);
}

static std::string base58Encode(const uint8_t * data, const size_t len) {
	static const char * const alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
	size_t zeros = 0;
	while (zeros < len && data[zeros] == 0) {
		++zeros;
	}

	std::vector<uint8_t> input(data, data + len);
	std::vector<char> encoded;

	size_t start = zeros;
	while (start < input.size()) {
		int carry = 0;
		for (size_t i = start; i < input.size(); ++i) {
			const int value = (carry << 8) + input[i];
			input[i] = static_cast<uint8_t>(value / 58);
			carry = value % 58;
		}
		encoded.push_back(alphabet[carry]);
		while (start < input.size() && input[start] == 0) {
			++start;
		}
	}

	std::string result(zeros, '1');
	for (auto it = encoded.rbegin(); it != encoded.rend(); ++it) {
		result.push_back(*it);
	}
	return result;
}

static std::string tronHexAddress(const uint8_t * const address20) {
	std::array<uint8_t, 21> raw{};
	raw[0] = 0x41;
	std::memcpy(raw.data() + 1, address20, 20);
	return toHex(raw.data(), raw.size());
}

static std::string tronBase58Address(const uint8_t * const address20) {
	std::array<uint8_t, 21> raw{};
	raw[0] = 0x41;
	std::memcpy(raw.data() + 1, address20, 20);

	uint8_t first[32];
	uint8_t second[32];
	sha256(raw.data(), raw.size(), first);
	sha256(first, sizeof(first), second);

	std::array<uint8_t, 25> payload{};
	std::memcpy(payload.data(), raw.data(), raw.size());
	std::memcpy(payload.data() + raw.size(), second, 4);
	return base58Encode(payload.data(), payload.size());
}

static void printResult(cl_ulong4 seed, cl_ulong round, result r, cl_uchar score, const std::chrono::time_point<std::chrono::steady_clock> & timeStart, const Mode & mode) {
	// Time delta
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - timeStart).count();

	// Format private key
	cl_ulong carry = 0;
	cl_ulong4 seedRes;

	seedRes.s[0] = seed.s[0] + round; carry = seedRes.s[0] < round;
	seedRes.s[1] = seed.s[1] + carry; carry = !seedRes.s[1];
	seedRes.s[2] = seed.s[2] + carry; carry = !seedRes.s[2];
	seedRes.s[3] = seed.s[3] + carry + r.foundId;

	std::ostringstream ss;
	ss << std::hex << std::setfill('0');
	ss << std::setw(16) << seedRes.s[3] << std::setw(16) << seedRes.s[2] << std::setw(16) << seedRes.s[1] << std::setw(16) << seedRes.s[0];
	const std::string strPrivate = ss.str();

	// Format public key
	const std::string strTronHex = tronHexAddress(r.foundHash);
	const std::string strTronBase58 = tronBase58Address(r.foundHash);

	// Print to screen
	const std::string strVT100ClearLine = "\33[2K\r";
	std::cout << strVT100ClearLine << "  Time: " << std::setw(5) << seconds << "s Score: " << std::setw(2) << (int) score << " Private: 0x" << strPrivate << ' ';

	std::cout << mode.transformName();
	std::cout << ": " << strTronBase58 << " (hex: 0x" << strTronHex << ")" << std::endl;

	// print to file
	std::ofstream outFile("output.txt", std::ios::app);
	outFile << "Time: " << std::setw(5) << seconds << "s Score: " << std::setw(2) << (int) score << " Private: 0x" << strPrivate << ' ';
	outFile << mode.transformName();
	outFile << ": " << strTronBase58 << " (hex: 0x" << strTronHex << ")" << std::endl;
	outFile.close();
}

unsigned int getKernelExecutionTimeMicros(cl_event & e) {
	cl_ulong timeStart = 0, timeEnd = 0;
	clWaitForEvents(1, &e);
	clGetEventProfilingInfo(e, CL_PROFILING_COMMAND_START, sizeof(timeStart), &timeStart, NULL);
	clGetEventProfilingInfo(e, CL_PROFILING_COMMAND_END, sizeof(timeEnd), &timeEnd, NULL);
	return (timeEnd - timeStart) / 1000;
}

Dispatcher::OpenCLException::OpenCLException(const std::string s, const cl_int res) :
	std::runtime_error( s + " (res = " + toString(res) + ")"),
	m_res(res)
{

}

void Dispatcher::OpenCLException::OpenCLException::throwIfError(const std::string s, const cl_int res) {
	if (res != CL_SUCCESS) {
		throw OpenCLException(s, res);
	}
}

cl_command_queue Dispatcher::Device::createQueue(cl_context & clContext, cl_device_id & clDeviceId) {
	// nVidia CUDA Toolkit 10.1 only supports OpenCL 1.2 so we revert back to older functions for compatability
#ifdef PROFANITY_DEBUG
	cl_command_queue_properties p = CL_QUEUE_PROFILING_ENABLE;
#else
	cl_command_queue_properties p = NULL;
#endif

#ifdef CL_VERSION_2_0
	const cl_command_queue ret = clCreateCommandQueueWithProperties(clContext, clDeviceId, &p, NULL);
#else
	const cl_command_queue ret = clCreateCommandQueue(clContext, clDeviceId, p, NULL);
#endif
	return ret == NULL ? throw std::runtime_error("failed to create command queue") : ret;
}

cl_kernel Dispatcher::Device::createKernel(cl_program & clProgram, const std::string s) {
	cl_kernel ret  = clCreateKernel(clProgram, s.c_str(), NULL);
	return ret == NULL ? throw std::runtime_error("failed to create kernel \"" + s + "\"") : ret;
}

cl_ulong4 Dispatcher::Device::createSeed() {
#ifdef PROFANITY_DEBUG
	cl_ulong4 r;
	r.s[0] = 1;
	r.s[1] = 1;
	r.s[2] = 1;
	r.s[3] = 1;
	return r;
#else
	// We do not need really safe crypto random here, since we inherit safety
	// of the key from the user-provided seed public key.
	// We only need this random to not repeat same job among different devices
	std::random_device rd;

	cl_ulong4 diff;
	diff.s[0] = (((uint64_t)rd()) << 32) | rd();
	diff.s[1] = (((uint64_t)rd()) << 32) | rd();
	diff.s[2] = (((uint64_t)rd()) << 32) | rd();
	diff.s[3] = (((uint64_t)rd() & 0x0000ffff) << 32) | rd(); // zeroing 2 highest bytes to prevent overflowing sum private key after adding to seed private key
	return diff;
#endif
}

Dispatcher::Device::Device(Dispatcher & parent, cl_context & clContext, cl_program & clProgram, cl_device_id clDeviceId, const size_t worksizeLocal, const size_t size, const size_t index, const Mode & mode, cl_ulong4 clSeedX, cl_ulong4 clSeedY) :
	m_parent(parent),
	m_index(index),
	m_clDeviceId(clDeviceId),
	m_worksizeLocal(worksizeLocal),
	m_clScoreMax(0),
	m_clQueue(createQueue(clContext, clDeviceId) ),
	m_kernelInit( createKernel(clProgram, "profanity_init") ),
	m_kernelInverse(createKernel(clProgram, "profanity_inverse")),
	m_kernelIterate(createKernel(clProgram, "profanity_iterate")),
	m_kernelTransform( mode.transformKernel() == "" ? NULL : createKernel(clProgram, mode.transformKernel())),
	m_kernelScore(createKernel(clProgram, mode.kernel)),
	m_memPrecomp(clContext, m_clQueue, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(g_precomp), g_precomp),
	m_memPointsDeltaX(clContext, m_clQueue, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, size, true),
	m_memInversedNegativeDoubleGy(clContext, m_clQueue, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, size, true),
	m_memPrevLambda(clContext, m_clQueue, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, size, true),
	m_memResult(clContext, m_clQueue, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, PROFANITY_MAX_SCORE + 1),
	m_memData1(clContext, m_clQueue, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, 20),
	m_memData2(clContext, m_clQueue, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, 20),
	m_clSeed(createSeed()),
	m_clSeedX(clSeedX),
	m_clSeedY(clSeedY),
	m_round(0),
	m_speed(PROFANITY_SPEEDSAMPLES),
	m_sizeInitialized(0),
	m_eventFinished(NULL)
{

}

Dispatcher::Device::~Device() {

}

Dispatcher::Dispatcher(cl_context & clContext, cl_program & clProgram, const Mode mode, const size_t worksizeMax, const size_t inverseSize, const size_t inverseMultiple, const cl_uchar clScoreQuit, const std::string & seedPublicKey)
	: m_clContext(clContext)
	, m_clProgram(clProgram)
	, m_mode(mode)
	, m_worksizeMax(worksizeMax)
	, m_inverseSize(inverseSize)
	, m_size(inverseSize*inverseMultiple)
	, m_clScoreMax(mode.score)
	, m_clScoreQuit(clScoreQuit)
	, m_eventFinished(NULL)
	, m_countPrint(0)
	, m_publicKeyX(fromHex(seedPublicKey.substr(0, 64)))
	, m_publicKeyY(fromHex(seedPublicKey.substr(64, 64)))
{
}

Dispatcher::~Dispatcher() {

}

void Dispatcher::addDevice(cl_device_id clDeviceId, const size_t worksizeLocal, const size_t index) {
	Device * pDevice = new Device(*this, m_clContext, m_clProgram, clDeviceId, worksizeLocal, m_size, index, m_mode, m_publicKeyX, m_publicKeyY);
	m_vDevices.push_back(pDevice);
}

void Dispatcher::run() {
	m_eventFinished = clCreateUserEvent(m_clContext, NULL);
	timeStart = std::chrono::steady_clock::now();

	init();

	const auto timeInitialization = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - timeStart).count();
	std::cout << "Initialization time: " << timeInitialization << " seconds" << std::endl;

	m_quit = false;
	m_countRunning = m_vDevices.size();

	std::cout << "Running..." << std::endl;
	std::cout << "  Always verify that a private key generated by this program corresponds to the" << std::endl;
	std::cout << "  public key printed by importing it to a wallet of your choice. This program" << std::endl;
	std::cout << "  like any software might contain bugs and it does by design cut corners to" << std::endl;
	std::cout << "  improve overall performance." << std::endl;
	std::cout << std::endl;

	for (auto it = m_vDevices.begin(); it != m_vDevices.end(); ++it) {
		dispatch(*(*it));
	}

	clWaitForEvents(1, &m_eventFinished);
	clReleaseEvent(m_eventFinished);
	m_eventFinished = NULL;
}

void Dispatcher::init() {
	std::cout << "Initializing devices..." << std::endl;
	std::cout << "  This should take less than a minute. The number of objects initialized on each" << std::endl;
	std::cout << "  device is equal to inverse-size * inverse-multiple. To lower" << std::endl;
	std::cout << "  initialization time (and memory footprint) I suggest lowering the" << std::endl;
	std::cout << "  inverse-multiple first. You can do this via the -I switch. Do note that" << std::endl;
	std::cout << "  this might negatively impact your performance." << std::endl;
	std::cout << std::endl;

	const auto deviceCount = m_vDevices.size();
	m_sizeInitTotal = m_size * deviceCount;
	m_sizeInitDone = 0;

	cl_event * const pInitEvents = new cl_event[deviceCount];

	for (size_t i = 0; i < deviceCount; ++i) {
		pInitEvents[i] = clCreateUserEvent(m_clContext, NULL);
		m_vDevices[i]->m_eventFinished = pInitEvents[i];
		initBegin(*m_vDevices[i]);
	}

	clWaitForEvents(deviceCount, pInitEvents);
	for (size_t i = 0; i < deviceCount; ++i) {
		m_vDevices[i]->m_eventFinished = NULL;
		clReleaseEvent(pInitEvents[i]);
	}

	delete[] pInitEvents;

	std::cout << std::endl;
}

void Dispatcher::initBegin(Device & d) {
	// Set mode data
	for (auto i = 0; i < 20; ++i) {
		d.m_memData1[i] = m_mode.data1[i];
		d.m_memData2[i] = m_mode.data2[i];
	}

	// Write precompute table and mode data
	d.m_memPrecomp.write(true);
	d.m_memData1.write(true);
	d.m_memData2.write(true);

	// Kernel arguments - profanity_begin
	d.m_memPrecomp.setKernelArg(d.m_kernelInit, 0);
	d.m_memPointsDeltaX.setKernelArg(d.m_kernelInit, 1);
	d.m_memPrevLambda.setKernelArg(d.m_kernelInit, 2);
	d.m_memResult.setKernelArg(d.m_kernelInit, 3);
	CLMemory<cl_ulong4>::setKernelArg(d.m_kernelInit, 4, d.m_clSeed);
	CLMemory<cl_ulong4>::setKernelArg(d.m_kernelInit, 5, d.m_clSeedX);
	CLMemory<cl_ulong4>::setKernelArg(d.m_kernelInit, 6, d.m_clSeedY);

	// Kernel arguments - profanity_inverse
	d.m_memPointsDeltaX.setKernelArg(d.m_kernelInverse, 0);
	d.m_memInversedNegativeDoubleGy.setKernelArg(d.m_kernelInverse, 1);

	// Kernel arguments - profanity_iterate
	d.m_memPointsDeltaX.setKernelArg(d.m_kernelIterate, 0);
	d.m_memInversedNegativeDoubleGy.setKernelArg(d.m_kernelIterate, 1);
	d.m_memPrevLambda.setKernelArg(d.m_kernelIterate, 2);

	// Kernel arguments - profanity_transform_*
	if(d.m_kernelTransform) {
		d.m_memInversedNegativeDoubleGy.setKernelArg(d.m_kernelTransform, 0);
	}

	// Kernel arguments - profanity_score_*
	d.m_memInversedNegativeDoubleGy.setKernelArg(d.m_kernelScore, 0);
	d.m_memResult.setKernelArg(d.m_kernelScore, 1);
	d.m_memData1.setKernelArg(d.m_kernelScore, 2);
	d.m_memData2.setKernelArg(d.m_kernelScore, 3);

	CLMemory<cl_uchar>::setKernelArg(d.m_kernelScore, 4, d.m_clScoreMax); // Updated in handleResult()

	// Seed device
	initContinue(d);
}

void Dispatcher::initContinue(Device & d) {
	size_t sizeLeft = m_size - d.m_sizeInitialized;
	const size_t sizeInitLimit = m_size / 20;

	// Print progress
	const size_t percentDone = m_sizeInitDone * 100 / m_sizeInitTotal;
	std::cout << "  " << percentDone << "%\r" << std::flush;

	if (sizeLeft) {
		cl_event event;
		const size_t sizeRun = std::min(sizeInitLimit, std::min(sizeLeft, m_worksizeMax));
		const auto resEnqueue = clEnqueueNDRangeKernel(d.m_clQueue, d.m_kernelInit, 1, &d.m_sizeInitialized, &sizeRun, NULL, 0, NULL, &event);
		OpenCLException::throwIfError("kernel queueing failed during initilization", resEnqueue);

		// See: https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clSetEventCallback.html
		// If an application needs to wait for completion of a routine from the above list in a callback, please use the non-blocking form of the function, and
		// assign a completion callback to it to do the remainder of your work. Note that when a callback (or other code) enqueues commands to a command-queue,
		// the commands are not required to begin execution until the queue is flushed. In standard usage, blocking enqueue calls serve this role by implicitly
		// flushing the queue. Since blocking calls are not permitted in callbacks, those callbacks that enqueue commands on a command queue should either call
		// clFlush on the queue before returning or arrange for clFlush to be called later on another thread.
		clFlush(d.m_clQueue); 

		std::lock_guard<std::mutex> lock(m_mutex);
		d.m_sizeInitialized += sizeRun;
		m_sizeInitDone += sizeRun;

		const auto resCallback = clSetEventCallback(event, CL_COMPLETE, staticCallback, &d);
		OpenCLException::throwIfError("failed to set custom callback during initialization", resCallback);
	} else {
		// Printing one whole string at once helps in avoiding garbled output when executed in parallell
		const std::string strOutput = "  GPU" + toString(d.m_index) + " initialized";
		std::cout << strOutput << std::endl;
		clSetUserEventStatus(d.m_eventFinished, CL_COMPLETE);
	}
}

void Dispatcher::enqueueKernel(cl_command_queue & clQueue, cl_kernel & clKernel, size_t worksizeGlobal, const size_t worksizeLocal, cl_event * pEvent = NULL) {
	const size_t worksizeMax = m_worksizeMax;
	size_t worksizeOffset = 0;
	while (worksizeGlobal) {
		const size_t worksizeRun = std::min(worksizeGlobal, worksizeMax);
		const size_t * const pWorksizeLocal = (worksizeLocal == 0 ? NULL : &worksizeLocal);
		const auto res = clEnqueueNDRangeKernel(clQueue, clKernel, 1, &worksizeOffset, &worksizeRun, pWorksizeLocal, 0, NULL, pEvent);
		OpenCLException::throwIfError("kernel queueing failed", res);

		worksizeGlobal -= worksizeRun;
		worksizeOffset += worksizeRun;
	}
}

void Dispatcher::enqueueKernelDevice(Device & d, cl_kernel & clKernel, size_t worksizeGlobal, cl_event * pEvent = NULL) {
	try {
		enqueueKernel(d.m_clQueue, clKernel, worksizeGlobal, d.m_worksizeLocal, pEvent);
	} catch ( OpenCLException & e ) {
		// If local work size is invalid, abandon it and let implementation decide
		if ((e.m_res == CL_INVALID_WORK_GROUP_SIZE || e.m_res == CL_INVALID_WORK_ITEM_SIZE) && d.m_worksizeLocal != 0) {
			std::cout << std::endl << "warning: local work size abandoned on GPU" << d.m_index << std::endl;
			d.m_worksizeLocal = 0;
			enqueueKernel(d.m_clQueue, clKernel, worksizeGlobal, d.m_worksizeLocal, pEvent);
		}
		else {
			throw;
		}
	}
}

void Dispatcher::dispatch(Device & d) {
	cl_event event;
	d.m_memResult.read(false, &event);

#ifdef PROFANITY_DEBUG
	cl_event eventInverse;
	cl_event eventIterate;

	enqueueKernelDevice(d, d.m_kernelInverse, m_size / m_inverseSize, &eventInverse);
	enqueueKernelDevice(d, d.m_kernelIterate, m_size, &eventIterate);
#else
	enqueueKernelDevice(d, d.m_kernelInverse, m_size / m_inverseSize);
	enqueueKernelDevice(d, d.m_kernelIterate, m_size);
#endif

	if (d.m_kernelTransform) {
		enqueueKernelDevice(d, d.m_kernelTransform, m_size);
	}

	enqueueKernelDevice(d, d.m_kernelScore, m_size);
	clFlush(d.m_clQueue);

#ifdef PROFANITY_DEBUG
	// We're actually not allowed to call clFinish here because this function is ultimately asynchronously called by OpenCL.
	// However, this happens to work on my computer and it's not really intended for release, just something to aid me in
	// optimizations.
	clFinish(d.m_clQueue); 
	std::cout << "Timing: profanity_inverse = " << getKernelExecutionTimeMicros(eventInverse) << "us, profanity_iterate = " << getKernelExecutionTimeMicros(eventIterate) << "us" << std::endl;
#endif

	const auto res = clSetEventCallback(event, CL_COMPLETE, staticCallback, &d);
	OpenCLException::throwIfError("failed to set custom callback", res);
}

void Dispatcher::handleResult(Device & d) {
	for (auto i = PROFANITY_MAX_SCORE; i > m_clScoreMax; --i) {
		result & r = d.m_memResult[i];

		if (r.found > 0 && i >= d.m_clScoreMax) {
			d.m_clScoreMax = i;
			CLMemory<cl_uchar>::setKernelArg(d.m_kernelScore, 4, d.m_clScoreMax);

			std::lock_guard<std::mutex> lock(m_mutex);
			if (i >= m_clScoreMax) {
				m_clScoreMax = i;

				if (m_clScoreQuit && i >= m_clScoreQuit) {
					m_quit = true;
				}

				printResult(d.m_clSeed, d.m_round, r, i, timeStart, m_mode);
			}

			break;
		}
	}
}

void Dispatcher::onEvent(cl_event event, cl_int status, Device & d) {
	if (status != CL_COMPLETE) {
		std::cout << "Dispatcher::onEvent - Got bad status: " << status << std::endl;
	}
	else if (d.m_eventFinished != NULL) {
		initContinue(d);
	} else {
		++d.m_round;
		handleResult(d);

		bool bDispatch = true;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			d.m_speed.sample(m_size);
			printSpeed();

			if( m_quit ) {
				bDispatch = false;
				if(--m_countRunning == 0) {
					clSetUserEventStatus(m_eventFinished, CL_COMPLETE);
				}
			}
		}

		if (bDispatch) {
			dispatch(d);
		}
	}
}

// This is run when m_mutex is held.
void Dispatcher::printSpeed() {
	++m_countPrint;
	if( m_countPrint > m_vDevices.size() ) {
		std::string strGPUs;
		double speedTotal = 0;
		unsigned int i = 0;
		for (auto & e : m_vDevices) {
			const auto curSpeed = e->m_speed.getSpeed();
			speedTotal += curSpeed;
			strGPUs += " GPU" + toString(e->m_index) + ": " + formatSpeed(curSpeed);
			++i;
		}

		//const std::string strVT100ClearLine = "\33[2K\r";
		//std::cerr << strVT100ClearLine << "Total: " << formatSpeed(speedTotal) << " -" << strGPUs << '\r' << std::flush;
		const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - timeStart).count();
		if (seconds % 15 == 0) {
			std::cerr << "Total: " << formatSpeed(speedTotal) << " -" << strGPUs << '\r' << std::flush << std::endl;
		}

		m_countPrint = 0;
	}
}

void CL_CALLBACK Dispatcher::staticCallback(cl_event event, cl_int event_command_exec_status, void * user_data) {
	Device * const pDevice = static_cast<Device *>(user_data);
	pDevice->m_parent.onEvent(event, event_command_exec_status, *pDevice);
	clReleaseEvent(event);
}

std::string Dispatcher::formatSpeed(double f) {
	const std::string S = " KMGT";

	unsigned int index = 0;
	while (f > 1000.0f && index < S.size()) {
		f /= 1000.0f;
		++index;
	}

	std::ostringstream ss;
	ss << std::fixed << std::setprecision(3) << (double)f << " " << S[index] << "H/s";
	return ss.str();
}
