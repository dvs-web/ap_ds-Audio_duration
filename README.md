
# `ap_ds` - 音频时长解析器

`ap_ds` 是 **ap_ds音频库** 的一个专注分支，它是一个用C编写的、仅 **14KB** 的轻量级DLL，为Python等语言提供 **MP3, OGG, FLAC, WAV** 四大音频格式的**时长获取**功能。

> **“为啥造这个轮子？因为FFmpeg太重，Pygame太瞎，WINAPI太残，不如自己写个明白。”**

## 🚀 特性

- **🪶 极致轻量**：14KB，零依赖，纯C编写。
- **⚡ 性能暴力**：C原生性能，解析秒级完成。
- **🎯 精准定位**：不为取代FFmpeg，只为解决轻量级库的“功能盲区”。
- **⚖️ 架构清晰**：与SDL2等播放器完美互补，你负责播，我负责读。
- **⚠️ 真实坦诚**：明确**仅支持四大金刚格式**，服务条款“霸道”。

## 📦 用法（Python示例）

```
from ctypes import CDLL

# 加载DLL
audio = CDLL('./audio_parser.dll')

# 自动检测格式（推荐）
duration_seconds = audio.GetAudioDuration(b"path/to/your/audio.mp3")
print(f"时长: {duration_seconds} 秒")

# 或使用指定格式函数（效率更高）
duration_seconds = audio.GetMp3Duration(b"path/to/your/audio.mp3")
duration_seconds = audio.GetFlacDuration(b"path/to/your/audio.flac")
# ... 其他格式同理
```

## 🤔 为什么存在？（“轮子宣言”）

| 对比对象 | 我们的优势 | 他们的缺陷 |
| :--- | :--- | :--- |
| **FFmpeg / pydub** | **14KB** 零依赖，即插即用 | **~260MB** 依赖地狱，部署噩梦 |
| **Pygame / PySDL2** | **能准确读取时长**，支持多格式 | **只能播，不能读**，对元数据“失明” |
| **WINAPI底层调用** | **接口简单**，功能专注 | **复杂到残疾**，兼容性差 |
| **其他Python音频库** | **C原生性能**，解析飞快 | **Python循环**，速度堪忧 |

**结论**：在“轻量级”和“功能完备”之间，我们选择了第三条路——**用最合适的工具组合，解决最具体的痛点**。

## 🛠️ 技术架构

```
你的Python应用
    ↓ (ctypes调用)
audio_parser.dll (本库，14KB，负责元数据解析)
    ↓
你的音频文件 (MP3/OGG/FLAC/WAV)
    ↑
SDL2.dll (2.67MB，负责播放，与本库解耦)
```

## 🚫 限制与条款（“爱用不用”版）

1.  **格式**：明确支持 **MP3, OGG, FLAC, WAV**。**不支持AAC等**，别问，问就是懒。
2.  **性质**：本库是**个人技术练习作品**，非商业级产品。
3.  **责任**：我们尽力让代码可靠，但不对你的数据丢失负责。**详见下文《服务条款》。**

## 📜 服务条款（精简硬核版）

> 本库是按“现状”提供的技术实验品。你使用本库即代表：
> 1.  理解这是一个**非商业、无SLA保障**的个人项目。
> 2.  同意**不会用其存储任何重要或敏感数据**。
> 3.  认同**因使用本库导致的任何直接或间接损失，责任上限为零元**。
> 4.  若不同意以上条款，**请立即删除本库**。

## 🏰 关于作者与项目脉络

**DVS** - 一个10岁的中国全栈开发者。

- **主站**：[DVS云盘 dvsyun.top](https://dvsyun.top)
- **邮箱**：`me@dvsyun.top` (所有邮件都会看，但回复看心情)

**本项目进化史**：
`WINAPI残疾版` → `Pygame缝合怪` → **`ap_ds (当前)`**
（从底层挣扎到封装妥协，最终走向架构整合的觉悟之路）

---

**© DVS | 工具为效率服务，不为逼格服务。**

# English Version

# `ap_ds` - Audio Duration Parser

`ap_ds` is a focused branch of the **ap_ds Audio Library**. It's a lightweight **14KB** DLL written in C, providing **duration reading** for four major audio formats (**MP3, OGG, FLAC, WAV**) to languages like Python.

> **"Why reinvent the wheel? Because FFmpeg is bloated, Pygame is blind, WINAPI is crippled. Better to write something that just works."**

## 🚀 Features

- **🪶 Featherweight**: 14KB, zero dependencies, pure C.
- **⚡ Brutal Performance**: Native C speed, parses in seconds.
- **🎯 Precision Focus**: Not here to replace FFmpeg, just to fill the "feature gap" in lightweight libraries.
- **⚖️ Clear Architecture**: Perfect complement to players like SDL2. You handle playback, I handle reading.
- **⚠️ Brutally Honest**: Explicitly supports **only the Big Four formats**. Terms of Service are "take it or leave it".

## 📦 Usage (Python Example)

```python
from ctypes import CDLL

# Load DLL
audio = CDLL('./audio_parser.dll')

# Auto-detect format (Recommended)
duration_seconds = audio.GetAudioDuration(b"path/to/your/audio.mp3")
print(f"Duration: {duration_seconds} seconds")

# Or use format-specific functions (More Efficient)
duration_seconds = audio.GetMp3Duration(b"path/to/your/audio.mp3")
duration_seconds = audio.GetFlacDuration(b"path/to/your/audio.flac")
# ... and so on for other formats
```

## 🤔 Why This Exists? (The "Wheel Manifesto")

| Alternative | Our Edge | Their Flaw |
| :--- | :--- | :--- |
| **FFmpeg / pydub** | **14KB**, Zero Dependencies, Plug & Play | **~260MB** Dependency Hell, Deployment Nightmare |
| **Pygame / PySDL2** | **Accurately Reads Duration**, Multi-format | **Can only play, not read**, "Blind" to metadata |
| **WINAPI Calls** | **Simple Interface**, Focused Functionality | **Cripplingly Complex**, Poor Compatibility |
| **Other Python Audio Libs** | **Native C Performance**, Blazing Fast Parsing | **Python Loops**, Sluggish Speed |

**Conclusion**: Between "lightweight" and "feature-complete", we chose a third path—**using the right tool combination to solve a specific pain point.**

## 🛠️ Technical Architecture

```
Your Python App
    ↓ (ctypes call)
audio_parser.dll (This lib, 14KB, handles metadata parsing)
    ↓
Your Audio Files (MP3/OGG/FLAC/WAV)
    ↑
SDL2.dll (2.67MB, handles playback, decoupled from this lib)
```

## 🚫 Limitations & Terms ("Love It or Leave It" Edition)

1.  **Formats**: Explicitly supports **MP3, OGG, FLAC, WAV**. **No AAC, etc.** Don't ask, the answer is "because we can".
2.  **Nature**: This is a **personal technical exercise**, not a commercial-grade product.
3.  **Liability**: We strive for reliable code but take **no responsibility for your data loss**. **See Terms below.**

## 📜 Terms of Service (Hardcore Simplified)

> This library is provided "as is", a technical experiment. Using it means:
> 1.  You understand this is a **non-commercial project with no SLA guarantee**.
> 2.  You agree **NOT to use it with any important or sensitive data**.
> 3.  You agree that **our liability for any direct or indirect loss from using this library is capped at ZERO dollars**.
> 4.  If you disagree with these terms, **delete this library immediately**.

## 🏰 About The Author & Project Context

**DVS** - A 10-year-old Chinese full-stack developer.

- **Website**: [DVS Cloud Disk dvsyun.top](https://dvsyun.top)
- **Email**: `me@dvsyun.top` (Read all emails, reply depending on mood)

**Project Evolution**:
`Crippled WINAPI Version` → `Pygame Frankencode` → **`ap_ds (Current)`**
(From low-level struggle to encapsulation compromise, finally reaching the path of architectural integration enlightenment)

---

**© DVS | Tools serve efficiency, not ego.**
