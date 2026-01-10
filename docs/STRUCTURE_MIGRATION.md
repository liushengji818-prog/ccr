# 项目结构迁移说明 / Project Structure Migration Guide

**日期 / Date**: 2026-01-09  
**状态 / Status**: ✅ 已完成 / Completed

---

## 概述 / Overview

项目已按照标准 CMake 布局重新组织，提高代码的可维护性和可集成性。

The project has been reorganized following standard CMake layout conventions for better maintainability and integration.

---

## 新的目录结构 / New Directory Structure

```
.
├── CMakeLists.txt          # CMake 构建配置
├── Makefile                # Makefile 构建配置（兼容）
├── README.md               # 项目主文档
├── src/                    # 源代码目录
│   ├── profanity.cpp       # 主程序入口
│   ├── Dispatcher.cpp      # GPU任务调度器
│   ├── Mode.cpp            # 匹配模式处理
│   ├── precomp.cpp         # 预计算处理
│   └── test/               # 测试文件
│       └── test_address_validity.cpp
├── include/                # 公共头文件目录
│   ├── Dispatcher.hpp
│   ├── Mode.hpp
│   ├── precomp.hpp
│   ├── ArgParser.hpp
│   ├── CLMemory.hpp
│   ├── SpeedSample.hpp
│   ├── help.hpp
│   ├── lexical_cast.hpp
│   ├── types.hpp
│   ├── IMPROVEMENTS.hpp
│   └── picosha2.h
├── kernel/                 # GPU内核文件目录
│   ├── kernel_profanity.hpp
│   ├── kernel_keccak.hpp
│   └── kernel_sha256.hpp
├── third_party/            # 第三方依赖目录
│   ├── OpenCL/             # OpenCL头文件
│   │   └── include/CL/
│   └── Curl/               # libcurl头文件
│       └── include/curl/
├── docs/                   # 文档目录
│   ├── README_PACKAGE.md
│   ├── CMAKE_BUILD_INSTRUCTIONS.md
│   ├── REFACTORING_SUMMARY.md
│   └── ...                 # 其他文档
└── cmake/                  # CMake 辅助文件目录（可选）
```

---

## 迁移详情 / Migration Details

### 文件移动 / Files Moved

#### 源代码文件 → `src/`
- ✅ `profanity.cpp` → `src/profanity.cpp`
- ✅ `Dispatcher.cpp` → `src/Dispatcher.cpp`
- ✅ `Mode.cpp` → `src/Mode.cpp`
- ✅ `precomp.cpp` → `src/precomp.cpp`

#### 头文件 → `include/`
- ✅ `ArgParser.hpp` → `include/ArgParser.hpp`
- ✅ `CLMemory.hpp` → `include/CLMemory.hpp`
- ✅ `Dispatcher.hpp` → `include/Dispatcher.hpp`
- ✅ `Mode.hpp` → `include/Mode.hpp`
- ✅ `precomp.hpp` → `include/precomp.hpp`
- ✅ `SpeedSample.hpp` → `include/SpeedSample.hpp`
- ✅ `help.hpp` → `include/help.hpp`
- ✅ `lexical_cast.hpp` → `include/lexical_cast.hpp`
- ✅ `types.hpp` → `include/types.hpp`
- ✅ `IMPROVEMENTS.hpp` → `include/IMPROVEMENTS.hpp`
- ✅ `picosha2.h` → `include/picosha2.h`

#### 内核文件 → `kernel/`
- ✅ `kernel_profanity.hpp` → `kernel/kernel_profanity.hpp`
- ✅ `kernel_keccak.hpp` → `kernel/kernel_keccak.hpp`
- ✅ `kernel_sha256.hpp` → `kernel/kernel_sha256.hpp`

#### 第三方依赖 → `third_party/`
- ✅ `OpenCL/` → `third_party/OpenCL/`
- ✅ `Curl/` → `third_party/Curl/`

#### 测试文件 → `src/test/`
- ✅ `test_address_validity.cpp` → `src/test/test_address_validity.cpp`

#### 文档文件 → `docs/`
- ✅ 所有 `.md` 文件 → `docs/`

---

## 构建系统更新 / Build System Updates

### CMakeLists.txt

已更新以反映新的目录结构：
- ✅ 源文件路径：`src/*.cpp`
- ✅ 包含目录：`include/`, `kernel/`, `src/`
- ✅ 第三方依赖：`third_party/OpenCL/include`, `third_party/Curl/include`
- ✅ 测试文件：`src/test/test_address_validity.cpp`
- ✅ 启用 CTest 支持

### Makefile

已更新以反映新的目录结构：
- ✅ 源文件路径：`src/*.cpp`
- ✅ 包含标志：`-I./include -I./kernel -I./src -I./third_party/OpenCL/include -I./third_party/Curl/include`
- ✅ 测试文件路径：`src/test/test_address_validity.cpp`
- ✅ 编译规则已适配新的路径结构

---

## 包含路径 / Include Paths

由于构建系统已经配置了正确的包含目录，源代码中的 `#include` 语句无需修改：

### 头文件包含 / Header Includes

```cpp
// 这些仍然可以正常工作，因为构建系统配置了正确的包含路径
#include "Dispatcher.hpp"        // 从 include/ 查找
#include "kernel_profanity.hpp"  // 从 kernel/ 查找
#include "picosha2.h"            // 从 include/ 查找
```

### 第三方库包含 / Third-party Includes

```cpp
#include <CL/cl.h>               // 从 third_party/OpenCL/include/CL/ 查找
#include <curl/curl.h>           // 从 third_party/Curl/include/curl/ 查找
```

---

## 构建说明 / Build Instructions

### 使用 CMake（推荐）

```bash
mkdir build && cd build
cmake ..
make
# 或使用 ninja: ninja
```

### 使用 Makefile（兼容）

```bash
make clean
make
```

### 运行测试

```bash
./test_address_validity
# 或使用 CMake: ctest
```

---

## 优势 / Benefits

### ✅ 标准布局 / Standard Layout
- 遵循 CMake 标准目录结构
- 易于集成到其他项目
- 支持标准安装流程

### ✅ 代码组织 / Code Organization
- 源代码、头文件、内核文件分离清晰
- 测试文件独立管理
- 第三方依赖集中管理

### ✅ 可维护性 / Maintainability
- 目录结构清晰，易于理解
- 文档集中管理（`docs/`）
- 根目录干净整洁

### ✅ 版本控制 / Version Control
- 文档在 `docs/`，可直接同步到 GitHub Wiki
- 第三方依赖建议使用 Git 子模块或包管理器
- 清晰的目录结构便于管理

---

## 后续建议 / Future Recommendations

1. **第三方依赖管理** / **Third-party Dependency Management**
   - 考虑使用 Git 子模块管理 `third_party/` 中的依赖
   - 或使用包管理器（如 vcpkg, Conan）管理依赖
   - 避免将第三方代码直接提交到仓库

2. **CMake 模块** / **CMake Modules**
   - 在 `cmake/` 目录中添加自定义 FindModules
   - 添加工具链配置文件
   - 添加 CMake 宏和函数

3. **测试框架** / **Testing Framework**
   - 考虑集成 Google Test 或 Catch2
   - 使用 CTest 管理测试
   - 扩展 `src/test/` 目录

4. **安装支持** / **Installation Support**
   - CMakeLists.txt 已包含安装规则
   - 可以添加更多安装目标（如头文件、文档）

---

## 验证 / Verification

✅ 所有文件已移动到新位置  
✅ CMakeLists.txt 已更新  
✅ Makefile 已更新  
✅ 文档路径引用已更新  
✅ 构建系统配置正确  
✅ 根目录结构简洁  

---

**状态 / Status**: ✅ 迁移完成 / Migration Complete  
**日期 / Date**: 2026-01-09
