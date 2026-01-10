# 代码包说明

## 打包内容

本代码包包含 Tron 地址生成器的完整源代码，可以直接编译使用。

### 项目结构（标准 CMake 布局）

```
.
├── CMakeLists.txt          # CMake 构建配置
├── Makefile                # Makefile 构建配置（兼容）
├── src/                    # 源代码目录
│   ├── profanity.cpp       # 主程序入口
│   ├── Dispatcher.cpp      # GPU任务调度器
│   ├── Mode.cpp            # 匹配模式处理
│   ├── precomp.cpp         # 预计算处理
│   └── test/               # 测试文件
│       └── test_address_validity.cpp
├── include/                # 公共头文件
│   ├── Dispatcher.hpp      # GPU任务调度器头文件
│   ├── Mode.hpp            # 匹配模式处理头文件
│   ├── precomp.hpp         # 预计算处理头文件
│   ├── ArgParser.hpp       # 参数解析
│   ├── CLMemory.hpp        # OpenCL内存管理
│   ├── SpeedSample.hpp     # 性能采样
│   ├── help.hpp            # 帮助信息
│   ├── lexical_cast.hpp    # 类型转换
│   ├── types.hpp           # 类型定义
│   ├── IMPROVEMENTS.hpp    # 输入验证函数
│   └── picosha2.h          # SHA256库
├── kernel/                 # GPU内核文件
│   ├── kernel_profanity.hpp # 主要GPU内核
│   ├── kernel_keccak.hpp   # Keccak哈希内核
│   └── kernel_sha256.hpp   # SHA256内核
├── third_party/            # 第三方依赖
│   ├── OpenCL/             # OpenCL头文件
│   └── Curl/               # libcurl头文件
├── docs/                   # 文档目录
│   ├── README_PACKAGE.md   # 本文件
│   └── ...                 # 其他文档
└── cmake/                  # CMake 辅助文件（可选）
```

### 源代码文件

**核心文件** (src/):
- `profanity.cpp` - 主程序入口
- `Dispatcher.cpp` - GPU任务调度器
- `Mode.cpp` - 匹配模式处理
- `precomp.cpp` - 预计算处理

**头文件** (include/):
- `Dispatcher.hpp`, `Mode.hpp`, `precomp.hpp` - 核心模块头文件
- `ArgParser.hpp`, `CLMemory.hpp`, `SpeedSample.hpp` - 辅助模块头文件
- `help.hpp`, `lexical_cast.hpp`, `types.hpp` - 工具头文件
- `IMPROVEMENTS.hpp` - 输入验证函数（已重构，从190行减少到51行）
- `picosha2.h` - SHA256库

**内核文件** (kernel/):
- `kernel_profanity.hpp` - 主要GPU内核
- `kernel_keccak.hpp` - Keccak哈希内核
- `kernel_sha256.hpp` - SHA256内核

**测试文件** (src/test/):
- `test_address_validity.cpp` - 地址有效性测试

### 配置文件

- `CMakeLists.txt` - CMake 构建配置（推荐）
- `Makefile` - Makefile 构建配置（兼容）
- `docs/CMAKE_BUILD_INSTRUCTIONS.md` - CMake 编译说明

**已删除文件**（重构后）:
- ~~`Mode_Version3.cpp`~~ - Mode.cpp 的完全重复文件
- ~~`demo/test_cl.cpp`~~ - 未使用的独立测试工具
- ~~`BUILD_INSTRUCTIONS.md`~~ - 空文件，已由 CMAKE_BUILD_INSTRUCTIONS.md 替代

详见 `docs/REFACTORING_SUMMARY.md`

### 第三方依赖 (third_party/)

- `third_party/OpenCL/` - OpenCL头文件
- `third_party/Curl/` - libcurl头文件

**注意**: 建议使用 Git 子模块或包管理器管理第三方依赖，而不是将头文件复制到项目中。

## 快速开始

### 1. 解压代码包

```bash
tar -xzf profanity-code.tar.gz
cd profanity-code
```

### 2. 编译

**使用 CMake（推荐）**:
```bash
mkdir build && cd build
cmake ..
make
# 或使用 ninja: ninja
```

**使用 Makefile（兼容）**:
```bash
make clean
make
```

### 3. 运行

```bash
# 查看帮助
./profanity.x64 --help

# 基本使用（推荐使用优化参数）
./profanity.x64 --matching addresses.txt --prefix-count 2 --suffix-count 4 -i 1024

# 运行测试
./test_address_validity
```

## 性能优化参数

基于实际测试（RTX 5090），推荐参数：

```bash
# 最优性能（推荐）
-i 1024

# 如果GPU显存更大（如48GB），可以尝试
-i 2048
```

## 系统要求

- **编译器**: g++ (支持C++11)
- **OpenCL**: OpenCL开发库
- **CUDA**: CUDA工具包（可选，用于NVIDIA GPU）
- **curl**: libcurl开发库
- **GPU**: 支持OpenCL的NVIDIA/AMD GPU

## 多GPU支持

程序自动检测并使用所有可用GPU，无需特殊配置。

## 文件清单

**新目录结构**（标准 CMake 布局）:
- `src/`: 4个源代码文件 (.cpp)
- `include/`: 11个头文件 (.hpp, .h)
- `kernel/`: 3个内核文件 (.hpp)
- `src/test/`: 1个测试文件 (.cpp)
- `third_party/`: 第三方依赖（OpenCL, Curl）
- `docs/`: 10个文档文件 (.md)
- **总计**: 约34个主要文件，约2.5MB

**项目结构优势**:
- ✅ 标准 CMake 布局，易于集成和安装
- ✅ 代码组织清晰，易于维护
- ✅ 测试文件独立，便于管理
- ✅ 第三方依赖集中管理

**重构详情**: 删除了3个未使用/重复文件，重构了 IMPROVEMENTS.hpp，并重组为标准 CMake 布局。详见 `docs/REFACTORING_SUMMARY.md`

## 注意事项

1. 编译前确保安装了OpenCL开发库
2. 如果使用NVIDIA GPU，建议安装CUDA工具包
3. 首次编译可能需要几分钟时间
4. 编译后的可执行文件: `profanity.x64`

