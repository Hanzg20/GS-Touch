# LIte版系统需求设计文档 V0.2

**版本：** v2.0
**日期：** 2025-10-30
**项目：** GoldSky\_Lite洗车终端优化升级
**编写：** Eaglson Development Team

---

## 🎯 1. 项目概述

## 1.1 项目背景

基于Nayax VPOS操作流程分析和业界POS机标准，对现有Eaglson洗车终端进行用户体验优化，简化操作流程，减少用户操作次数，提升服务效率。

## 1.2 优化目标

* **流程简化**：从原有8步操作简化为5步
* **指示明确**：采用4LED指示灯系统提供清晰的状态反馈
* **用户友好**：减少等待时间，自动化处理流程
* **标准对齐**：符合业界POS机操作标准

## 1.3 设计原则

* **一键到位**：关键操作一次完成
* **自动进阶**：成功后自动进入下一步
* **清晰反馈**：LED灯组合提供状态信息
* **容错处理**：异常情况自动恢复

---

## 🔄 2. 业务流程设计

## 2.1 流程对比分析


| 对比项       | 原版流程 | 优化后流程 | 改进点       |
| ------------ | -------- | ---------- | ------------ |
| **操作步骤** | 8步      | 5步        | 简化37.5%    |
| **用户按键** | 12次     | 6次        | 减少50%      |
| **确认界面** | 3个      | 1个        | 减少等待时间 |
| **自动流程** | 2个      | 4个        | 提高自动化   |

## 2.2 详细流程设计

## **步骤0️⃣：欢迎屏幕**

**功能定位**：品牌展示和服务介绍
**用户体验**：无需操作，按OK进入服务

**界面内容：**

<pre class="not-prose w-full rounded font-mono text-sm font-extralight"><div class="codeWrapper text-light selection:text-super selection:bg-super/10 my-md relative flex flex-col rounded font-mono text-sm font-normal bg-subtler"><div class="translate-y-xs -translate-x-xs bottom-xl mb-xl flex h-0 items-start justify-end md:sticky md:top-[100px]"><div class="overflow-hidden rounded-full border-subtlest ring-subtlest divide-subtlest bg-base"><div class="border-subtlest ring-subtlest divide-subtlest bg-subtler"></div></div></div><div class="-mt-xl"><div><div data-testid="code-language-indicator" class="text-quiet bg-subtle py-xs px-sm inline-block rounded-br rounded-tl-[3px] font-thin">text</div></div><div><span><code><span><span>┌────────────────────────────┐
</span></span><span>│ ░░ EAGLSON COIN WASH ░░    │
</span><span>│   Professional Car Care    │
</span><span>├────────────────────────────┤
</span><span>│ 🚗 24小时自助洗车服务      │
</span><span>│ 💳 会员享优惠价格          │
</span><span>│ ⚡ 快捷·安全·高效          │
</span><span>│                            │
</span><span>│ Press OK to Start Service  │
</span><span>└────────────────────────────┘
</span><span></span></code></span></div></div></div></pre>

**LED状态：** L1亮 + L2亮/闪 + L3灭 + L4灭
**操作：** OK键启动服务
**超时：** 无限循环显示

## **步骤1️⃣：选择套餐**

**功能定位**：用户选择洗车服务类型
**用户体验**：SELECT切换，OK确认

**界面内容：**

<pre class="not-prose w-full rounded font-mono text-sm font-extralight"><div class="codeWrapper text-light selection:text-super selection:bg-super/10 my-md relative flex flex-col rounded font-mono text-sm font-normal bg-subtler"><div class="translate-y-xs -translate-x-xs bottom-xl mb-xl flex h-0 items-start justify-end md:sticky md:top-[100px]"><div class="overflow-hidden rounded-full border-subtlest ring-subtlest divide-subtlest bg-base"><div class="border-subtlest ring-subtlest divide-subtlest bg-subtler"></div></div></div><div class="-mt-xl"><div><div data-testid="code-language-indicator" class="text-quiet bg-subtle py-xs px-sm inline-block rounded-br rounded-tl-[3px] font-thin">text</div></div><div><span><code><span><span>┌────────────────────────────┐
</span></span><span>│ Select Your Package        │
</span><span>├────────────────────────────┤
</span><span>│ > Quick Wash    $4.00  4min│ ← 当前选中
</span><span>│   Standard      $6.00  6min│
</span><span>│   Deluxe        $9.00  8min│
</span><span>│   Premium      $12.00 10min│
</span><span>│                            │
</span><span>│ SELECT=Next  OK=Confirm    │
</span><span>└────────────────────────────┘
</span><span></span></code></span></div></div></div></pre>

**LED状态：** L1亮 + L2亮/闪 + L3**亮** + L4灭
**操作：** SELECT循环选择，OK确认
**超时：** 20秒返回欢迎屏幕

## **步骤2️⃣：刷卡支付**

**功能定位**：一次刷卡完成验证+扣费
**用户体验**：刷卡后自动处理，无需额外确认

**界面内容（等待刷卡）：**

<pre class="not-prose w-full rounded font-mono text-sm font-extralight"><div class="codeWrapper text-light selection:text-super selection:bg-super/10 my-md relative flex flex-col rounded font-mono text-sm font-normal bg-subtler"><div class="translate-y-xs -translate-x-xs bottom-xl mb-xl flex h-0 items-start justify-end md:sticky md:top-[100px]"><div class="overflow-hidden rounded-full border-subtlest ring-subtlest divide-subtlest bg-base"><div class="border-subtlest ring-subtlest divide-subtlest bg-subtler"></div></div></div><div class="-mt-xl"><div><div data-testid="code-language-indicator" class="text-quiet bg-subtle py-xs px-sm inline-block rounded-br rounded-tl-[3px] font-thin">text</div></div><div><span><code><span><span>┌────────────────────────────┐
</span></span><span>│ Please Tap Your Card       │
</span><span>├────────────────────────────┤
</span><span>│                            │
</span><span>│      ◯ ◯ ◯                 │ ← NFC动画
</span><span>│                            │
</span><span>│ Selected: Standard $6.00   │
</span><span>│                            │
</span><span>│ Waiting for card...        │
</span><span>└────────────────────────────┘
</span><span></span></code></span></div></div></div></pre>

**界面内容（处理中1.5秒）：**

<pre class="not-prose w-full rounded font-mono text-sm font-extralight"><div class="codeWrapper text-light selection:text-super selection:bg-super/10 my-md relative flex flex-col rounded font-mono text-sm font-normal bg-subtler"><div class="translate-y-xs -translate-x-xs bottom-xl mb-xl flex h-0 items-start justify-end md:sticky md:top-[100px]"><div class="overflow-hidden rounded-full border-subtlest ring-subtlest divide-subtlest bg-base"><div class="border-subtlest ring-subtlest divide-subtlest bg-subtler"></div></div></div><div class="-mt-xl"><div><div data-testid="code-language-indicator" class="text-quiet bg-subtle py-xs px-sm inline-block rounded-br rounded-tl-[3px] font-thin">text</div></div><div><span><code><span><span>┌────────────────────────────┐
</span></span><span>│ Processing Payment...      │
</span><span>├────────────────────────────┤
</span><span>│ ✓ Card: ****8116           │
</span><span>│ ✓ Package: Standard        │
</span><span>│ ✓ Amount: $6.00            │
</span><span>│ ✓ New Balance: $70.00      │
</span><span>│                            │
</span><span>│ Starting wash service...   │
</span><span>└────────────────────────────┘
</span><span></span></code></span></div></div></div></pre>

**LED状态：**

* 等待时：L1亮 + L2亮/闪 + L3亮 + L4**快闪**
* 处理时：L1亮 + L2**快闪** + L3**亮** + L4亮

**自动流程：** 读卡→验证→扣费→自动进入步骤4
**超时：** 15秒无刷卡返回欢迎屏幕

## **步骤3️⃣：系统准备**

**功能定位**：设备准备和启动确认
**用户体验**：自动显示1.5秒后进入洗车

**界面内容：**

<pre class="not-prose w-full rounded font-mono text-sm font-extralight"><div class="codeWrapper text-light selection:text-super selection:bg-super/10 my-md relative flex flex-col rounded font-mono text-sm font-normal bg-subtler"><div class="translate-y-xs -translate-x-xs bottom-xl mb-xl flex h-0 items-start justify-end md:sticky md:top-[100px]"><div class="overflow-hidden rounded-full border-subtlest ring-subtlest divide-subtlest bg-base"><div class="border-subtlest ring-subtlest divide-subtlest bg-subtler"></div></div></div><div class="-mt-xl"><div><div data-testid="code-language-indicator" class="text-quiet bg-subtle py-xs px-sm inline-block rounded-br rounded-tl-[3px] font-thin">text</div></div><div><span><code><span><span>┌────────────────────────────┐
</span></span><span>│ System Ready               │
</span><span>├────────────────────────────┤
</span><span>│ ✓ Payment Confirmed        │
</span><span>│ ✓ Equipment Check          │
</span><span>│ ✓ Water System Ready       │
</span><span>│                            │
</span><span>│ Initializing wash...       │
</span><span>│ ▓▓▓▓▓▓▓▓░░ 80%             │
</span><span>└────────────────────────────┘
</span><span></span></code></span></div></div></div></pre>

**LED状态：** L1亮 + L2亮/闪 + L3亮 + L4**亮**
**自动流程：** 设备自检→自动进入洗车模式

## **步骤4️⃣：洗车进行**

**功能定位**：洗车设备运行控制
**用户体验**：实时显示进度和倒计时

**界面内容：**

<pre class="not-prose w-full rounded font-mono text-sm font-extralight"><div class="codeWrapper text-light selection:text-super selection:bg-super/10 my-md relative flex flex-col rounded font-mono text-sm font-normal bg-subtler"><div class="translate-y-xs -translate-x-xs bottom-xl mb-xl flex h-0 items-start justify-end md:sticky md:top-[100px]"><div class="overflow-hidden rounded-full border-subtlest ring-subtlest divide-subtlest bg-base"><div class="border-subtlest ring-subtlest divide-subtlest bg-subtler"></div></div></div><div class="-mt-xl"><div><div data-testid="code-language-indicator" class="text-quiet bg-subtle py-xs px-sm inline-block rounded-br rounded-tl-[3px] font-thin">text</div></div><div><span><code><span><span>┌────────────────────────────┐
</span></span><span>│ Wash in Progress           │
</span><span>├────────────────────────────┤
</span><span>│                            │
</span><span>│        05:24               │ ← 大字体倒计时
</span><span>│                            │
</span><span>│    Pulse 3/8               │ ← 脉冲进度
</span><span>│ ╔══════════════════════════╗│
</span><span>│ ║████████░░░░░░░░░░░░░░░░░░║│ ← 进度条
</span><span>│ ╚══════════════════════════╝│
</span><span>└────────────────────────────┘
</span><span></span></code></span></div></div></div></pre>

**LED状态：** L1**亮** + L2**亮** + L3**亮** + L4**亮**（四灯全亮）
**脉冲控制：** 每1秒发送100ms脉冲
**自动完成：** 时间到或脉冲完成自动进入步骤5

## **步骤5️⃣：服务完成**

**功能定位**：完成确认和感谢
**用户体验**：显示8秒后自动返回欢迎屏幕

**界面内容：**

<pre class="not-prose w-full rounded font-mono text-sm font-extralight"><div class="codeWrapper text-light selection:text-super selection:bg-super/10 my-md relative flex flex-col rounded font-mono text-sm font-normal bg-subtler"><div class="translate-y-xs -translate-x-xs bottom-xl mb-xl flex h-0 items-start justify-end md:sticky md:top-[100px]"><div class="overflow-hidden rounded-full border-subtlest ring-subtlest divide-subtlest bg-base"><div class="border-subtlest ring-subtlest divide-subtlest bg-subtler"></div></div></div><div class="-mt-xl"><div><div data-testid="code-language-indicator" class="text-quiet bg-subtle py-xs px-sm inline-block rounded-br rounded-tl-[3px] font-thin">text</div></div><div><span><code><span><span>┌────────────────────────────┐
</span></span><span>│    Wash Complete!          │
</span><span>├────────────────────────────┤
</span><span>│                            │
</span><span>│         ✓                  │ ← 大号对勾
</span><span>│                            │
</span><span>│     Thank You!             │
</span><span>│   Drive Safely             │
</span><span>│                            │
</span><span>└────────────────────────────┘
</span><span></span></code></span></div></div></div></pre>

**LED状态：** L1亮 + L2亮/闪 + L3**慢闪** + L4灭
**声音反馈：** 双短鸣 + 停顿 + 双短鸣
**自动返回：** 8秒后回到欢迎屏幕

---

## 💡 3. LED指示灯系统设计

## 3.1 硬件定义


| LED    | GPIO  | 功能定义       | 主要状态             |
| ------ | ----- | -------------- | -------------------- |
| **L1** | GPIO3 | **电源指示灯** | 常亮=正常供电        |
| **L2** | GPIO6 | **网络指示灯** | 亮=WiFi连接，闪=离线 |
| **L3** | GPIO5 | **进度指示灯** | 随流程步骤依次点亮   |
| **L4** | GPIO7 | **状态指示灯** | 交易/故障状态指示    |

## 3.2 LED状态矩阵


| 流程步骤     | L1电源   | L2网络     | L3进度     | L4状态     | 系统状态描述       |
| ------------ | -------- | ---------- | ---------- | ---------- | ------------------ |
| **0-欢迎**   | 🟢常亮   | 🔵亮/闪    | ⚫灭       | ⚫灭       | 系统待机，等待用户 |
| **1-选套餐** | 🟢常亮   | 🔵亮/闪    | 🟡**亮**   | ⚫灭       | 步骤1完成          |
| **2-等刷卡** | 🟢常亮   | 🔵亮/闪    | 🟡亮       | 🟡**快闪** | 等待用户刷卡       |
| **2-验证中** | 🟢常亮   | 🔵**快闪** | 🟡**亮**   | 🟡亮       | 步骤2完成          |
| **3-准备中** | 🟢常亮   | 🔵亮/闪    | 🟡亮       | 🟡**亮**   | 步骤3完成          |
| **4-洗车中** | 🟢**亮** | 🔵**亮**   | 🟡**亮**   | 🟡**亮**   | **四灯全亮运行**   |
| **5-完成**   | 🟢常亮   | 🔵亮/闪    | 🟡**慢闪** | ⚫灭       | 服务完成提示       |
| **错误状态** | 🟢常亮   | 🔵亮/闪    | ⚫灭       | 🔴**快闪** | 系统故障           |

## 3.3 LED控制逻辑

<pre class="not-prose w-full rounded font-mono text-sm font-extralight"><div class="codeWrapper text-light selection:text-super selection:bg-super/10 my-md relative flex flex-col rounded font-mono text-sm font-normal bg-subtler"><div class="translate-y-xs -translate-x-xs bottom-xl mb-xl flex h-0 items-start justify-end md:sticky md:top-[100px]"><div class="overflow-hidden rounded-full border-subtlest ring-subtlest divide-subtlest bg-base"><div class="border-subtlest ring-subtlest divide-subtlest bg-subtler"></div></div></div><div class="-mt-xl"><div><div data-testid="code-language-indicator" class="text-quiet bg-subtle py-xs px-sm inline-block rounded-br rounded-tl-[3px] font-thin">cpp</div></div><div><span><code><span class="token token">enum</span><span> </span><span class="token token">LEDStatus</span><span> </span><span class="token token punctuation">{</span><span>
</span><span>  LED_OFF</span><span class="token token punctuation">,</span><span>           </span><span class="token token">// 熄灭</span><span>
</span><span>  LED_ON</span><span class="token token punctuation">,</span><span>            </span><span class="token token">// 常亮</span><span>
</span><span>  LED_BLINK_SLOW</span><span class="token token punctuation">,</span><span>    </span><span class="token token">// 慢闪 (1Hz)</span><span>
</span><span>  LED_BLINK_FAST     </span><span class="token token">// 快闪 (5Hz)</span><span>
</span><span></span><span class="token token punctuation">}</span><span class="token token punctuation">;</span><span>
</span>
<span></span><span class="token token">struct</span><span> </span><span class="token token">SystemLEDs</span><span> </span><span class="token token punctuation">{</span><span>
</span><span>  </span><span class="token token">bool</span><span> power</span><span class="token token punctuation">;</span><span>                    </span><span class="token token">// L1: 电源灯</span><span>
</span><span>  LEDStatus network</span><span class="token token punctuation">;</span><span>             </span><span class="token token">// L2: 网络灯</span><span>
</span><span>  LEDStatus progress</span><span class="token token punctuation">;</span><span>            </span><span class="token token">// L3: 进度灯</span><span>
</span><span>  LEDStatus status</span><span class="token token punctuation">;</span><span>              </span><span class="token token">// L4: 状态灯</span><span>
</span><span></span><span class="token token punctuation">}</span><span class="token token punctuation">;</span><span>
</span></code></span></div></div></div></pre>

---

## ⚙️ 4. 技术规范

## 4.1 硬件配置

**主控制器：** ESP32-S3开发板
**显示屏：** 128×64 OLED（I2C接口）
**NFC模块：** RC522读卡器（SPI接口）
**用户界面：** 2个按钮（OK + SELECT）
**指示系统：** 4个LED指示灯 + 蜂鸣器
**通信接口：** WiFi网络连接

## 4.2 引脚配置

<pre class="not-prose w-full rounded font-mono text-sm font-extralight"><div class="codeWrapper text-light selection:text-super selection:bg-super/10 my-md relative flex flex-col rounded font-mono text-sm font-normal bg-subtler"><div class="translate-y-xs -translate-x-xs bottom-xl mb-xl flex h-0 items-start justify-end md:sticky md:top-[100px]"><div class="overflow-hidden rounded-full border-subtlest ring-subtlest divide-subtlest bg-base"><div class="border-subtlest ring-subtlest divide-subtlest bg-subtler"></div></div></div><div class="-mt-xl"><div><div data-testid="code-language-indicator" class="text-quiet bg-subtle py-xs px-sm inline-block rounded-br rounded-tl-[3px] font-thin">cpp</div></div><div><span><code><span class="token token">// 显示和通信</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">I2C_SDA</span><span class="token token macro property"> </span><span class="token token macro property expression">8</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">I2C_SCL</span><span class="token token macro property"> </span><span class="token token macro property expression">9</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">RC522_CS</span><span class="token token macro property"> </span><span class="token token macro property expression">10</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">SPI_MOSI</span><span class="token token macro property"> </span><span class="token token macro property expression">11</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">SPI_SCK</span><span class="token token macro property"> </span><span class="token token macro property expression">12</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">SPI_MISO</span><span class="token token macro property"> </span><span class="token token macro property expression">13</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">RC522_RST</span><span class="token token macro property"> </span><span class="token token macro property expression">14</span><span>
</span>
<span></span><span class="token token">// 用户界面</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">BTN_OK</span><span class="token token macro property"> </span><span class="token token macro property expression">1</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">BTN_SELECT</span><span class="token token macro property"> </span><span class="token token macro property expression">2</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">BUZZER</span><span class="token token macro property"> </span><span class="token token macro property expression">16</span><span>
</span>
<span></span><span class="token token">// LED指示灯系统</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">LED_POWER</span><span class="token token macro property"> </span><span class="token token macro property expression">3</span><span class="token token macro property expression">      </span><span class="token token macro property">// L1: 电源灯</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">LED_NETWORK</span><span class="token token macro property"> </span><span class="token token macro property expression">6</span><span class="token token macro property expression">    </span><span class="token token macro property">// L2: 网络灯  </span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">LED_PROGRESS</span><span class="token token macro property"> </span><span class="token token macro property expression">5</span><span class="token token macro property expression">   </span><span class="token token macro property">// L3: 进度灯</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">LED_STATUS</span><span class="token token macro property"> </span><span class="token token macro property expression">7</span><span class="token token macro property expression">     </span><span class="token token macro property">// L4: 状态灯</span><span>
</span>
<span></span><span class="token token">// 控制输出</span><span>
</span><span></span><span class="token token macro property directive-hash">#</span><span class="token token macro property directive">define</span><span class="token token macro property"> </span><span class="token token macro property macro-name">PULSE_OUT</span><span class="token token macro property"> </span><span class="token token macro property expression">7</span><span>
</span></code></span></div></div></div></pre>

## 4.3 软件架构

**开发环境：** Arduino IDE 2.x
**核心库：**

* WiFi.h（网络通信）
* Wire.h（I2C通信）
* SPI.h（SPI通信）
* U8g2lib.h（OLED显示）
* MFRC522.h（NFC读卡）
* ArduinoJson.h（数据解析）

**后端服务：** Supabase PostgreSQL数据库
**API通信：** RESTful HTTPS接口

## 4.4 数据库集成

**会员卡表：** jc\_vip\_cards

* card\_uid (主键)
* card\_credit (余额)
* is\_active (状态)
* cardholder\_name (持卡人)

**交易记录表：** jc\_transaction\_history

* machine\_id (设备ID)
* card\_uid (卡片ID)
* transaction\_amount (交易金额)
* balance\_before/after (余额变化)

---

## 📊 5. 性能指标

## 5.1 响应时间要求


| 操作类型 | 目标时间 | 最大时间   |
| -------- | -------- | ---------- |
| 按键响应 | 50ms     | 100ms      |
| NFC读卡  | 0.5s     | 1.0s       |
| 网络验证 | 1.0s     | 3.0s       |
| 脉冲发送 | 1.0s间隔 | ±50ms精度 |

## 5.2 用户体验指标


| 指标项        | 目标值 | 说明             |
| ------------- | ------ | ---------------- |
| 完整流程时间  | <30秒  | 从选择到开始洗车 |
| 按键操作次数  | ≤6次  | 包括选择和确认   |
| 错误恢复时间  | <3秒   | 异常后自动返回   |
| LED状态识别度 | 100%   | 清晰区分各种状态 |

## 5.3 可靠性要求

**系统可用率：** ≥99.9%
**连续运行时间：** ≥24小时
**网络中断恢复：** 自动重连
**异常处理：** 自动重置到安全状态

---

## 🔧 6. 实施计划

## 6.1 开发阶段

**Phase 1 - 核心功能开发（3天）**

* LED控制系统实现
* 简化流程状态机
* 基础UI界面更新

**Phase 2 - 集成测试（2天）**

* 硬件集成测试
* 流程完整性验证
* 异常情况测试

**Phase 3 - 用户体验优化（1天）**

* 界面细节调优
* LED效果调试
* 声音反馈完善

## 6.2 验收标准

* ✅ 所有LED指示灯按设计正确显示
* ✅ 用户操作次数减少到6次以内
* ✅ 完整流程测试通过
* ✅ 异常情况自动恢复
* ✅ 代码质量达到生产标准

## 6.3 部署要求

**硬件检查：** 确认4个LED正确连接到指定GPIO
**软件更新：** 上传优化后的完整程序
**功能验证：** 现场完整流程测试
**用户培训：** LED指示灯含义说明

---

## 📝 7. 套餐配置


| 套餐名称 | 英文名     | 价格    | 时长   | 脉冲数 |
| -------- | ---------- | ------- | ------ | ------ |
| 快速洗车 | Quick Wash | \$4.00  | 4分钟  | 4脉冲  |
| 标准洗车 | Standard   | \$6.00  | 6分钟  | 8脉冲  |
| 豪华洗车 | Deluxe     | \$9.00  | 8分钟  | 9脉冲  |
| 至尊洗车 | Premium    | \$12.00 | 10分钟 | 12脉冲 |

---

## 🔄 8. 状态机设计

<pre class="not-prose w-full rounded font-mono text-sm font-extralight"><div class="codeWrapper text-light selection:text-super selection:bg-super/10 my-md relative flex flex-col rounded font-mono text-sm font-normal bg-subtler"><div class="translate-y-xs -translate-x-xs bottom-xl mb-xl flex h-0 items-start justify-end md:sticky md:top-[100px]"><div class="overflow-hidden rounded-full border-subtlest ring-subtlest divide-subtlest bg-base"><div class="border-subtlest ring-subtlest divide-subtlest bg-subtler"></div></div></div><div class="-mt-xl"><div><div data-testid="code-language-indicator" class="text-quiet bg-subtle py-xs px-sm inline-block rounded-br rounded-tl-[3px] font-thin">cpp</div></div><div><span><code><span class="token token">enum</span><span> </span><span class="token token">SystemState</span><span> </span><span class="token token punctuation">{</span><span>
</span><span>  STATE_WELCOME</span><span class="token token punctuation">,</span><span>           </span><span class="token token">// 0. 欢迎屏幕</span><span>
</span><span>  STATE_SELECT_PACKAGE</span><span class="token token punctuation">,</span><span>    </span><span class="token token">// 1. 选择套餐</span><span>
</span><span>  STATE_CARD_SCAN</span><span class="token token punctuation">,</span><span>         </span><span class="token token">// 2. 刷卡支付（自动验证+扣费）</span><span>
</span><span>  STATE_SYSTEM_READY</span><span class="token token punctuation">,</span><span>      </span><span class="token token">// 3. 系统准备</span><span>
</span><span>  STATE_PROCESSING</span><span class="token token punctuation">,</span><span>        </span><span class="token token">// 4. 洗车进行</span><span>
</span><span>  STATE_COMPLETE</span><span class="token token punctuation">,</span><span>          </span><span class="token token">// 5. 服务完成</span><span>
</span><span>  STATE_ERROR              </span><span class="token token">// 错误状态</span><span>
</span><span></span><span class="token token punctuation">}</span><span class="token token punctuation">;</span></code></span></div></div></div></pre>
