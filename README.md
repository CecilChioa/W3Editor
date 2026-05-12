# W3Editor

64 位现代化 Warcraft III / YDWE 编辑器。当前实现从 M0 开始推进：Archive 抽象、CLI、自动测试骨架。

## 构建

```powershell
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug
```

## CLI 示例

```powershell
.\build\windows-x64-debug\Debug\w3editor_cli.exe --open .\testmap
.\build\windows-x64-debug\Debug\w3editor_cli.exe --open .\testmap --file testdemo.w3x --out build/artifacts --log build/logs --debug
```

常用参数：

- `--log` / `-l`：日志目录
- `--debug` / `-d`：调试输出
- `--out` / `-o`：输出目录
- `--dump`：转储目录
- `--tmp`：临时文件目录
```