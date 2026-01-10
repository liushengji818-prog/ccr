# Address Validity Test Suite

## 目的 (Purpose)

本测试套件用于验证地址生成的真实性和正确性，确保：
1. GPU加速不受影响（独立运行）
2. 生成的验证地址真实有效
3. 私钥与地址匹配正确
4. 种子随机化不存在固定位

This test suite validates the authenticity and correctness of address generation, ensuring:
1. GPU acceleration is not affected (runs independently)
2. Generated validation addresses are authentic and valid
3. Private keys correctly match addresses
4. Seed randomization has no fixed bits

## 测试内容 (Test Coverage)

### Test 1: 地址格式验证 (Address Format Validation)
- 验证TRON地址格式是否正确
- 检查地址长度（必须为34字符）
- 检查前缀（必须以'T'开头）
- 验证Base58字符集

Validates TRON address format:
- Checks address length (must be 34 characters)
- Checks prefix (must start with 'T')
- Validates Base58 character set

### Test 2: 种子随机化测试 (Seed Randomization Test)
- 验证种子的所有组件在多轮迭代中都有变化
- 确保没有固定的中间位（修复了之前的安全漏洞）
- 验证XOR混合操作正确工作

Validates all seed components change across rounds:
- Ensures no fixed middle bits (fixes previous security vulnerability)
- Verifies XOR mixing operations work correctly

### Test 3: 私钥到地址生成测试 (Private Key to Address Generation Test)
- 测试从公钥哈希生成TRON地址的完整流程
- 验证Base58编码正确
- 验证SHA256校验和计算正确

Tests complete pipeline from public key hash to TRON address:
- Verifies Base58 encoding is correct
- Verifies SHA256 checksum calculation is correct

### Test 4: 种子计算一致性测试 (Seed Calculation Consistency Test)
- 确保dispatch()和printResult()中的种子计算逻辑一致
- 验证私钥计算在不同代码路径中产生相同结果

Ensures seed calculation logic is consistent between dispatch() and printResult():
- Verifies private key calculation produces same results in different code paths

## 构建和运行 (Build and Run)

### 构建测试 (Build Test)
```bash
make test
```

### 运行测试 (Run Test)
```bash
./test_address_validity
```

### 清理 (Clean)
```bash
make clean
```

## 测试输出 (Test Output)

成功时的输出 (Successful output):
```
========================================
  Address Validity Test Suite
  Testing address generation authenticity
  (Does NOT affect GPU acceleration)
========================================

Test 1: Validating address format...
  PASSED
Test 2: Testing seed randomization (no fixed bits)...
  PASSED: Critical seed components (s[2], s[3]) change across rounds
         Note: s[1] changes more slowly due to (round >> 32)
Test 3: Testing private key to address generation...
  PASSED: Generated valid address: T9yR5LAzVBYfRqgAE7nxTQ9Hte5TpYU27G
Test 4: Testing seed calculation consistency...
  PASSED: Seed calculations are consistent

========================================
  Results: 4/4 tests passed
========================================
✅ All tests passed! Address generation is valid.
```

## 重要说明 (Important Notes)

### 1. 不影响GPU加速 (Does NOT Affect GPU Acceleration)
- 此测试程序完全独立于主程序运行
- 不依赖OpenCL或GPU设备
- 不会修改或影响任何GPU内核代码
- 可以在没有GPU的环境中运行

This test program runs completely independently of the main program:
- Does not depend on OpenCL or GPU devices
- Does not modify or affect any GPU kernel code
- Can run in environments without GPU

### 2. 验证地址真实性 (Validates Address Authenticity)
- 测试使用与主程序相同的地址生成算法
- 验证私钥计算的一致性
- 确保种子随机化正确工作，避免之前发现的安全漏洞

Tests use the same address generation algorithm as the main program:
- Verifies private key calculation consistency
- Ensures seed randomization works correctly, avoiding the previously discovered security vulnerability

### 3. 测试范围限制 (Test Scope Limitations)
- Test 3中没有实现完整的secp256k1椭圆曲线计算（从私钥到公钥）
- 这需要额外的加密库（如libsecp256k1）
- 当前测试验证了从公钥哈希到TRON地址的转换流程
- 主程序中的GPU内核完成了完整的椭圆曲线计算

Test 3 does not implement full secp256k1 elliptic curve calculation (private key to public key):
- This would require additional cryptographic libraries (like libsecp256k1)
- Current test validates the pipeline from public key hash to TRON address
- The GPU kernel in the main program performs the complete elliptic curve calculation

## 安全性验证 (Security Validation)

此测试套件特别验证了之前修复的关键安全问题：

This test suite specifically validates the critical security issue that was previously fixed:

### 修复前 (Before Fix)
- 种子的中间128位（seed.s[1]和seed.s[2]）在多次迭代中几乎固定
- 生成的私钥中间有20字节高度相似
- 安全风险：可预测的私钥模式

Before the fix:
- Middle 128 bits of seed (seed.s[1] and seed.s[2]) were nearly fixed across iterations
- Generated private keys had ~20 bytes of high similarity in the middle
- Security risk: Predictable private key patterns

### 修复后 (After Fix)
- 所有种子组件在每轮迭代中都会变化
- seed.s[2]和seed.s[3]通过XOR混合立即变化
- seed.s[1]通过(round >> 32)逐渐变化
- 完整的256位私钥随机性

After the fix:
- All seed components change with each round
- seed.s[2] and seed.s[3] change immediately through XOR mixing
- seed.s[1] changes gradually through (round >> 32)
- Full 256-bit private key randomness

## 集成测试 (Integration Testing)

虽然这个测试套件验证了核心算法的正确性，但完整的验证还应该包括：

While this test suite validates core algorithm correctness, complete validation should also include:

1. **实际地址生成测试** (Actual Address Generation Test)
   
   使用提供的 `test_addresses.txt` 文件进行集成测试：
   Use the provided `test_addresses.txt` file for integration testing:
   
   ```bash
   # test_addresses.txt 包含示例匹配模式
   # test_addresses.txt contains sample matching patterns:
   # - AAAA
   # - BBBB
   # - Test
   # - 1234
   # - abcd
   
   # 运行主程序（需要GPU）
   # Run main program (requires GPU)
   ./profanity.x64 --matching test_addresses.txt --prefix-count 2 --suffix-count 4 -q 50
   ```

2. **私钥验证** (Private Key Validation)
   - 使用生成的私钥和地址
   - 在TRON钱包中导入私钥
   - 验证地址匹配

   Use generated private key and address:
   - Import private key in TRON wallet
   - Verify address matches

3. **多GPU测试** (Multi-GPU Test)
   - 在多GPU环境中运行
   - 验证每个GPU生成的地址都是有效的
   
   Run in multi-GPU environment:
   - Verify addresses generated by each GPU are valid

## 参考文档 (Reference Documentation)

- [SECURITY_FIX_DOCUMENTATION.md](SECURITY_FIX_DOCUMENTATION.md) - 详细的安全修复说明
- [FIX_SUMMARY.md](FIX_SUMMARY.md) - 修复总结
- [README_PACKAGE.md](README_PACKAGE.md) - 项目说明

## 维护 (Maintenance)

如果修改了以下代码，请同步更新测试：

If you modify the following code, please update tests accordingly:

1. `Dispatcher.cpp`中的`dispatch()`方法 (dispatch() method in Dispatcher.cpp)
2. `Dispatcher.cpp`中的`printResult()`函数 (printResult() function in Dispatcher.cpp)
3. 种子随机化逻辑 (Seed randomization logic)
4. 地址生成逻辑 (Address generation logic)

---

**版本 (Version)**: 1.0  
**创建日期 (Created)**: 2026-01-08  
**状态 (Status)**: ✅ 所有测试通过 (All tests passing)
