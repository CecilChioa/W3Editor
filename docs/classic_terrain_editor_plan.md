# W3Editor 经典版地形编辑实现规划（参考 YDWE / HiveWE）

## 1. 目标与范围

### 1.1 总目标
- 在 `W3Editor` 中实现“**经典版 Warcraft III 地形编辑**”可用闭环：
  1. 打开 `.w3x/.w3m`
  2. 完整读取并构建地形数据模型
  3. 3D 视图正确渲染（地表/悬崖/水/路径/阴影）
  4. 提供核心地形编辑工具（抬升、降低、平滑、纹理刷、悬崖刷、水）
  5. 保存回地图（优先保存关键地形文件）

### 1.2 版本策略
- **优先：经典版数据流程（MPQ + SLK/TXT/INI）**
- **次级：重制版兼容（CASC）**
- 运行时策略：
  1. 先尝试经典版路径（`war3.mpq/War3x.mpq/War3Patch.mpq`）
  2. 失败后再尝试重制版存储
  3. UI/日志明确报告当前数据源

### 1.3 本阶段不做
- 触发器编辑器（只保留加载位点）
- 完整对象编辑器 UI（仅保证地形编辑依赖的数据可加载）
- 战役级多图管理

---

## 2. 参考项目职责映射

> 原则：不直接粘贴实现，按现项目架构做“等价流程 + 等价数据”。

### 2.1 参考 YDWE（经典编辑器工作流）
- 菜单/工具栏驱动的编辑入口
- 经典地图资源组织方式
- 地形工具交互范式（笔刷/强度/半径/模式）

### 2.2 参考 HiveWE（现代渲染与编辑流程）
- 打开地图后的统一加载管线
- 地形网格 + 纹理批次 + 视图叠加（路径/阴影/小地图）
- 编辑器状态机（当前工具、笔刷参数、可视化层开关）

### 2.3 W3Editor 对齐目标
- 流程顺序对齐 HiveWE
- 经典资源优先策略对齐 YDWE
- 交互形态先做到 YDWE/HiveWE 常见最小集合

---

## 3. 统一加载管线（落地到当前项目）

当前已有基础（`map_loader`, `w3e_loader`, `game_data_loader`, `terrain_model`），下一步固定为以下阶段：

1. 设置 `hierarchy.map_directory`（解包目录）
2. 加载经典游戏数据（SLK/TXT/INI，重制版作为回退）
3. 预留 trigger strings / triggers
4. 加载 `war3mapMisc.txt`（gameplay constants）
5. 加载 `war3map.w3i`
6. 加载 `war3map.w3e`（地形核心）
7. 加载 `war3map.wpm`（路径）
8. 加载对象修改文件（`.w3u/.w3t/.w3d...`）
9. 加载 `war3map.doo`、`war3mapUnits.doo`
10. 加载 `war3map.w3r/.w3c/.w3s`
11. 加载导入表、小地图图标、阴影
12. 构建内存态 `TerrainModel + EditorMapContext`，`loaded = true`

---

## 4. 模块拆分规划（解决“代码集中在 w3editor.cpp”问题）

## 4.1 UI 层
- `src/main_window/w3editor_mainwindow.*`
  - 主窗口生命周期、Dock 管理、主菜单入口
- `src/main_window/w3editor_actions.*`
  - Action 创建、启用状态、快捷键绑定
- `src/main_window/w3editor_toolbar.*`
  - 顶部工具栏、地形工具参数条
- `src/main_window/w3editor_status.*`
  - 状态栏、FPS/选择/刷子参数显示

## 4.2 编辑器状态层
- `src/editor/editor_context.*`
  - 当前地图、当前工具、选择状态、撤销栈入口
- `src/editor/terrain_tool_state.*`
  - 笔刷半径/强度/形状/纹理层/悬崖层

## 4.3 地形数据与算法层
- `src/terrain/terrain_edit_ops.*`
  - 抬升/降低/平滑/设高/平刷
- `src/terrain/texture_paint_ops.*`
  - 纹理权重或 tile index 写入
- `src/terrain/cliff_ops.*`
  - 悬崖级别、悬崖类型处理
- `src/terrain/water_ops.*`
  - 水高度与水域标记编辑

## 4.4 渲染层
- `src/render/terrain_renderer.*`
  - 地形网格渲染、材质绑定
- `src/render/overlay_renderer.*`
  - 路径网格/阴影/边界/选区刷圈
- `src/render/minimap_renderer.*`
  - 小地图纹理更新与显示

## 4.5 IO 层扩展
- `src/map_io/map_save_pipeline.*`
  - 写回 `w3e/wpm`（先做）、后续扩展 `doo/units`
- `src/map_io/classic_data_source.*`
  - 经典 MPQ 数据源统一封装
- `src/map_io/reforged_data_source.*`
  - CASC 回退数据源封装

---

## 5. 分阶段实施（建议顺序）

## Phase A（架构清理）
1. 拆分 `w3editor.cpp` 到 UI 子模块（不改行为）
2. 清理重复菜单/重复工具栏，保留单一入口
3. 建立 `EditorContext`，把地图状态从窗口类剥离

**验收：**
- 编译通过
- 现有“打开地图 + 渲染 + 日志”行为不回退

## Phase B（地形编辑 MVP）
1. 增加地形编辑工具：Raise/Lower/Smooth/Flatten
2. 鼠标命中地形坐标，笔刷作用到角点高度
3. 实时重建局部网格并刷新渲染
4. 接入基础撤销/重做（高度修改）

**验收：**
- 可在 3D 视图拖拽改地形
- 撤销/重做可用

## Phase C（纹理/悬崖/水）
1. 纹理刷（按 tileset 限制）
2. 悬崖工具（层级与类型）
3. 水工具（水面高度/可视标记）

**验收：**
- 地形三大核心工具可用，视觉反馈正确

## Phase D（保存回写）
1. 写回 `war3map.w3e`
2. 写回 `war3map.wpm`（若路径联动编辑已启用）
3. 保存 `.w3x`（重新打包）流程打通

**验收：**
- 保存后可被 HiveWE / YDWE / 游戏重新打开且数据一致

## Phase E（对齐与性能）
1. 与 HiveWE/YDWE 对照测试样例地图
2. 大图（256+）下局部更新优化
3. 编辑器可视层（路径/阴影/网格）开关与性能调优

---

## 6. 数据正确性与兼容性清单

每次里程碑要跑以下用例：

1. 小图（32x32）/中图（128x128）/大图（320x320）
2. 经典版地图（主要）
3. 含自定义 tileset 地图
4. 含大量 doodad/阴影/pathing 地图
5. 编辑后保存并回读比对：
   - 尺寸、offset、corner 数
   - 高度范围、水高度范围
   - tileset 与 tile id 表
   - pathing 统计

---

## 7. 国际化（中文优先，英文可切换）

1. 统一使用 Qt 翻译机制（`tr()` + `.ts/.qm`）
2. 默认语言：中文
3. 菜单 `语言` 下支持即时切换 `中文/English`
4. 日志输出保留双语模板能力（调试可切英文）

---

## 8. 近期两周执行建议（可直接开工）

### Week 1
- 完成 Phase A（拆分 `w3editor.cpp` + 单菜单/单工具栏）
- 建立 `EditorContext`
- 接入基础地形工具框架（空实现）

### Week 2
- 完成 Phase B（高度编辑 + 撤销/重做）
- 局部网格刷新打通
- 保存 `w3e` 的最小闭环（先不打包）

---

## 9. 当前仓库下一步（立即执行项）

1. 先做 **Phase A-1**：把 `w3editor.cpp` 拆成 `mainwindow/actions/toolbar/status`
2. 同步修复 UI 文本与编码问题（统一 UTF-8）
3. 添加 `EditorContext` 并迁移地图状态成员
4. 保证重新编译通过后，再进入地形工具实现

