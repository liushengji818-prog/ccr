#ifndef HPP_MODE
#define HPP_MODE

#include <string>
#include <vector>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

class Mode {
	private:
		Mode();

	public:
		static Mode matching(const std::string& matchingInput);

		std::vector<cl_uchar> data1;
		std::vector<cl_uchar> data2;
		cl_uchar score{0};
		cl_uchar prefixCount{0};
		cl_uchar suffixCount{0};
		cl_uchar matchingCount{0};
};

#endif /* HPP_MODE */
