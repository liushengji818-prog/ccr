# Profanity - TRON Address Generator

[![CI](https://github.com/liushengji818-prog/shabi/actions/workflows/ci.yml/badge.svg)](https://github.com/liushengji818-prog/shabi/actions/workflows/ci.yml)

GPU-accelerated TRON address generator with pattern matching capabilities.

## Features

- ✅ GPU acceleration using OpenCL
- ✅ Multi-GPU support (automatic detection)
- ✅ Pattern matching (prefix/suffix)
- ✅ Performance optimization
- ✅ Standard CMake layout for easy integration
- ✅ Clean, maintainable codebase

## Supported Platforms

- **Linux** (Ubuntu, Debian, Fedora, CentOS, Arch)
- **macOS** (10.15+)

## System Requirements

- **Compiler**: g++ or clang with C++11 support
- **Build System**: CMake 3.10+
- **OpenCL**: OpenCL 1.2+ compatible GPU and drivers
- **libcurl**: 7.x or later
- **GPU**: OpenCL-compatible GPU (NVIDIA, AMD, or Intel)

## Dependencies Installation

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libcurl4-openssl-dev \
    ocl-icd-opencl-dev
