#include <algorithm>
#include <array>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <map>
#include <set>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#include <OpenCL/cl_ext.h> // Included to get topology to get an actual unique identifier per device
#else
#include <CL/cl.h>
#include <CL/cl_ext.h> // Included to get topology to get an actual unique identifier per device
#endif

#include "Dispatcher.hpp"
#include "ArgParser.hpp"
#include "Mode.hpp"
#include "help.hpp"
#include "kernel_profanity.hpp"
#include "kernel_sha256.hpp"
#include "kernel_keccak.hpp"
#include "IMPROVEMENTS.hpp"

// Constants
// Define NVIDIA-specific OpenCL extensions if not already defined
#ifndef CL_DEVICE_PCI_BUS_ID_NV
#define CL_DEVICE_PCI_BUS_ID_NV 0x4008
#endif
#ifndef CL_DEVICE_PCI_SLOT_ID_NV
#define CL_DEVICE_PCI_SLOT_ID_NV 0x4009
#endif

constexpr size_t MAX_MATCHING_FILE_SIZE = 10UL * 1024 * 1024;  // 10MB limit
constexpr size_t MAX_OUTPUT_FILE_SIZE = 100UL * 1024 * 1024;   // 100MB limit

std::string readFile(const char *const szFilename)
{
	std::ifstream inputFile(szFilename, std::ios::in | std::ios::binary);
	std::ostringstream contents;
	contents << inputFile.rdbuf();
	return contents.str();
}

std::vector<cl_device_id> getAllDevices(cl_device_type deviceType = CL_DEVICE_TYPE_GPU)
{
	std::vector<cl_device_id> vDevices;

	cl_uint platformIdCount = 0;
	clGetPlatformIDs(0, nullptr, &platformIdCount);

	std::vector<cl_platform_id> platformIds(platformIdCount);
	clGetPlatformIDs(platformIdCount, platformIds.data(), nullptr);

	for (const auto& platform : platformIds)
	{
		cl_uint countDevice = 0;
		if (clGetDeviceIDs(platform, deviceType, 0, nullptr, &countDevice) != CL_SUCCESS) {
            continue;
        }

		std::vector<cl_device_id> deviceIds(countDevice);
		if (clGetDeviceIDs(platform, deviceType, countDevice, deviceIds.data(), &countDevice) != CL_SUCCESS) {
            continue;
        }

		std::copy(deviceIds.begin(), deviceIds.end(), std::back_inserter(vDevices));
	}

	return vDevices;
}

template <typename T, typename U, typename V, typename W>
T clGetWrapper(U function, V param, W param2)
{
	T result{};
	function(param, param2, sizeof(result), &result, nullptr);
	return result;
}

template <typename U, typename V, typename W>
std::string clGetWrapperString(U function, V param, W param2)
{
	size_t len = 0;
	function(param, param2, 0, nullptr, &len);
	if (len <= 1)
	{
		// If there is no data or only a null terminator, return an empty string
		return std::string();
	}
	std::vector<char> buffer(len);
	function(param, param2, len, buffer.data(), nullptr);
	return {buffer.begin(), buffer.end() - 1}; // -1 to exclude null terminator
}

template <typename T, typename U, typename V, typename W>
std::vector<T> clGetWrapperVector(U function, V param, W param2)
{
	size_t len = 0;
	function(param, param2, 0, nullptr, &len);
	len /= sizeof(T);
	std::vector<T> result;
	if (len > 0)
	{
		result.resize(len);
		function(param, param2, len * sizeof(T), result.data(), nullptr);
	}
	return result;
}

std::vector<std::string> getBinaries(cl_program &clProgram)
{
	std::vector<std::string> vReturn;
	auto vSizes = clGetWrapperVector<size_t>(clGetProgramInfo, clProgram, CL_PROGRAM_BINARY_SIZES);
	if (!vSizes.empty())
	{
		std::vector<std::vector<unsigned char>> buffers(vSizes.size());
		std::vector<unsigned char *> pBuffers(vSizes.size());

		for (size_t i = 0; i < vSizes.size(); ++i)
		{
			buffers[i].resize(vSizes[i]);
			pBuffers[i] = buffers[i].data();
		}

		clGetProgramInfo(clProgram, CL_PROGRAM_BINARIES, vSizes.size() * sizeof(unsigned char *), pBuffers.data(), nullptr);

		for (size_t i = 0; i < vSizes.size(); ++i)
		{
			vReturn.emplace_back(reinterpret_cast<char *>(buffers[i].data()), vSizes[i]);
		}
	}

	return vReturn;
}

unsigned int getUniqueDeviceIdentifier(const cl_device_id &deviceId)
{
#if defined(CL_DEVICE_TOPOLOGY_AMD)
	auto topology = clGetWrapper<cl_device_topology_amd>(clGetDeviceInfo, deviceId, CL_DEVICE_TOPOLOGY_AMD);
	if (topology.raw.type == CL_DEVICE_TOPOLOGY_TYPE_PCIE_AMD)
	{
		return (topology.pcie.bus << 16) + (topology.pcie.device << 8) + topology.pcie.function;
	}
#endif
	auto bus_id = clGetWrapper<cl_int>(clGetDeviceInfo, deviceId, CL_DEVICE_PCI_BUS_ID_NV);
	auto slot_id = clGetWrapper<cl_int>(clGetDeviceInfo, deviceId, CL_DEVICE_PCI_SLOT_ID_NV);
	return (bus_id << 16) + slot_id;
}

template <typename T>
bool printResult(const T &value, const cl_int &err)
{
	std::cout << ((value == nullptr) ? toString(err) : "Done") << '\n';
	return value == nullptr;
}

bool printResult(const cl_int err)
{
	std::cout << ((err != CL_SUCCESS) ? toString(err) : "Done") << '\n';
	return err != CL_SUCCESS;
}

std::string getDeviceCacheFilename(cl_device_id &device, const size_t &inverseSize)
{
	const auto uniqueId = getUniqueDeviceIdentifier(device);
	return "cache-opencl." + toString(inverseSize) + "." + toString(uniqueId);
}

int main(int argc, char **argv)
{
	try
	{
		ArgParser argp(argc, argv);
		bool bHelp = false;

		std::string matchingInput;
		std::string outputFile;
		// localhost test post url
		std::string postUrl = "http://127.0.0.1:7002/api/address";
		std::vector<size_t> vDeviceSkipIndex;
		size_t worksizeLocal = 64;
		size_t worksizeMax = 0;
		bool bNoCache = false;
		size_t inverseSize = 255;
		size_t inverseMultiple = 16384;
		size_t prefixCount = 0;
		size_t suffixCount = 6;
		size_t quitCount = 0;

		argp.addSwitch('h', "help", bHelp);
		argp.addSwitch('m', "matching", matchingInput);
		argp.addSwitch('w', "work", worksizeLocal);
		argp.addSwitch('W', "work-max", worksizeMax);
		argp.addSwitch('n', "no-cache", bNoCache);
		argp.addSwitch('o', "output", outputFile);
		argp.addSwitch('p', "post", postUrl);
		argp.addSwitch('i', "inverse-size", inverseSize);
		argp.addSwitch('I', "inverse-multiple", inverseMultiple);
		argp.addSwitch('b', "prefix-count", prefixCount);
		argp.addSwitch('e', "suffix-count", suffixCount);
		argp.addSwitch('q', "quit-count", quitCount);
		argp.addMultiSwitch('s', "skip", vDeviceSkipIndex);

		if (!argp.parse())
		{
			std::cout << "error: bad arguments, try again :<" << '\n';
			return 1;
		}

		if (bHelp)
		{
			std::cout << g_strHelp << '\n';
			return 0;
		}

		if (matchingInput.empty())
		{
			std::cout << "error: matching file must be specified :<" << '\n';
			return 1;
		}

		// Parameter validation - using improved validation functions
		if (!Improvements::validateMatchingCounts(prefixCount, suffixCount))
		{
			return 1;
		}

		if (!Improvements::validateQuitCount(quitCount))
		{
			// validateQuitCount only issues warnings, does not interrupt execution
		}

		// Note: prefixCount and suffixCount are size_t (unsigned); basic validation,
		// including checks for exceeding MAX_ADDRESS_LENGTH and the case where both are zero,
		// is performed in validateMatchingCounts.

		if (prefixCount > 34)
		{
			std::cout << "error: the number of prefix matches cannot be greater than 34 :<" << '\n';
			return 1;
		}

		if (suffixCount > 34)
		{
			std::cout << "error: the number of suffix matches cannot be greater than 34 :<" << '\n';
			return 1;
		}

		// Check file size - prevent malicious input
		std::ifstream matchFile(matchingInput, std::ios::ate | std::ios::binary);
		if (matchFile.is_open())
		{
			const std::streamsize fileSize = matchFile.tellg();
			if (fileSize > static_cast<std::streamsize>(MAX_MATCHING_FILE_SIZE))
			{
				std::cout << "error: matching file is too large (" << fileSize << " > "
						  << MAX_MATCHING_FILE_SIZE << " bytes)" << '\n';
				return 1;
			}
			matchFile.close();
		}

		Mode mode = Mode::matching(matchingInput);

		if (mode.matchingCount <= 0)
		{
			std::cout << "error: please check your matching file to make sure the path and format are correct :<" << '\n';
			return 1;
		}

		mode.prefixCount = prefixCount;
		mode.suffixCount = suffixCount;

		std::vector<cl_device_id> vFoundDevices = getAllDevices();
		std::vector<cl_device_id> vDevices;
		std::map<cl_device_id, size_t> mDeviceIndex;

		std::vector<std::string> vDeviceBinary;
		std::vector<size_t> vDeviceBinarySize;
		cl_int errorCode = CL_SUCCESS;
		bool bUsedCache = false;

		std::cout << "Devices:" << std::endl;
		for (size_t i = 0; i < vFoundDevices.size(); ++i)
		{
			if (std::find(vDeviceSkipIndex.begin(), vDeviceSkipIndex.end(), i) != vDeviceSkipIndex.end())
			{
				continue;
			}
			cl_device_id &deviceId = vFoundDevices[i];
			const auto strName = clGetWrapperString(clGetDeviceInfo, deviceId, CL_DEVICE_NAME);
			const auto computeUnits = clGetWrapper<cl_uint>(clGetDeviceInfo, deviceId, CL_DEVICE_MAX_COMPUTE_UNITS);
			const auto globalMemSize = clGetWrapper<cl_ulong>(clGetDeviceInfo, deviceId, CL_DEVICE_GLOBAL_MEM_SIZE);
			bool precompiled = false;

			if (!bNoCache)
			{
				std::ifstream fileIn(getDeviceCacheFilename(deviceId, inverseSize), std::ios::binary);
				if (fileIn.is_open())
				{
					vDeviceBinary.emplace_back((std::istreambuf_iterator<char>(fileIn)), std::istreambuf_iterator<char>());
					vDeviceBinarySize.push_back(vDeviceBinary.back().size());
					precompiled = true;
				}
			}

			std::cout << "  GPU-" << i << ": " << strName << ", " << globalMemSize << " bytes available, " << computeUnits << " compute units (precompiled = " << (precompiled ? "yes" : "no") << ")" << '\n';
			vDevices.push_back(vFoundDevices[i]);
			mDeviceIndex[vFoundDevices[i]] = i;
		}

		if (vDevices.empty())
		{
			return 1;
		}

		std::cout << '\n';
		std::cout << "OpenCL:" << '\n';
		std::cout << "  Context creating ..." << std::flush;
		auto *clContext = clCreateContext(nullptr, vDevices.size(), vDevices.data(), nullptr, nullptr, &errorCode);
		if (printResult(clContext, errorCode))
		{
			return 1;
		}

		cl_program clProgram = nullptr;
		if (vDeviceBinary.size() == vDevices.size())
		{
			// Create program from binaries
			bUsedCache = true;

			std::cout << "  Binary kernel loading..." << std::flush;
			std::vector<const unsigned char *> pKernels(vDevices.size());
			for (size_t i = 0; i < vDeviceBinary.size(); ++i)
			{
				pKernels[i] = reinterpret_cast<const unsigned char *>(vDeviceBinary[i].data());
			}

			std::vector<cl_int> pStatus(vDevices.size());

			clProgram = clCreateProgramWithBinary(clContext, vDevices.size(), vDevices.data(), vDeviceBinarySize.data(), pKernels.data(), pStatus.data(), &errorCode);
			if (printResult(clProgram, errorCode))
			{
				return 1;
			}
		}
		else
		{
			// Create a program from the kernel source
			std::cout << "  Kernel compiling ..." << std::flush;

			// const std::string strKeccak = readFile("keccak.cl");
			// const std::string strSha256 = readFile("sha256.cl");
			// const std::string strVanity = readFile("profanity.cl");
			// std::array<const char *, 3> szKernels = {strKeccak.c_str(), strSha256.c_str(), strVanity.c_str()};

			std::array<const char *, 3> szKernels = {kernel_keccak.c_str(), kernel_sha256.c_str(), kernel_profanity.c_str()};
			clProgram = clCreateProgramWithSource(clContext, szKernels.size(), szKernels.data(), nullptr, &errorCode);
			if (printResult(clProgram, errorCode))
			{
				return 1;
			}
		}

		// Build the program
		std::cout << "  Program building ..." << std::flush;
		const std::string strBuildOptions = "-D PROFANITY_INVERSE_SIZE=" + toString(inverseSize) + " -D PROFANITY_MAX_SCORE=" + toString(PROFANITY_MAX_SCORE);
		if (printResult(clBuildProgram(clProgram, vDevices.size(), vDevices.data(), strBuildOptions.c_str(), nullptr, nullptr)))
		{
			return 1;
		}

		// Save binary to improve future start times
		if (!bUsedCache && !bNoCache)
		{
			std::cout << "  Program saving ..." << std::flush;
			auto binaries = getBinaries(clProgram);
			for (size_t i = 0; i < binaries.size(); ++i)
			{
				std::ofstream fileOut(getDeviceCacheFilename(vDevices[i], inverseSize), std::ios::binary);
				fileOut.write(binaries[i].data(), binaries[i].size());
			}
			std::cout << "Done" << '\n';
		}

		std::cout << '\n';

		Dispatcher dispatcher(clContext, clProgram, mode, worksizeMax == 0 ? inverseSize * inverseMultiple : worksizeMax, inverseSize, inverseMultiple, quitCount, outputFile, postUrl);

		for (auto &device : vDevices)
		{
			dispatcher.addDevice(device, worksizeLocal, mDeviceIndex[device]);
		}

		dispatcher.run();
		clReleaseContext(clContext);
		return 0;
	}
	catch (const std::runtime_error &e)
	{
		std::cout << "std::runtime_error - " << e.what() << '\n';
	}
	catch (...)
	{
		std::cout << "unknown exception occurred" << '\n';
	}

	return 1;
}
