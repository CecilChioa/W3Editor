# W3Editor 新项目规划：64 位现代化 YDWE war3经典版地形编辑器

## 1. 项目定位

`W3Editor` 是一个面向 Warcraft III 地图开发的 64 位现代化编辑器项目，目标是逐步替代/升级 YDWE 的核心编辑能力。

第一阶段不追求完整复刻 YDWE，而是优先打通最小闭环：

```text
新建地图 → 打开地图 → 渲染地形 → 编辑地形 → 保存地图 → 再次打开验证
```

该闭环稳定后，再扩展单位、装饰物、物体编辑器、触发器、导入管理器、LNI/YDWE 兼容链路。

## 2. 旧项目参考

旧编辑器目录：

```text
C:\war3\mytool\W3EditorOld
```

可参考内容：

- `src/file_formats/`：MPQ、SLK、MDX 等格式处理经验。
- `src/base/map/`：地图结构相关逻辑。
- `src/main_window/glwidget.cpp`：旧 OpenGL 视图实现经验。
- `data/shaders/terrain.*`、`water.*`、`cliff.*`：地形、水面、悬崖 shader 参考。
- `data/icons/terrain/`：地形工具图标可复用。
- `data/test map/`：由外部工具解包得到的目录地图，只能作为调试样本和格式对照，不能作为主输入假设。
- `third_party/w3x2lni`、`vendor/w3x2lni-runtime`：后续兼容 LNI/YDWE 工作流时参考，但第一阶段不得依赖它们完成解包。

注意：旧项目只作为参考，不直接继续在旧架构上修补。

## 3. 工作区权限边界与测试样本

### 3.1 工作区权限边界

- 完全授权可写范围仅限 `C:\war3\mytool\W3Editor\` 及其子目录。
- `C:\war3\mytool\W3Editor\` 之外的任何目录仅允许只读访问或复制文件到本项目目录内，不允许修改、删除、覆盖或生成文件。
- `C:\war3\mytool\W3EditorOld`、`C:\war3\mytool\FIM\YDWE-master`、`C:\war3\mytool\FIM\w3x2lni-master` 等外部目录均视为只读参考源。
- 如需处理外部参考文件，必须先复制到 `C:\war3\mytool\W3Editor\` 下的明确位置。
- 构建产物、测试产物、日志、诊断快照只能写入本项目目录下，例如 `build/`、`build/artifacts/`、`build/logs/`、`diagnostics/`。

### 3.2 测试样本地图

真实 `.w3x` 样本目录：

```text
C:\war3\mytool\W3Editor\testmap
```

当前样本：

```text
C:\war3\mytool\W3Editor\testmap\九种兵器3.w3x
C:\war3\mytool\W3Editor\testmap\testdemo.w3x
C:\war3\mytool\W3Editor\testmap\testmap1.w3x
```

使用规则：

- 第一阶段自动化测试优先使用 `C:\war3\mytool\W3Editor\testmap\testdemo.w3x` 作为默认样本，同时保留另外两个样本做兼容性回归。
- 测试必须直接读取 `.w3x` 包内文件，不允许依赖外部解包工具。
- 不得覆盖、修改或删除 `testmap` 中的原始样本地图。
- round-trip 输出、导出目录、dump、日志和诊断文件必须写入 `build/artifacts/`、`build/logs/` 或 `diagnostics/`。

## 4. 项目文档维护机制

规则文件和规划文档必须随着项目推进同步维护，避免后续实现继续引用过期假设。

维护要求：

- 当架构边界、阶段目标、依赖选择、测试样本、构建方式、CLI 行为、日志观测或 UI 规则发生变化时，必须同步更新 `.cursor/rules/w3editor-project.mdc` 和本文档。
- 当代码实现与既有规划不一致时，先判断是实现偏离还是规划过期；若规划过期，应以当前可验证实现和用户最新决策为准修正文档。
- 每个阶段完成后，应在本文档中记录已完成能力、未完成风险、下一阶段入口和验收方式。
- 规则文件负责提供硬约束；本文档负责记录阶段规划、设计依据、落地顺序和验收标准。

Git 与忽略文件维护：

- 只推送已完成构建、测试或明确人工验证的代码与文档；未验证实验代码不得进入主分支。
- 推送前必须检查 `git status` 和待提交 diff，确认没有构建产物、日志、诊断快照、临时文件或外部参考目录内容混入。
- `.gitignore` 必须覆盖 `build/`、`build/artifacts/`、`build/logs/`、`diagnostics/`、CMake/vcpkg 生成目录、IDE 本地配置、二进制产物和 round-trip 派生产物。
- `testmap` 中的原始样本地图作为第一阶段测试输入管理，不得被测试覆盖；由测试生成的地图、dump、日志和诊断文件不得提交。
- 提交或推送前如发现规则、规划或 `.gitignore` 与当前工程状态不一致，必须先同步更新。

## 5. 二次分析结论：旧编辑器与 YDWE 的使用边界

### 5.1 `W3EditorOld` 结论

旧编辑器已经证明了一条可行链路：Qt + OpenGL 视口、地形 SoA 数据、地形刷子、shader、资源加载、测试地图都存在可参考实现。

可复用方向：

- `src/base/terrain.ixx`：地形 corner 字段定义可参考，包括高度、水高、地表贴图、贴图变化、悬崖贴图、层高、边界、坡道、荒芜、浅水、边界、悬崖、romp、特殊装饰物等。
- `src/brush/terrain_operators.cpp`：抬高、降低、平整、平滑、贴图、悬崖/水面等工具行为可参考。
- `src/main_window/glwidget.cpp`：Qt + OpenGL 视口、调试信息、相机和鼠标拾取经验可参考。
- `data/shaders/`：地形、水面、悬崖 shader 可参考。
- `data/icons/terrain/`：地形工具图标可迁移。
- `data/test map/`：作为解包后的对照样本，不作为 M0/M1 主测试输入。

必须规避的问题：

- 旧 `Map` 类同时持有地图数据、资源、渲染、物理、UI、brush、undo 和大量编辑器状态，耦合过重。
- 旧 `Terrain` 同时依赖 Qt、OpenGL、Bullet、资源管理、SLK 和渲染缓冲，不适合作为新项目核心模型。
- 旧 `TerrainOperator` 直接访问全局 `map`、Qt `QRect`、渲染更新和 undo，不能照搬到新架构。
- 新项目必须把 `TerrainModel` 做成纯数据/纯逻辑层，渲染、资源、UI、物理全部外置。
- 旧代码可以参考字段和算法，不应直接复制模块结构。

落地到新项目：

- `TerrainCorner` 至少覆盖旧项目已验证字段：`height`、`waterHeight`、`groundTexture`、`groundVariation`、`cliffVariation`、`cliffTexture`、`layerHeight`、`mapEdge`、`ramp`、`blight`、`water`、`boundary`、`cliff`、`romp`、`specialDoodad`。
- `TerrainModel` 允许采用 SoA 存储以便局部刷新和批量刷子计算，但对外提供清晰的 `getCorner/setCorner` 或等价接口。
- 高度计算需保留 War3 语义：最终地面高度类似 `height + layerHeight - 2.0`，最终水面高度由 `waterHeight` 与水面偏移共同决定。
- 抬高/降低/平整/平滑/贴图/水面工具先按旧项目行为复刻可用体验，但实现必须拆成 `TerrainBrush`、`TerrainEditCommand`、`TerrainUndoStack`、`TerrainEditorController`。
- 所有编辑操作应返回最小脏区域，供 `TerrainRenderer`、`WaterRenderer`、`WpmWriter` 局部更新使用。
- 平滑、贴图随机 variation、荒芜地表与悬崖相邻限制等 War3 兼容细节，需要在地形测试中固化。

### 5.2 `YDWE-master` 结论

YDWE 是基于原版 WorldEdit 的二次开发体系，价值主要在工作流和兼容生态，而不是地形编辑器内核。

可参考方向：

- `Development/Component/说明.txt`：YDWE 的用户价值包括兼容旧版本、插件、JASS 系统、示例地图、双开等。
- `Development/Plugin/Warcraft3/yd_virtual_mpq` 与 `Development/Plugin/Lua/virtual_mpq`：虚拟 MPQ 是 YDWE 工作流的重要组成，可作为后期资源覆盖/测试运行参考。
- `Development/Core/BlpConv`：BLP 读写实现可参考。
- `Development/Core/SlkLib`：SLK 解析、对象表加载可参考。
- `OpenSource/StormLib`：MPQ 读写可作为依赖或实现参考。
- `Development/Component/plugin/w3x2lni`：LNI/对象数据转换链路可作为后期兼容参考。
- `Development/Component/example(演示地图)`：后期可作为 YDWE 兼容测试样本。

第一阶段不得迁入：

- Detours/MinHook/hook 注入体系。
- YDTrigger GUI hook。
- Lua 插件系统。
- WorldEdit 内存 patch。
- YDWE 原启动器与 32 位兼容层。

新项目方向应是：先做独立 64 位编辑器核心；YDWE 兼容作为后续数据和工作流兼容层，而不是第一阶段内核依赖。

落地到新项目：

- 第一阶段只吸收 YDWE 的“数据格式和工作流经验”，不吸收其“注入式扩展机制”。
- BLP/SLK/对象表属于资源与后续物编基础设施，第一阶段只为地形贴图显示提供最小读取能力，不提前实现完整物体编辑器。
- `w3x2lni`、LNI、YDTrigger、JASSHelper、pjass、Lua 插件链路全部后置；当前只保留目录地图、MPQ 地图、基础 War3 文件格式的兼容入口。
- YDWE 示例地图可作为后期兼容测试集，第一阶段测试主样本应包含真实 `.w3x/.w3m` 地图包；`W3EditorOld/data/test map` 只能作为解包后的对照样本。
- 
## 6. 推荐技术栈

优先路线：

```text
C++20 + Qt 6 Widgets + CMake + vcpkg + OpenGL/RHI 抽象
```

标准策略：

- C++20 作为工程基线，所有核心模块必须在 C++20 下稳定编译。
- C++23 特性只允许在独立工具或局部模块中谨慎使用，且不得成为核心工程硬依赖。
- 不使用 C++26 特性作为项目代码依赖。

原因：

- 与旧项目技术栈接近，迁移经验成本低。
- Qt 6 支持 64 位和现代化桌面 UI。
- C++ 适合处理 War3 的二进制地图格式、MPQ、BLP、MDX、W3E 等。
- 地形渲染、刷子编辑、模型预览等性能需求较高。

建议依赖：

- UI：Qt 6 Widgets。
- 构建：CMake + vcpkg。
- 日志：spdlog。
- 数学：glm。
- MPQ：StormLib 或自封装 MPQ 层。
- 测试：doctest 或 Catch2。
- 图像：Qt Image + 独立 BLP loader。

## 7. 第一阶段 MVP 范围

第一阶段仍以地形闭环为主，但必须预留并启动地形编辑依赖的资源和预览基础设施，避免后续模型、装饰物和贴图预览能力无法接入。

地形编辑支撑能力：

- 模型批量预览：提供装饰物、单位、地形装饰和模型资源的批量索引、筛选、缩略图/预览状态和异步加载队列规划。
- 资源管理：提供独立资源索引、Archive 资源查找、BLP 贴图读取、SLK/对象表读取入口、MDX/模型资源入口和缺失资源回退策略。
- 预览缓存：预览缩略图、模型元信息、贴图元信息和失败状态必须可缓存、可失效、可通过 CLI 或测试验证。
- 自动测试优先：资源索引、批量预览列表、缓存命中、缺失资源回退和加载取消必须优先设计自动测试或 CLI 验证入口。

### 7.1 新建地图

支持：

- 地图尺寸：32x32、64x64、96x96、128x128、160x160、192x192、224x224、256x256。
- 初始地形集：洛丹伦夏季、荒芜之地、村庄、城邦等。
- 生成最小地图文件：

```text
war3map.w3i
war3map.w3e
war3map.wpm
war3map.shd
war3map.mmp
```

### 7.2 打开地图

支持：

- `.w3x/.w3m` MPQ 地图，这是第一阶段必须支持的主输入路径。
- 已解包地图目录，仅作为调试、对照和本项目导出的派生产物支持。

第一阶段必须先实现本项目自己的 Archive/MPQ 读取能力，能直接从 `.w3x/.w3m` 包内读取 `war3map.w3e`、`war3map.w3i` 等文件；不得依赖 `w3x2lni` 或其他外部解包工具作为打开地图的前置步骤。

打开时通过 `Archive` 抽象读取：

```text
war3map.w3i   地图基础信息
war3map.w3e   地形高度、贴图、悬崖、水面
war3map.wpm   路径网格
war3map.shd   阴影数据
war3map.mmp   小地图数据
```

如果非关键文件不存在，允许创建默认数据，但必须记录日志。

### 7.3 地形渲染

最低目标：

- 显示高度地形网格。
- 显示基础地表贴图。
- 显示水面。
- 显示地图边界。
- 鼠标拾取地形格点。
- 显示刷子范围。

渲染层拆分：

```text
TerrainLayer    地表网格 + 贴图
WaterLayer      水面
OverlayLayer    网格线、刷子、选区、路径显示
```

### 7.4 地形编辑工具

第一阶段只做核心工具：

- 抬高地形。
- 降低地形。
- 平整地形。
- 平滑地形。
- 修改地表贴图。
- 添加/移除浅水。
- 撤销/重做。

刷子参数：

- 形状：圆形、方形。
- 半径：1-16。
- 强度：1-100。
- 软边缘开关。

### 7.5 保存地图

必须保证：

- `war3map.w3e` 能正确写回。
- `war3map.wpm` 可同步更新。
- `war3map.shd` 可保持原数据或重建默认数据。
- `war3map.w3i` 基础信息不丢失。
- 保存前自动备份。
- `.w3x/.w3m` 初期优先另存为，避免破坏原图。

第一阶段验收标准：

```text
打开测试地图 → 抬高一块地形 → 保存 → 重新打开 → 修改仍存在
```

进阶验收：保存后的地图可被原版 World Editor 打开且不崩溃。

## 8. 推荐目录结构

```text
W3Editor/
  CMakeLists.txt
  vcpkg.json
  README.md
  docs/
    W3EditorNext规划.md
  apps/
    editor/
      main.cpp
      EditorMainWindow.h
      EditorMainWindow.cpp
      TerrainEditorView.h
      TerrainEditorView.cpp
      NewMapDialog.h
      NewMapDialog.cpp
  engine/
    core/
      Log.h
      Result.h
      FileSystem.h
    map/
      WarMapDocument.h
      WarMapDocument.cpp
      MapOpenOptions.h
      MapSaveOptions.h
    formats/
      archive/
        IArchive.h
        DirectoryArchive.h
        DirectoryArchive.cpp
        MpqArchive.h
        MpqArchive.cpp
      w3e/
        W3eTypes.h
        W3eReader.h
        W3eReader.cpp
        W3eWriter.h
        W3eWriter.cpp
      w3i/
      wpm/
      shd/
      mmp/
    terrain/
      TerrainModel.h
      TerrainModel.cpp
      TerrainBrush.h
      TerrainBrush.cpp
      TerrainEditCommand.h
      TerrainUndoStack.h
    render/
      RenderDevice.h
      TerrainRenderer.h
      TerrainRenderer.cpp
      WaterRenderer.h
      BrushOverlayRenderer.h
    resources/
      ResourceIndex.h
      ResourceIndex.cpp
      ResourceManager.h
      ResourceManager.cpp
      TextureResourceCache.h
      ModelResourceCatalog.h
      MissingResourcePolicy.h
    preview/
      PreviewCatalog.h
      PreviewCatalog.cpp
      PreviewCache.h
      PreviewLoadQueue.h
      ModelPreviewProvider.h
    assets/
      AssetProvider.h
      WarcraftDataSource.h
      BlpTextureLoader.h
  data/
    icons/
    shaders/
    themes/
  tests/
    format_tests/
    terrain_tests/
    resource_tests/
    preview_tests/
  界面参考/
```

## 9. 模块边界

### 9.1 `formats`

只负责格式读写，不依赖 Qt UI，不处理编辑器交互。

要求：

- Reader 只解析二进制到结构体。
- Writer 只把结构体写回二进制。
- 不在格式层做地形刷子逻辑。
- 必须支持 round-trip 测试。

### 9.2 `terrain`

负责地形数据模型和编辑逻辑。

要求：

- 不直接依赖 Qt 控件。
- 通过明确接口暴露高度、贴图、水面、路径等数据。
- 所有编辑行为通过 Command 记录，便于撤销/重做。

### 9.3 `render`

负责把 `TerrainModel` 渲染出来。

要求：

- 不直接读写地图文件。
- 支持局部刷新。
- 地形、水面、覆盖层分开。

### 9.4 `resources`

负责统一管理地图编辑所需资源，不直接承担 UI 展示和地形编辑逻辑。

要求：

- 通过 Archive 抽象读取地图包、游戏基础资源和后续资源覆盖层。
- 管理 BLP 贴图、SLK/对象表、MDX/模型、图标、地形装饰和缺失资源回退策略。
- 提供资源索引、缓存、失效、错误日志和批量查询接口。
- 必须支持自动化测试验证资源索引、缓存命中、缺失资源回退和错误路径。

### 9.5 `preview`

负责模型批量预览和预览缓存，不直接修改地图数据。

要求：

- 支持装饰物、单位、地形装饰、贴图和水面相关资源的批量预览列表。
- 支持异步加载队列、取消、失败状态、缩略图/预览缓存和筛选。
- 预览数据来自 `resources`，不允许 UI 面板直接散落读取模型或贴图文件。
- 必须提供 CLI 或单元测试入口验证批量预览列表、缓存命中、加载取消和缺失资源占位。

### 9.6 `apps/editor`

负责窗口、菜单、工具栏、面板、用户输入。

要求：

- 不直接解析 `w3e/wpm` 二进制。
- 通过 `WarMapDocument` 和 `TerrainEditorController` 操作地图。

## 10. 数据流

### 10.1 打开地图

```text
用户选择地图目录或 .w3x/.w3m
        ↓
WarMapDocument::open()
        ↓
DirectoryArchive / MpqArchive
        ↓
读取 w3i / w3e / wpm / shd / mmp
        ↓
构建 TerrainModel
        ↓
TerrainEditorView 绑定 TerrainModel
        ↓
TerrainRenderer 渲染
```

### 10.2 编辑地形

```text
鼠标点击/拖拽
        ↓
TerrainEditorController
        ↓
TerrainBrush 计算影响区域
        ↓
TerrainEditCommand 修改 TerrainModel
        ↓
TerrainUndoStack 记录变更
        ↓
Renderer 局部刷新
```

### 10.3 保存地图

```text
WarMapDocument::save()
        ↓
TerrainModel 写回 W3E/WPM/SHD 数据
        ↓
W3eWriter / WpmWriter / ShdWriter
        ↓
DirectoryArchive / MpqArchive 写入
        ↓
保存成功，标记 clean
```

## 11. UI 规划

界面参考目录：

```text
C:\war3\mytool\W3Editor\界面参考
```

主界面建议：

```text
┌──────────────────────────────────────────────┐
│ 菜单栏：文件 编辑 查看 层面 模块 工具 高级 配置 │
├──────────────────────────────────────────────┤
│ 工具栏：新建 打开 保存 撤销 重做 选择 地形工具   │
├───────────────┬──────────────────────┬───────┤
│ 地形工具面板   │ 3D 地形视图             │ 小地图 │
│ - 抬高/降低    │                      │       │
│ - 平整/平滑    │                      ├───────┤
│ - 贴图         │                      │ 属性  │
│ - 水面         │                      │       │
├───────────────┴──────────────────────┴───────┤
│ 状态栏：坐标、高度、贴图ID、地图尺寸、保存状态     │
└──────────────────────────────────────────────┘
```

第一阶段 UI 原则：

- 默认界面语言为简体中文。
- 国际化第一阶段只支持英文资源，不扩展其他语言。
- 所有新增 UI 文案必须先提供中文主文案；英文翻译作为国际化资源维护。
- 优先深色主题。
- 地形视图占最大空间。
- 左侧为当前工具与刷子参数。
- 右侧为小地图和属性，可折叠。
- 工具图标可从旧项目 `data/icons/terrain` 迁移。
- 不做复杂皮肤系统，先保证稳定和清晰。

## 12. 开发里程碑

阶段门禁原则：每个里程碑必须通过验收后才能进入下一阶段；如果验收失败，优先回退修正当前阶段，不扩大范围。

### M0：Archive 与地形格式验证

目标：不做 UI，先证明本项目能直接打开 `.w3x/.w3m`，从地图包内读取 `war3map.w3e`，并完成 `w3e` 读写验证。

任务：

- 建立最小 CMake 工程。
- 实现 `IArchive`、`MpqArchive`、`DirectoryArchive`。
- `MpqArchive` 必须能直接列出 `.w3x/.w3m` 内部文件，并读取 `war3map.w3e`。
- 实现 `W3eReader`。
- 实现 `W3eWriter`。
- 从真实 `.w3x/.w3m` 读取包内 `war3map.w3e`，不得调用 `w3x2lni` 或外部解包命令。
- `W3EditorOld/data/test map/war3map.w3e` 只能作为解包对照样本，不作为主测试输入。
- 输出地图尺寸、地形集、高度范围、tile 数量。
- 校验 `TerrainCorner` 字段覆盖旧项目 `Corner` 语义。
- 校验最终地面高度、水面高度、层高、高度上下限的 War3 兼容规则。
- 做 round-trip 测试。

验收：

```text
读取 C:\war3\mytool\W3Editor\testmap\testdemo.w3x → Archive 提取包内 war3map.w3e 到内存 → W3eReader 解析 → W3eWriter 写出内存数据 → 再读取 → TerrainModel 一致
```

附加检查：

- 未修改字段必须保持二进制或语义等价，不得因为未知字段丢失导致原图失真。
- `TerrainModel` dump 应能定位任意 corner 的高度、层高、水面、贴图、悬崖和边界状态。
- 至少包含一个自动化或命令行验证入口，避免只靠 UI 手动观察。

### M1：编辑器空壳

目标：64 位 Qt 6 编辑器能启动。

任务：

- 主窗口。
- 菜单栏。
- 工具栏。
- 状态栏。
- 基础深色主题。
- 打开地图目录入口。

验收：

- Windows 64 位构建成功。
- 编辑器启动稳定。

### M2：打开地图并显示地形

目标：能直接打开真实 `.w3x/.w3m` 并显示地形；目录地图只作为调试路径。

任务：

- `MpqArchive`。
- `DirectoryArchive`。
- `WarMapDocument::open()`。
- `TerrainModel`。
- `TerrainRenderer`。
- 相机控制。
- 鼠标拾取。
- 地形、水面、刷子覆盖层分开渲染。
- 渲染层只消费 `TerrainModel` 快照或只读接口，不读取 `w3e` 二进制。

验收：

- 能打开真实 `.w3x/.w3m` 样本。
- 能看到地形。
- 状态栏显示坐标和高度。
- 同一地图的解包目录对照样本仅用于排查差异，不作为功能通过条件。

### M3：地形编辑

目标：能修改地形。

任务：

- 抬高/降低。
- 平整/平滑。
- 贴图刷。
- 水面开关。
- 刷子预览。
- 撤销/重做。
- 每个工具通过 `TerrainEditCommand` 记录修改前后数据。
- 每个工具返回最小脏区域，触发局部渲染刷新和后续路径数据同步。
- 贴图刷需要处理 variation；荒芜贴图与悬崖相邻限制按 War3 行为验证。

验收：

- 修改可视化实时生效。
- 撤销/重做稳定。

### M4：保存闭环

目标：完整闭环。

任务：

- 保存 `.w3x/.w3m`，默认另存为新文件。
- 可选导出目录地图用于调试。
- 自动备份。
- 重新打开验证。
- 保存必须通过 `WarMapDocument` 和 `Archive` 抽象，不允许 UI 直接写文件。
- MPQ 写入默认生成新文件，不覆盖原图。
- 保存后立即重新读取 `w3e/wpm/shd/mmp/w3i`，验证关键字段仍可解析。

验收：

```text
新建/打开 → 编辑 → 保存 → 重新打开 → 修改存在
```

## 13. 暂不纳入第一阶段

以下内容等地形闭环稳定后再做：

- 触发器编辑器。
- 物体编辑器。
- 单位面板完整功能。
- 装饰物面板完整功能。
- 导入管理器完整功能。
- JASS/Lua 编译链。
- YDWE 插件系统。
- w3x2lni 兼容。


YDWE 兼容扩展顺序：

1. 先做 MPQ 打开/另存。
2. 再做 BLP/SLK/对象表资源读取。
3. 再做 LNI/w3x2lni 项目识别和转换辅助。
4. 最后再考虑 YDTrigger、JASSHelper、Lua 插件、运行时测试等 YDWE 生态能力。
5. 不以 hook 原版 WorldEdit 作为新项目主路线。

## 14. 编码原则

- 优先自动测试；能用 CTest、CLI、round-trip、JSON dump、资源索引校验或预览缓存校验覆盖的问题，不应只依赖人工 UI 验证。
- 优先小步提交、小模块验证。
- 格式读写必须先于 UI。
- 所有二进制格式必须有 round-trip 测试。
- 保存地图必须先备份。
- 默认不覆盖用户原图，优先另存为。
- 不做与当前里程碑无关的大重构。
- 避免把 UI、格式解析、渲染、编辑逻辑写在同一个类里。
- 避免单文件过重；当单个源文件持续膨胀到难以审查、测试或维护时，必须按职责拆分到 Reader/Writer、Model、Controller、Service、Cache、ViewModel、Widget 等独立文件。
- 同一问题经过多次自动测试和修复仍无法稳定解决时，必须停止盲目试错，回读 `W3EditorOld`、`YDWE-master` 或相关参考项目，重新确认格式语义、资源链路和架构边界后再调整方案。
- 重规划必须记录触发原因、已失败的验证方式、参考项目证据、调整后的模块边界和新的验收方式。
- 反复失败的问题应优先收敛成最小可复现 CLI/单元测试，再进入 UI 联调。

## 15. 自动化 CLI 要求

项目必须提供足够的命令行入口，用于无人值守构建、测试、格式验证和日志采集。CLI 是第一阶段验收的一部分，不是附属工具。

最低命令集：

```text
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug
w3editor_cli --open "C:\war3\mytool\W3Editor\testmap\testdemo.w3x"
w3editor_cli --open "C:\war3\mytool\W3Editor\testmap\testdemo.w3x" --file war3map.w3e --out build/artifacts
w3editor_cli --open "C:\war3\mytool\W3Editor\testmap\testdemo.w3x" --file war3map.w3e --dump build/artifacts/dump
w3editor_cli --open "C:\war3\mytool\W3Editor\testmap\testdemo.w3x" --roundtrip build/artifacts/roundtrip.w3x
w3editor_cli --open "C:\war3\mytool\W3Editor\testmap\testdemo.w3x" --check --log build/logs --debug
```

CLI 约束：

- CLI 主入口必须使用简单、直观、短参数：`--open`、`--close`、`--file`、`--log`/`-l`、`--debug`/`-d`、`--out`/`-o`、`--dump`、`--tmp`；项目初期不保留旧命令兼容。
- Windows CLI 启动时必须主动设置 UTF-8 控制台输入/输出，避免中文路径、中文文件名和中文错误信息乱码。
- 所有命令必须有稳定退出码：`0` 成功，非 `0` 失败。
- 所有失败必须输出明确错误信息和日志位置。
- 所有格式测试必须支持从 `.w3x/.w3m` 直接读取包内文件。
- 不允许 CLI 在测试中调用 `w3x2lni`、YDWE 旧 exe 或其他外部解包器。
- 测试产物统一写入 `build/artifacts/`，日志统一写入 `build/logs/`。
- JSON dump 用于自动比对，文本输出只用于人工阅读。

## 16. 运行状态与日志观测

第一阶段需要内置可观测性，方便自动化工具、MCP 或其他外部控制器读取运行状态。

最低要求：

- 日志采用结构化 JSON Lines，默认写入 `build/logs/w3editor.jsonl` 或用户指定路径。
- 关键事件必须记录：启动、打开地图、Archive 文件列表、读取关键文件、解析失败、保存开始、备份路径、保存完成、重新打开验证结果。
- 编辑器和 CLI 都应支持 `--log`/`-l`、`--debug`/`-d`、`--out`/`-o`、`--dump`、`--tmp` 参数；日志、调试输出、通用输出、转储和临时文件都按目录语义处理。
- CLI 必须提供 `status` 或 `diagnostics` 子命令，输出最近一次运行的状态摘要。
- GUI 调试构建应提供本地诊断端点或状态快照文件，例如 `diagnostics/state.json`，内容包括当前地图路径、dirty 状态、当前工具、选中坐标、最近错误。
- MCP 工具可以读取这些日志、状态快照和 CLI 输出；项目本身不应依赖某个特定 MCP 才能测试。
- 更优先的方案是“稳定 CLI + 结构化日志 + 状态快照文件”，MCP 只作为外部自动化读取层。

## 17. 当前实现状态

### 17.1 已完成

- 已创建 C++20 + CMake 工程骨架，当前默认预设为 `Visual Studio 18 2026`、`x64`、`Debug`。
- 已建立 `w3editor_engine` 静态库、`w3editor_cli` 命令行程序和 `w3editor_archive_tests` 自动测试目标。
- 已实现最小 Archive 抽象：`IArchive`、`DirectoryArchive`、`MpqArchive` 占位实现。
- `DirectoryArchive` 已支持目录文件列表和文件读取，可用于调试目录地图和测试夹具。
- `MpqArchive` 已作为 `.w3x/.w3m` 主路径占位接入，当前会明确返回“MPQ reading is not implemented yet”，下一步接入 StormLib。
- 已建立最小自动测试，覆盖目录 Archive 列表、读取和缺失文件错误路径。

### 17.2 已验证

```powershell
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug
.\build\windows-x64-debug\Debug\w3editor_cli.exe --open .\testmap
```

验证结果：

- CMake 配置通过。
- Debug 构建通过。
- `archive_tests`、`cli_close` 通过，`2/2` tests passed。
- CLI 可通过 `--open .\testmap` 列出 `testmap` 目录中的 `.w3x` 样本文件，并能正常输出中文文件名 `九种兵器3.w3x`。


### 17.3 当前限制

- 尚未接入 StormLib，不能直接列出 `.w3x/.w3m` 包内文件。
- 尚未实现 `war3map.w3e` 读取、dump 和 round-trip。
- CLI 还未实现结构化日志写入、JSON dump 和目录参数对应的完整产物生成；当前已保留 `--log`/`-l`、`--debug`/`-d`、`--out`/`-o`、`--dump`、`--tmp` 入口。

## 18. 下一步建议

第一步创建最小 Archive + W3E 验证工程：

```text
W3Editor/tools/w3editor_cli
W3Editor/engine/formats/archive
W3Editor/engine/formats/w3e
```

先完成：

```text
读取 C:\war3\mytool\W3Editor\testmap\testdemo.w3x
列出 Archive 内部文件
从包内读取 war3map.w3e 到内存
打印地图基础信息
写出 round-trip 测试产物
验证再读一致
输出 JSON dump 和结构化日志
```

该步骤完成后，再进入 Qt 编辑器 UI。