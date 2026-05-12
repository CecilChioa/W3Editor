#pragma once

#include "core/Result.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace w3editor::formats::archive {

struct ArchiveEntry {
    // Archive 内部统一使用正斜杠相对路径，避免 Windows 路径分隔符泄漏到 War3 格式层。
    std::string path;
    std::uint64_t size = 0;
};

// 地图包读取的统一抽象：上层只关心“列文件”和“读文件”，不关心来源是目录地图还是 MPQ。
// formats/map、w3e 读写和后续资源索引都应依赖该接口，避免 UI 或格式模块直接绑定 StormLib。
class IArchive {
public:
    virtual ~IArchive() = default;

    [[nodiscard]] virtual core::Result<std::vector<ArchiveEntry>> listFiles() const = 0;
    [[nodiscard]] virtual core::Result<std::vector<std::uint8_t>> readFile(const std::string& path) const = 0;
};

[[nodiscard]] bool hasWarcraftMapExtension(const std::filesystem::path& path);

} // namespace w3editor::formats::archive