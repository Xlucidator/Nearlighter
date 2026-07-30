# VS Code 中使用 CMake Preset

## 工作区设置

在本地 `.vscode/settings.json` 中启用项目 preset：

```json
{
    "cmake.useCMakePresets": "always",
    "cmake.configureOnOpen": false,
    "cmake.configureOnEdit": true
}
```

- `cmake.useCMakePresets`：构建目录和 configuration 由 `CMakePresets.json` 决定。
- `cmake.configureOnOpen`：关闭项目打开时的自动配置。
- `cmake.configureOnEdit`：保存 `CMakeLists.txt` 或已知 `.cmake` 文件时自动重新配置；不会编译 C++ 文件。

## 首次初始化

关闭 `configureOnOpen` 后，需要为当前工作区手动初始化一次：

1. 按 `Ctrl+Shift+P` 打开命令面板。
2. 执行 `CMake: Select Configure Preset`。
3. 选择 `debug` 或 `release`。
4. 执行一次 `CMake: Configure`。

初始化后，保存 CMake 文件会自动使用当前 preset 重新配置，并写入 `build/debug/` 或 `build/release/`，不会在顶层 `build/` 生成另一套构建文件。
