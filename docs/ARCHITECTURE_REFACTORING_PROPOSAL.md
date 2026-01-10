# Architecture Refactoring Proposal
## TRON Address Generator - Modular Design

**Date**: 2026-01-09  
**Status**: Proposal (No Code Changes)  
**Purpose**: Propose clean modular architecture for address generation pipeline

---

## Current Architecture Analysis

### Current State: Monolithic Design
The codebase currently follows a monolithic pattern with tight coupling:

```
profanity.cpp (main + CLI + device management)
    ↓
Dispatcher.cpp (GPU orchestration + crypto + HTTP + result handling)
    ↓
Multiple kernels + utilities scattered across files
```

**Problems**:
1. **Massive files**: Dispatcher.cpp (22K lines), precomp.cpp (1.6M lines)
2. **Mixed responsibilities**: Single files handle multiple concerns
3. **Tight coupling**: Changes in one area affect multiple components
4. **Hard to test**: No clear boundaries for unit testing
5. **Poor reusability**: Can't extract components for other uses

---

## Proposed Modular Architecture

### Core Principle: Separation of Concerns

```
┌─────────────────────────────────────────────────────────────┐
│                         Application Layer                    │
│                        (profanity.cpp)                       │
└─────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌──────────────┐    ┌──────────────────┐    ┌─────────────┐
│     CLI      │    │  GPU Orchestrator │    │   Output    │
│   Module     │    │      Module       │    │   Module    │
└──────────────┘    └──────────────────┘    └─────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌──────────────┐    ┌──────────────────┐    ┌─────────────┐
│    Crypto    │    │   OpenCL Kernel  │    │   Pattern   │
│   Module     │    │     Manager      │    │  Matching   │
└──────────────┘    └──────────────────┘    └─────────────┘
        │                     │                     │
        └─────────────────────┴─────────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │   Utilities &    │
                    │   Common Types   │
                    └──────────────────┘
```

---

## Detailed Module Design

### Module 1: CLI Module
**Purpose**: Handle command-line interface and user interaction  
**Responsibility**: Argument parsing, help text, user input validation

**Current Files**:
- `ArgParser.hpp` ✅ Keep as-is
- `help.hpp` ✅ Keep as-is
- Parts of `profanity.cpp` (lines 171-280) → Extract to CLI module

**Proposed Structure**:
```
cli/
├── ArgParser.hpp       (existing, keep)
├── help.hpp           (existing, keep)
└── CLIController.hpp  (NEW - extracted from profanity.cpp)
    ├── parseArguments()
    ├── validateInputs()
    ├── displayHelp()
    └── setupConfiguration()
```

**Interfaces**:
```cpp
class CLIController {
public:
    struct Configuration {
        std::string matchingFile;
        int prefixCount;
        int suffixCount;
        int quitCount;
        bool inverseMultiple;
        size_t inverseSize;
        std::string outputFile;
        std::string postUrl;
    };
    
    static Configuration parse(int argc, char** argv);
    static void displayHelp();
};
```

---

### Module 2: Crypto Module
**Purpose**: Handle all cryptographic operations  
**Responsibility**: Hashing, encoding, address generation

**Current Files**:
- `picosha2.h` ✅ Keep as-is (third-party)
- Parts of `Dispatcher.cpp` (lines 18-107) → Extract to Crypto module

**Proposed Structure**:
```
crypto/
├── Hash.hpp           (NEW - SHA256 operations)
│   ├── sha256()
│   └── sha256_hex_string()
├── Encoding.hpp       (NEW - Base58 encoding)
│   ├── base58Encode()
│   ├── base58Decode()
│   └── hexToStr()
└── AddressGenerator.hpp (NEW - TRON address generation)
    ├── publicKeyToAddress()
    ├── toTronAddress()
    └── validateAddress()
```

**Interfaces**:
```cpp
namespace Crypto {
    // Hash operations
    std::string sha256(const std::string& input);
    
    // Encoding
    std::string base58Encode(const std::vector<uint8_t>& data);
    bool base58Decode(const std::string& input, std::vector<uint8_t>& out);
    
    // Address generation
    std::string toTronAddress(const std::string& publicKeyHash);
    bool isValidTronAddress(const std::string& address);
}
```

---

### Module 3: Pattern Matching Module
**Purpose**: Handle address pattern matching logic  
**Responsibility**: Define matching rules, score addresses

**Current Files**:
- `Mode.hpp` / `Mode.cpp` → Refactor and enhance
- ~~`Mode_Version3.cpp`~~ ✅ **DELETED** (was duplicate - removed in refactoring, see REFACTORING_SUMMARY.md)

**Proposed Structure**:
```
matching/
├── MatchingMode.hpp   (refactored from Mode.hpp)
│   ├── struct MatchingRule
│   ├── class MatchingMode
│   └── scoring logic
└── MatchingMode.cpp   (refactored from Mode.cpp)
    ├── loadPatternsFromFile()
    ├── scoreAddress()
    └── isMatch()
```

**Interfaces**:
```cpp
class MatchingMode {
public:
    struct Rule {
        std::string pattern;
        int prefixCount;
        int suffixCount;
    };
    
    static MatchingMode fromFile(const std::string& filename);
    int scoreAddress(const std::string& address) const;
    bool isMatch(const std::string& address) const;
    
private:
    std::vector<Rule> m_rules;
    int m_prefixCount;
    int m_suffixCount;
};
```

---

### Module 4: OpenCL Kernel Manager
**Purpose**: Manage GPU kernels and compilation  
**Responsibility**: Kernel loading, compilation, caching

**Current Files**:
- `kernel_profanity.hpp` ✅ Keep kernel code
- `kernel_sha256.hpp` ✅ Keep kernel code
- `kernel_keccak.hpp` ✅ Keep kernel code
- Parts of `profanity.cpp` (lines 280-420) → Extract to KernelManager

**Proposed Structure**:
```
gpu/
├── kernels/
│   ├── kernel_profanity.hpp  (existing, keep)
│   ├── kernel_sha256.hpp     (existing, keep)
│   └── kernel_keccak.hpp     (existing, keep)
├── KernelManager.hpp         (NEW)
│   ├── loadKernels()
│   ├── compileKernels()
│   ├── cacheKernels()
│   └── getCompiledKernel()
└── KernelManager.cpp
```

**Interfaces**:
```cpp
class KernelManager {
public:
    struct KernelSet {
        cl_kernel init;
        cl_kernel inverse;
        cl_kernel iterate;
        cl_kernel score;
    };
    
    KernelSet loadKernels(cl_context context, cl_device_id device,
                          bool useCache, size_t inverseSize);
    void saveToCache(cl_device_id device, const std::string& binary);
    
private:
    std::string getCacheFilename(cl_device_id device, size_t inverseSize);
};
```

---

### Module 5: GPU Orchestrator
**Purpose**: Coordinate GPU task execution  
**Responsibility**: Device management, work distribution, result collection

**Current Files**:
- `Dispatcher.hpp` / `Dispatcher.cpp` → Split into multiple files
- `SpeedSample.hpp` ✅ Keep as-is
- `CLMemory.hpp` ✅ Keep as-is

**Proposed Structure**:
```
gpu/
├── DeviceManager.hpp     (NEW - device enumeration)
│   ├── enumerateDevices()
│   ├── getDeviceInfo()
│   └── selectDevices()
├── WorkDispatcher.hpp    (NEW - refactored from Dispatcher.hpp)
│   ├── class Device
│   ├── dispatchWork()
│   └── collectResults()
├── WorkDispatcher.cpp    (refactored from Dispatcher.cpp)
├── SpeedSample.hpp       (existing, keep)
└── CLMemory.hpp          (existing, keep)
```

**Interfaces**:
```cpp
class DeviceManager {
public:
    struct DeviceInfo {
        cl_device_id id;
        std::string name;
        size_t globalMemory;
        size_t computeUnits;
    };
    
    std::vector<DeviceInfo> enumerateDevices();
    std::vector<cl_device_id> selectDevices(const std::vector<size_t>& skipIndices);
};

class WorkDispatcher {
public:
    struct Result {
        std::string address;
        std::string privateKey;
        int score;
    };
    
    void start(cl_context context, cl_program program,
               const MatchingMode& mode, size_t worksizeMax);
    std::vector<Result> getResults();
    void stop();
    
private:
    class Device; // Internal device handler
};
```

---

### Module 6: Output Module
**Purpose**: Handle result output and reporting  
**Responsibility**: Console output, file writing, HTTP posting

**Current Files**:
- Parts of `Dispatcher.cpp` (printResult, HTTP posting) → Extract

**Proposed Structure**:
```
output/
├── ResultWriter.hpp    (NEW)
│   ├── writeToConsole()
│   ├── writeToFile()
│   └── postToUrl()
└── ResultWriter.cpp
```

**Interfaces**:
```cpp
class ResultWriter {
public:
    struct Result {
        std::string address;
        std::string privateKey;
        int score;
        size_t deviceIndex;
    };
    
    void writeToConsole(const Result& result);
    void appendToFile(const std::string& filename, const Result& result);
    bool postToUrl(const std::string& url, const Result& result);
    
private:
    std::mutex m_mutex; // Thread-safe writing
};
```

---

### Module 7: Utilities Module
**Purpose**: Common utilities and type definitions  
**Responsibility**: Shared helpers, type definitions

**Current Files**:
- `types.hpp` ✅ Keep as-is
- `lexical_cast.hpp` ✅ Keep as-is
- `IMPROVEMENTS.hpp` → Merge useful functions into Utils

**Proposed Structure**:
```
utils/
├── types.hpp          (existing, keep)
├── lexical_cast.hpp   (existing, keep)
├── Validation.hpp     (NEW - from IMPROVEMENTS.hpp)
│   ├── validateMatchingCounts()
│   ├── validateQuitCount()
│   └── validateFileSize()
└── Conversion.hpp     (NEW - string/hex utilities)
    ├── hexToStr()
    └── toHex()
```

---

### Module 8: Precomputation Module
**Purpose**: Manage precomputed elliptic curve data  
**Responsibility**: Load and provide precomputed points

**Current Files**:
- `precomp.hpp` ✅ Keep as-is
- `precomp.cpp` ✅ Keep as-is (1.6MB data file)

**Proposed Structure**:
```
precomputation/
├── precomp.hpp        (existing, keep)
└── precomp.cpp        (existing, keep - generated data)
```

**Note**: This is likely auto-generated data and should remain as-is.

---

## File Mapping: Current → Proposed

### Files to Split

#### `profanity.cpp` (12K lines) → Split into 3 files:
```
profanity.cpp (main + device enum)
    → main.cpp                    (NEW, ~200 lines - main entry point)
    → cli/CLIController.cpp       (NEW, ~200 lines - CLI logic)
    → gpu/DeviceManager.cpp       (NEW, ~300 lines - device enumeration)
```

#### `Dispatcher.cpp` (22K lines) → Split into 4 files:
```
Dispatcher.cpp
    → crypto/AddressGenerator.cpp  (NEW, ~200 lines - crypto functions)
    → gpu/WorkDispatcher.cpp       (NEW, ~15K lines - GPU orchestration)
    → output/ResultWriter.cpp      (NEW, ~300 lines - output handling)
    → gpu/DeviceManager.cpp        (merge device logic)
```

#### `Dispatcher.hpp` (3.7K lines) → Split into 3 files:
```
Dispatcher.hpp
    → gpu/WorkDispatcher.hpp       (NEW, ~2K lines - main dispatcher)
    → gpu/DeviceManager.hpp        (NEW, ~500 lines - device struct)
    → output/ResultWriter.hpp      (NEW, ~300 lines - result struct)
```

### Files to Merge/Delete

#### DELETE (✅ COMPLETED in refactoring):
- ✅ ~~`Mode_Version3.cpp`~~ **DELETED** (was duplicate of Mode.cpp) - see REFACTORING_SUMMARY.md
- ✅ ~~`demo/test_cl.cpp`~~ **DELETED** (was unused standalone tool) - see REFACTORING_SUMMARY.md
- ✅ ~~`BUILD_INSTRUCTIONS.md`~~ **DELETED** (was empty, replaced by CMAKE_BUILD_INSTRUCTIONS.md) - see REFACTORING_SUMMARY.md

#### REFACTOR:
- ✅ `IMPROVEMENTS.hpp` → **PARTIALLY COMPLETED**: Cleaned up from 190 to 51 lines (removed unused functions), see REFACTORING_SUMMARY.md
- `IMPROVEMENTS.hpp` → Future: Split remaining functions into `utils/` (optional future enhancement)
- `Mode.cpp` / `Mode.hpp` → Rename to `MatchingMode.*` (future refactoring)

### Files to Keep As-Is

✅ **Keep unchanged**:
- `ArgParser.hpp` (well-designed, standalone)
- `help.hpp` (simple, standalone)
- `SpeedSample.hpp` (focused, single responsibility)
- `CLMemory.hpp` (generic template, reusable)
- `types.hpp` (type definitions)
- `lexical_cast.hpp` (utility)
- `picosha2.h` (third-party library)
- `kernel_*.hpp` (GPU kernel code)
- `precomp.*` (generated data)
- `test_address_validity.cpp` (test utility)
- OpenCL headers (dependencies)
- Curl headers (dependencies)

---

## Proposed Directory Structure

```
ccr/
├── src/
│   ├── main.cpp                      (NEW - main entry point)
│   ├── cli/
│   │   ├── ArgParser.hpp             (existing)
│   │   ├── help.hpp                  (existing)
│   │   └── CLIController.{hpp,cpp}   (NEW - extracted)
│   ├── crypto/
│   │   ├── Hash.hpp                  (NEW)
│   │   ├── Encoding.{hpp,cpp}        (NEW)
│   │   └── AddressGenerator.{hpp,cpp} (NEW - extracted)
│   ├── matching/
│   │   ├── MatchingMode.hpp          (refactored Mode.hpp)
│   │   └── MatchingMode.cpp          (refactored Mode.cpp)
│   ├── gpu/
│   │   ├── kernels/
│   │   │   ├── kernel_profanity.hpp  (existing)
│   │   │   ├── kernel_sha256.hpp     (existing)
│   │   │   └── kernel_keccak.hpp     (existing)
│   │   ├── DeviceManager.{hpp,cpp}   (NEW - extracted)
│   │   ├── KernelManager.{hpp,cpp}   (NEW - extracted)
│   │   ├── WorkDispatcher.{hpp,cpp}  (refactored Dispatcher)
│   │   ├── CLMemory.hpp              (existing)
│   │   └── SpeedSample.hpp           (existing)
│   ├── output/
│   │   └── ResultWriter.{hpp,cpp}    (NEW - extracted)
│   ├── utils/
│   │   ├── types.hpp                 (existing)
│   │   ├── lexical_cast.hpp          (existing)
│   │   ├── Validation.hpp            (NEW - from IMPROVEMENTS)
│   │   └── Conversion.hpp            (NEW)
│   └── precomputation/
│       ├── precomp.hpp               (existing)
│       └── precomp.cpp               (existing)
├── include/                          (dependencies)
│   ├── OpenCL/
│   ├── Curl/
│   └── picosha2.h
├── tests/
│   └── test_address_validity.cpp    (existing)
│   ~~test_cl.cpp~~                  (was deleted - unused standalone tool, see REFACTORING_SUMMARY.md)
├── docs/
│   ├── README_PACKAGE.md            (existing)
│   ├── FIX_SUMMARY.md               (existing)
│   ├── SECURITY_FIX_DOCUMENTATION.md (existing)
│   ├── TEST_README.md               (existing)
│   └── ARCHITECTURE.md              (this document)
├── Makefile                         (update for new structure)
└── .gitignore                       (existing)
```

---

## Benefits of Proposed Architecture

### 1. **Separation of Concerns**
- Each module has a single, well-defined responsibility
- Changes in one module don't affect others
- Easier to understand and maintain

### 2. **Testability**
- Each module can be unit tested independently
- Mock interfaces for integration testing
- Clearer test coverage

### 3. **Reusability**
- Crypto module can be used in other projects
- Pattern matching logic is portable
- GPU orchestration framework is reusable

### 4. **Maintainability**
- Smaller files (200-500 lines each vs 22K lines)
- Clear boundaries and interfaces
- Easier onboarding for new developers

### 5. **Scalability**
- Easy to add new features (e.g., new hash algorithms)
- Easy to swap implementations (e.g., different GPU backends)
- Plugin architecture possible

---

## Migration Strategy (When Implementing)

### Phase 1: Extract Utilities (Low Risk)
1. Create `utils/Validation.hpp` from `IMPROVEMENTS.hpp`
2. Create `utils/Conversion.hpp` for helper functions
3. Update includes in existing files

### Phase 2: Extract Crypto Module (Medium Risk)
1. Create `crypto/` directory structure
2. Extract functions from `Dispatcher.cpp`
3. Update Dispatcher to use Crypto module
4. Run tests to verify

### Phase 3: Refactor Mode → MatchingMode (Low Risk)
1. Rename files
2. Update includes
3. ✅ ~~Delete `Mode_Version3.cpp`~~ **COMPLETED** (see REFACTORING_SUMMARY.md)

### Phase 4: Extract Output Module (Low Risk)
1. Create `output/ResultWriter`
2. Move output functions from Dispatcher
3. Update Dispatcher to use ResultWriter

### Phase 5: Split Dispatcher (High Risk)
1. Create `gpu/DeviceManager`
2. Create `gpu/KernelManager`
3. Refactor `Dispatcher` → `WorkDispatcher`
4. Update main.cpp
5. Extensive testing

### Phase 6: Extract CLI Module (Low Risk)
1. Create `cli/CLIController`
2. Extract from profanity.cpp
3. Create new main.cpp

---

## Updated Makefile Structure

```makefile
# Source directories
SRC_DIR=src
CRYPTO_DIR=$(SRC_DIR)/crypto
MATCHING_DIR=$(SRC_DIR)/matching
GPU_DIR=$(SRC_DIR)/gpu
OUTPUT_DIR=$(SRC_DIR)/output
UTILS_DIR=$(SRC_DIR)/utils
PRECOMP_DIR=$(SRC_DIR)/precomputation
CLI_DIR=$(SRC_DIR)/cli

# Source files
SOURCES= \
    $(SRC_DIR)/main.cpp \
    $(CLI_DIR)/CLIController.cpp \
    $(CRYPTO_DIR)/AddressGenerator.cpp \
    $(MATCHING_DIR)/MatchingMode.cpp \
    $(GPU_DIR)/DeviceManager.cpp \
    $(GPU_DIR)/KernelManager.cpp \
    $(GPU_DIR)/WorkDispatcher.cpp \
    $(OUTPUT_DIR)/ResultWriter.cpp \
    $(PRECOMP_DIR)/precomp.cpp

# Object files
OBJECTS=$(SOURCES:.cpp=.o)

# Executable
EXECUTABLE=profanity.x64

# ... rest of Makefile
```

---

## Core Address Generation Pipeline (Simplified)

```
User Input (CLI)
    ↓
Pattern Matching Rules (MatchingMode)
    ↓
Device Selection (DeviceManager)
    ↓
Kernel Compilation (KernelManager)
    ↓
Work Distribution (WorkDispatcher)
    ↓
GPU Execution (OpenCL Kernels)
    ↓
Result Collection (WorkDispatcher)
    ↓
Address Validation (Crypto Module)
    ↓
Output (ResultWriter)
```

---

## Dependencies Between Modules

```
main.cpp
  ├─→ cli/CLIController
  ├─→ gpu/DeviceManager
  ├─→ gpu/KernelManager
  ├─→ gpu/WorkDispatcher
  │     ├─→ matching/MatchingMode
  │     ├─→ crypto/AddressGenerator
  │     ├─→ output/ResultWriter
  │     ├─→ gpu/CLMemory
  │     └─→ gpu/SpeedSample
  └─→ matching/MatchingMode

crypto/AddressGenerator
  └─→ picosha2.h (third-party)

All modules
  └─→ utils/* (shared utilities)
```

---

## API Contract Examples

### Example 1: Using Crypto Module
```cpp
#include "crypto/AddressGenerator.hpp"

// Generate TRON address from public key hash
std::string pubKeyHash = "...";
std::string address = Crypto::toTronAddress(pubKeyHash);

// Validate address
if (Crypto::isValidTronAddress(address)) {
    std::cout << "Valid: " << address << std::endl;
}
```

### Example 2: Using MatchingMode
```cpp
#include "matching/MatchingMode.hpp"

// Load patterns from file
auto mode = MatchingMode::fromFile("patterns.txt");
mode.setPrefixCount(2);
mode.setSuffixCount(4);

// Score an address
int score = mode.scoreAddress("T9yD8xfN9aQ...");
if (score >= 100) {
    std::cout << "Match found! Score: " << score << std::endl;
}
```

### Example 3: Using WorkDispatcher
```cpp
#include "gpu/WorkDispatcher.hpp"
#include "gpu/DeviceManager.hpp"

// Setup devices
DeviceManager deviceMgr;
auto devices = deviceMgr.selectDevices({/* skip indices */});

// Setup dispatcher
WorkDispatcher dispatcher(context, program);
dispatcher.setMode(mode);
dispatcher.setDevices(devices);
dispatcher.start();

// Collect results
while (dispatcher.isRunning()) {
    auto results = dispatcher.getResults();
    for (const auto& result : results) {
        writer.writeToConsole(result);
    }
}
```

---

## Testing Strategy

### Unit Tests (Per Module)
```
tests/
├── crypto_test.cpp         (test hash, encoding, address gen)
├── matching_test.cpp       (test pattern matching logic)
├── validation_test.cpp     (test input validation)
└── encoding_test.cpp       (test base58 encode/decode)
```

### Integration Tests
```
tests/
├── integration_test.cpp    (test full pipeline)
└── gpu_integration_test.cpp (test GPU modules together)
```

### Existing Tests
```
tests/
└── test_address_validity.cpp (existing - keep)

Note: test_cl.cpp was deleted during refactoring (unused standalone tool)
See REFACTORING_SUMMARY.md for details
```

---

## Performance Considerations

### No Performance Impact Expected
- Modularization is compile-time organization
- Modern compilers optimize across translation units
- Inline functions for hot paths
- GPU kernel code unchanged

### Potential Improvements
- Easier to profile individual modules
- Easier to optimize specific components
- Better cache locality with focused modules

---

## Summary

### Current Issues
❌ Monolithic design  
❌ Mixed responsibilities  
❌ Hard to test  
❌ Difficult to maintain  
❌ Poor reusability  

### Proposed Solution
✅ Modular architecture  
✅ Clear separation of concerns  
✅ Testable components  
✅ Maintainable codebase  
✅ Reusable modules  

### Key Modules
1. **CLI Module** - User interface
2. **Crypto Module** - Cryptographic operations
3. **Pattern Matching Module** - Address matching logic
4. **OpenCL Kernel Manager** - Kernel compilation
5. **GPU Orchestrator** - Work distribution
6. **Output Module** - Result handling
7. **Utilities Module** - Shared helpers
8. **Precomputation Module** - Precomputed data

### Implementation Approach
- **Phase 1-2**: Extract utilities and crypto (low risk)
- **Phase 3-4**: Refactor Mode and output (low risk)
- **Phase 5**: Split Dispatcher (high risk - needs careful testing)
- **Phase 6**: Extract CLI (low risk)

---

**End of Architecture Proposal**

*Note: This is a proposal document. No code changes have been made.*
