
### vscode 保存自动使用 Preset 配置 CMake

使用 Preset：`.vscdde/settings.json` 中添加

```json
{
    "cmake.useCMakePresets": "always",
    "cmake.configureOnOpen": false,
}
```

保存时配置：刷新操作

 1. Ctrl+Shift+P
 2. CMake: Select Configure Preset
 3. 选择 debug
 4. 执行一次 CMake: Configure

之后再保存 CMakeLists.txt