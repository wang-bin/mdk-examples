# MDK Vulkan 外部渲染集成改进建议

本文记录 Unity 插件验证中暴露的 MDK Vulkan 集成限制和后续改进方向。
这些是设计建议，尚未在 MDK 核心中实现，也不代表已经决定增加新的公共 API。

跨后端的问题归属、具体源码修复点和统一接口设计见
[MDK 与 Unity 集成设计复盘](MDK_UNITY_INTEGRATION_DESIGN.md)。

## 当前集成方式及代价

Unity 官方原生插件示例通过 `CommandRecordingState()` 获取 Unity 正在录制的
command buffer，在 Unity 管理的 render pass 内记录绘制命令。接口提供
`commandBufferLevel`，插件不能假定它总是 primary；本次 Windows/Tuanjie
2022.3 验证中取得的是 secondary。

Secondary command buffer 可以由多个 CPU 线程分别录制，再由 primary 执行。
它不能单独提交到队列，也不能调用当前 MDK 离屏路径使用的
`vkCmdBeginRenderPass()` / `vkCmdEndRenderPass()`。切换到 render pass 外部
并不会把 secondary 变成 primary。

当前 Unity 桥接层采用以下路径：

1. 在普通插件事件中调用 `AccessTexture()`，由 Unity 记录目标纹理的状态转换。
2. 调用 `AccessQueue(..., flush=true)`，先提交 Unity 已录制的命令，再取得受保护的队列访问。
3. 在队列回调中使用插件自己的 primary command buffer，供 MDK 开启离屏 render pass。
4. 提交后等待 fence，确认 GPU 完成，再允许复用上传资源或回收旧纹理。

这条路径已验证能显示视频，但增加了队列提交和 CPU 等待 GPU 的开销。
尚未做性能基准测试，不能据此给出具体耗时或吞吐损失。

## 已有能力与实际缺口

`include/mdk/RenderAPI.h` 的 `VulkanRenderAPI` 已提供 `render_pass`、
`beginFrame`、`currentCommandBuffer`、`endFrame` 和 `renderTargetInfo`。
`video/vulkan/VkVideo.cpp` 也已有 framebuffer 为空时不主动开启/结束
render pass 的分支。因此，改进应先评估和完善现有外部 render pass 路径，
不能简单认定 MDK 完全没有这项能力。

实际缺口是：`VulkanVideo::render()` 把纹理准备、上传、资源更新和绘制放在
一次调用中。即使跳过 begin/end render pass，也不能据此认为整次调用可以在
Unity 正在执行的 render pass 内合法录制。需要逐项确认上传、barrier、资源
生命周期以及 pipeline 与宿主 render pass 的兼容性。

## 优先改进方向

### 1. 分离资源准备与绘制

将视频帧上传、硬件帧映射、纹理状态转换和上传资源准备，与实际 draw 分开。
宿主应能在 render pass 外执行准备阶段，在 render pass 内执行只包含合法
绘制命令的阶段。两个阶段可能使用不同的 command buffer，不能缓存假定不变的
宿主 command buffer；Unity 的资源访问操作也可能使之前取得的录制状态失效。

涉及的主要位置是 `video/vulkan/VkVideo.cpp` 的 `VulkanVideo::render()`、
`ensureTextures()`、`ensureLayerTextures()`，以及
`video/out/VkVideoRenderer.cpp` 的回调适配。应同时覆盖普通视频、分层视频和
字幕/叠加路径，避免只有主视频能使用宿主 command buffer。

### 2. 明确外部 render pass 的契约

应明确区分 MDK 自建离屏 render pass 与宿主已开启的 render pass，描述：

- command buffer 的级别，以及当前是否处于 render pass 内。
- 宿主 render pass、subpass、颜色格式、采样数等 pipeline 兼容信息。
- 目标附件的 load/store、清屏、布局转换由谁负责。
- MDK 可以更改哪些绑定和动态状态，宿主如何恢复后续绘制所需的状态。

外部路径不得擅自结束宿主 render pass、提交宿主 command buffer，或录制
不允许出现在当前 pass 内的上传命令。仅设置 `render_pass` 或传入 secondary
句柄不足以建立这个契约。

先检查现有回调能否表达这些信息，再决定是否需要兼容性扩展。Dynamic rendering
可以作为后续独立能力评估，不应成为 Unity 2022.3 基础路径的强制要求。

### 3. 支持多帧并行时的资源生命周期

当前桥接层的 fence 等待还保护了 MDK 上传缓冲、uniform/vertex buffer、
descriptor 等资源的复用。若取消逐次等待，需要先审查这些资源是否可能在
上一帧仍被 GPU 使用时被写入或释放。

建议按在途帧管理可变资源，并提供明确的“提交完成”通知或延迟回收约定。
Unity 的 `currentFrameNumber` / `safeFrameNumber` 可用于跟踪由 Unity 提交的
资源；插件自己提交的工作仍须用自己的 fence 或等效完成信号跟踪。
纹理版本确认也必须保留，不能用 CPU 已录制完成代替 GPU 使用结束。

### 4. 减少额外提交和同步

完成前述拆分后，优先评估把绘制接入 Unity 当前 render pass，由 Unity 统一
提交。对于仍需自己提交 primary 的路径，可评估批量提交、多帧资源和 GPU
同步方案，减少 `flush` 及 CPU fence 等待。

不能只删除 `vkWaitForFences()`：那会失去当前资源复用和释放的安全保证。
同一队列的提交顺序也不能代替所有内存依赖；仍须明确上传到采样、附件写入到
Unity 后续读取之间的 barrier、访问范围和布局跟踪。

### 5. 保留硬解 `copy=1` 的独立约束

当前 Unity Vulkan 集成的硬解只支持 `copy=1`：Windows 使用
`D3D11:copy=1`，Android 使用 `AMediaCodec:copy=1`，Linux 使用
`VAAPI:copy=1`，并保留软件解码回退。实际硬解验证目前仅覆盖 Windows。

支持 secondary command buffer 或消除 CPU 等待，并不会自动获得硬解零拷贝。
零拷贝需要单独处理各平台的外部内存导入、设备匹配、格式/采样支持、所有权、
同步和解码表面生命周期，应作为独立改进项目验证。

## 建议实施顺序与验收

先完善外部 render pass 契约并拆分准备/绘制阶段，再实现多帧资源和延迟回收，
最后优化提交与等待。硬解零拷贝保持独立，不作为前述改进的前置条件。

验收应覆盖 primary 离屏路径和宿主 secondary 路径；验证播放、暂停、停止、
纹理 resize、媒体切换、多播放器和销毁时的资源安全，并检查 Vulkan validation。
对照运行应区分 Unity 自身的校验错误与插件新增错误。性能比较至少记录 CPU
录制耗时、GPU 视频绘制耗时、队列提交次数及 CPU 等待时间，保持媒体、解码器
和输出分辨率一致，不能仅凭 FPS 判断优化效果。

## 参考

- [Unity 官方 Vulkan 原生插件示例](https://github.com/Unity-Technologies/NativeRenderingPlugin/blob/master/PluginSource/source/RenderAPI_Vulkan.cpp)
- [Unity Vulkan 插件接口及回调约束](https://github.com/Unity-Technologies/NativeRenderingPlugin/blob/master/PluginSource/source/Unity/IUnityGraphicsVulkan.h)
- [Khronos：command buffer 与多线程录制](https://docs.vulkan.org/samples/latest/samples/performance/command_buffer_usage/README.html)
