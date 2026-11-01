# aa55-frame

[English](README.md)

一个用于 `AA 55` 二进制帧的轻量 header-only C99 库，包含带参数检查的编码器、流式解码器和 CRC-8。不需要堆分配、操作系统接口或特定平台驱动。

## 特性

- C99 实现，提供 C++ 包含保护
- 单个头文件，无外部依赖
- 无动态内存分配
- 支持 0～32 字节载荷
- 编码器提供明确错误码，并检查输出缓冲区容量
- 逐字节解码，适用于 UART 中断、DMA 缓冲区和主机程序
- 可从噪声、拆包、粘包、非法长度和 CRC 错误中重新同步
- 提供主机端测试和最小示例

## 帧格式

| 字节 | 字段 | 说明 |
|---:|---|---|
| 0 | `0xAA` | 第一个帧头字节 |
| 1 | `0x55` | 第二个帧头字节 |
| 2 | `LEN` | 载荷长度，不包含 `CMD` 和 `CRC8` |
| 3 | `CMD` | 由应用定义的命令字 |
| 4.. | `PAYLOAD` | 零个或多个载荷字节 |
| 最后 | `CRC8` | 覆盖 `LEN`、`CMD` 和 `PAYLOAD` 的校验值 |

CRC 参数：多项式 `0x07`，初值 `0x00`，输入和输出均不反射，无最终异或。这是常见的 CRC-8 校验配置，ASCII 字符串 `123456789` 的校验值为 `0xF4`。

例如，命令字 `0x70`、载荷 `01 02 03` 的完整帧为：

```text
AA 55 03 70 01 02 03 DC
```

## 接入项目

### 复制头文件

将 [`include/aa55_frame.h`](include/aa55_frame.h) 复制到项目中，然后包含：

```c
#include "aa55_frame.h"
```

### CMake

```cmake
add_subdirectory(path/to/aa55-frame)
target_link_libraries(your_target PRIVATE aa55::frame)
```

所有函数都在头文件中实现，因此库目标是 `INTERFACE` 类型。

## 编码一帧

```c
uint8_t payload[] = {0x01, 0x02, 0x03};
uint8_t frame[AA55_FRAME_MAX_SIZE];
size_t frame_size;

aa55_status_t status = aa55_encode(
    frame, sizeof(frame),
    0x70,
    payload, sizeof(payload),
    &frame_size
);

if (status == AA55_STATUS_OK) {
    /* 发送 frame[0..frame_size)。 */
}
```

参数校验失败时，编码器会把 `frame_size` 设为零。输出缓冲区与载荷缓冲区不能重叠。

## 解码字节流

```c
aa55_rx_t rx;
aa55_rx_init(&rx);

void on_uart_byte(uint8_t byte)
{
    aa55_rx_result_t result = aa55_rx_feed(&rx, byte);

    if (result == AA55_RX_FRAME_READY) {
        /* 此处可读取 rx.command、rx.payload_length 和 rx.payload。 */
    }
}
```

每路输入流使用一个独立的接收器。解码字段在下一次调用 `aa55_rx_feed()` 前有效，需要长期保存时应主动复制。调用 `aa55_rx_reset()` 可以丢弃尚未接收完整的帧。

## API 概览

| 函数 | 用途 |
|---|---|
| `aa55_encode()` | 校验参数和缓冲区大小，然后编码一帧 |
| `aa55_rx_init()` | 初始化流式接收器 |
| `aa55_rx_reset()` | 丢弃当前未接收完整的帧 |
| `aa55_rx_feed()` | 处理一个输入字节 |
| `aa55_crc8()` | 计算字节数组的 CRC-8 |
| `aa55_crc8_update()` | 向 CRC-8 中加入一个字节 |

编码器状态值：

| 值 | 含义 |
|---|---|
| `AA55_STATUS_OK` | 编码成功 |
| `AA55_STATUS_INVALID_ARGUMENT` | 必要指针为 `NULL` |
| `AA55_STATUS_PAYLOAD_TOO_LARGE` | 载荷超过 32 字节 |
| `AA55_STATUS_BUFFER_TOO_SMALL` | 输出缓冲区容量不足 |

解码器返回值：

| 值 | 含义 |
|---|---|
| `AA55_RX_INCOMPLETE` | 还需要更多字节 |
| `AA55_RX_FRAME_READY` | 已得到一帧有效数据 |
| `AA55_RX_CRC_ERROR` | CRC 不匹配，接收器已开始重新同步 |
| `AA55_RX_LENGTH_ERROR` | 载荷长度非法，接收器已开始重新同步 |
| `AA55_RX_INVALID_ARGUMENT` | 接收器指针为 `NULL` |

## 构建与测试

只需要 CMake 3.15 或更新版本，以及支持 C99 的编译器：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

测试覆盖标准 CRC 向量、黄金帧、空载荷、最大载荷、参数检查、噪声、重复帧头、连续帧、CRC 和长度错误、全部命令字的往返测试，以及确定性随机输入。

构建后可运行示例：

```sh
./build/examples/aa55_basic
```

在 Windows 多配置生成器（如 Visual Studio）下，可执行文件也可能位于 `build/examples/Debug/`。

## 适用范围与限制

- 命令字和载荷含义由应用定义。
- CRC-8 用于检测传输错误，不提供身份认证，也不能防止主动篡改。
- 协议不包含序号、确认或重传机制；如有需要，应在应用层或传输层补充。
- 载荷上限固定为 32 字节，使接收器占用空间保持确定。

## 许可证

MIT，详见 [LICENSE](LICENSE)。
