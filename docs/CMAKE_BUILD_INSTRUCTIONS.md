# CMake Build Instructions

This document describes how to build the project using CMake as an alternative to the traditional Makefile.

## Prerequisites

### Required Dependencies
- **C++ Compiler**: g++ or clang with C++11 support
- **CMake**: Version 3.10 or higher
- **OpenCL**: OpenCL development files
  - Ubuntu/Debian: `sudo apt-get install ocl-icd-opencl-dev opencl-headers`
  - NVIDIA GPU: Install CUDA toolkit
  - AMD GPU: Install ROCm or AMDGPU-Pro
- **cURL**: libcurl development files
  - Ubuntu/Debian: `sudo apt-get install libcurl4-openssl-dev`

### Optional
- **Ninja**: For faster builds (optional, can use default make)
  - Ubuntu/Debian: `sudo apt-get install ninja-build`

## Quick Start

### 1. Using Makefile (Original Method)
```bash
# Release build
make clean
make

# Test build
make test

# Clean
make clean
```

### 2. Using CMake (New Method)

#### Release Build (Default)
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

#### Debug Build
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

#### Using Ninja (Faster)
```bash
mkdir build
cd build
cmake -G Ninja ..
ninja
```

## Build Configurations

### Release Mode
- Optimized for performance (`-O2`)
- Symbols stripped on Linux (`-s`)
- Compiler warnings enabled (`-Wall`)
- **Same as Makefile default behavior**

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### Debug Mode
- Debug symbols included (`-g`)
- No optimization (`-O0`)
- Compiler warnings enabled (`-Wall`)
- Better for debugging with gdb/lldb

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## Build Targets

### Main Executable: `profanity.x64`
```bash
# Build main program
make profanity.x64

# Or just
make
```

Equivalent to Makefile sources:
- Dispatcher.cpp
- Mode.cpp
- precomp.cpp
- profanity.cpp

### Test Executable: `test_address_validity`
```bash
# Build test program
make test_address_validity

# Or use the test_build target
make test_build
```

Equivalent to Makefile test target.

### All Targets
```bash
# Build everything
make all
```

### Clean
```bash
# Remove build artifacts
make clean

# For complete cleanup, remove build directory
cd ..
rm -rf build
```

## Platform-Specific Notes

### macOS
- OpenCL is provided as a system framework
- CMake automatically uses `-framework OpenCL`
- No additional OpenCL installation needed

### Linux
- Requires OpenCL development files
- CMake first tries system OpenCL, then falls back to local headers in `./OpenCL/include`
- If OpenCL library not found, build will show warnings but continue (will fail at link time)

## Advanced Usage

### Specify Custom Compiler
```bash
cmake -DCMAKE_CXX_COMPILER=g++-11 ..
```

### Verbose Build
```bash
make VERBOSE=1
```

### Parallel Build
```bash
# Use all CPU cores
make -j$(nproc)

# Or specify number of jobs
make -j4
```

### Out-of-Source Build (Recommended)
```bash
# Keep source directory clean
mkdir build
cd build
cmake ..
make
```

### Install
```bash
# Install to system (default: /usr/local/bin)
sudo make install

# Or specify custom prefix
cmake -DCMAKE_INSTALL_PREFIX=/opt/profanity ..
make install
```

## Troubleshooting

### OpenCL Not Found
If you see warnings about OpenCL not found:

1. **Check if OpenCL is installed:**
   ```bash
   ldconfig -p | grep OpenCL
   ```

2. **Install OpenCL development files:**
   - Ubuntu/Debian: `sudo apt-get install ocl-icd-opencl-dev`
   - NVIDIA: Install CUDA toolkit
   - AMD: Install ROCm

3. **Verify installation:**
   ```bash
   pkg-config --modversion OpenCL
   ```

### cURL Not Found
If you see warnings about cURL not found:

1. **Install cURL development files:**
   ```bash
   sudo apt-get install libcurl4-openssl-dev
   ```

2. **Verify installation:**
   ```bash
   curl-config --version
   ```

### CMake Version Too Old
If CMake version is too old:

```bash
# Check version
cmake --version

# Ubuntu: Install from Kitware's repository for latest version
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt-get update
sudo apt-get install cmake
```

## Comparison: Makefile vs CMake

| Feature | Makefile | CMake |
|---------|----------|-------|
| **Build Command** | `make` | `cmake .. && make` |
| **Clean** | `make clean` | `make clean` or `rm -rf build/` |
| **Release Build** | Default | `cmake -DCMAKE_BUILD_TYPE=Release ..` |
| **Debug Build** | Not supported | `cmake -DCMAKE_BUILD_TYPE=Debug ..` |
| **Platform Detection** | Manual (`uname -s`) | Automatic |
| **Dependency Detection** | Manual | Automatic (find_package) |
| **Out-of-Source Build** | No | Yes (recommended) |
| **IDE Support** | Limited | Excellent (CLion, VS Code, etc.) |
| **Parallel Build** | Manual (`-j`) | Automatic detection |

## Benefits of CMake

1. **Better Dependency Detection**: Automatically finds OpenCL and cURL
2. **Debug Support**: Easy to build with debug symbols
3. **IDE Integration**: Works with CLion, Visual Studio Code, etc.
4. **Cross-Platform**: Better handling of macOS vs Linux differences
5. **Out-of-Source Builds**: Keeps source directory clean
6. **Modern Build System**: Industry standard for C++ projects

## Migration Notes

- **No Changes Required**: The CMakeLists.txt preserves all Makefile behavior
- **Source Files**: Identical to Makefile (Dispatcher.cpp, Mode.cpp, precomp.cpp, profanity.cpp)
- **Compiler Flags**: Same flags as Makefile (`-std=c++11 -Wall -O2`)
- **Test Program**: Same as Makefile test target
- **Executable Names**: Same as Makefile (`profanity.x64`, `test_address_validity`)

## Examples

### Full Build Workflow
```bash
# Clone repository
git clone https://github.com/liushengji818-prog/ccr.git
cd ccr

# Install dependencies (Ubuntu/Debian)
sudo apt-get install cmake g++ ocl-icd-opencl-dev libcurl4-openssl-dev

# Build Release version
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Run
./profanity.x64 --help

# Build test
make test_build
./test_address_validity
```

### Development Workflow
```bash
# Build Debug version for development
mkdir build_debug
cd build_debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Debug with gdb
gdb ./profanity.x64
```

### CI/CD Pipeline
```bash
# Automated build script
#!/bin/bash
set -e

# Clean build
rm -rf build
mkdir build
cd build

# Configure
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
make -j$(nproc)

# Test
./test_address_validity

# Package
tar czf profanity-$(uname -m).tar.gz profanity.x64
```

## Questions?

For issues or questions:
1. Check this README
2. Check original Makefile behavior
3. Review CMakeLists.txt comments
4. Open an issue on GitHub

---

**Note**: Both Makefile and CMake are supported. Use whichever fits your workflow better!
