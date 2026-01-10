# Third-Party Dependencies

This directory contains third-party dependencies used by the Profanity project. These dependencies are vendored directly (header files included in the repository) for ease of development and to ensure compatibility.

## Dependencies

### OpenCL Headers

- **Name**: OpenCL (Open Computing Language)
- **Version**: 1.2 / 2.0 compatible headers
- **Location**: `third_party/OpenCL/include/`
- **Purpose**: GPU computation interface for address generation
- **Source**: Khronos OpenCL Headers
- **URL**: https://github.com/KhronosGroup/OpenCL-Headers
- **License**: Apache License 2.0
- **Usage**: Headers only (requires linking against OpenCL runtime library at link time and ICD loader at runtime)
- **Installation**: 
  - Ubuntu/Debian: `sudo apt-get install ocl-icd-opencl-dev`
  - macOS: Built-in OpenCL framework
  - NVIDIA: CUDA Toolkit
  - AMD: ROCm or AMDGPU-Pro driver

### libcurl Headers

- **Name**: libcurl
- **Version**: Compatible with libcurl 7.x+
- **Location**: `third_party/Curl/include/`
- **Purpose**: HTTP client library (potentially for updates/networking features)
- **Source**: curl project
- **URL**: https://curl.se/libcurl/
- **License**: curl License (MIT/X derivative license)
- **Usage**: Headers only (requires linking against libcurl library at link/runtime)
- **Installation**:
  - Ubuntu/Debian: `sudo apt-get install libcurl4-openssl-dev`
  - macOS: Built-in curl library (or `brew install curl` for newer versions)
  - Fedora: `sudo dnf install libcurl-devel`

## Vendoring Strategy

This project uses **direct vendoring** of header files:

1. **Headers included**: Only header files are vendored in this repository
2. **Libraries required at runtime**: The actual compiled libraries (`.so`, `.dylib`, `.dll`) must be installed on the system
3. **Build fallback**: CMake first tries to detect system-installed libraries, then falls back to these vendored headers if system headers are not found

## License Compliance

### OpenCL Headers License

The OpenCL headers are licensed under the **Apache License 2.0**:

```
Copyright (c) 2008-2023 The Khronos Group Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0
```

Full license: https://www.apache.org/licenses/LICENSE-2.0

### libcurl License

The libcurl library is licensed under a **curl License** (MIT/X derivative):

```
Copyright (c) 1996 - 2023, Daniel Stenberg, <daniel@haxx.se>, et al.

Permission to use, copy, modify, and distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright
notice and this permission notice appear in all copies.
```

Full license: https://curl.se/docs/copyright.html

## Notes

- These dependencies are only used for compilation
- Runtime dependencies must be satisfied by system-installed libraries
- CMake build system automatically detects and uses system libraries when available
- Vendored headers are used as fallback for compilation only
