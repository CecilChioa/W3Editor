#pragma once

#include "formats/archive/IArchive.h"

namespace w3editor::formats::archive {

// 目录 Archive 仅作为调试、测试夹具和派生产物读取入口。
// 第一阶段主路径仍然是 .w3x/.w3m，经由 MpqArchive 读取包内 war3map.w3e。
class DirectoryArchive final : public IArchive {
public:
    explicit DirectoryArchive(std::filesystem::path rootPath);

    [[nodiscard]] core::Result<std::vector<ArchiveEntry>> listFiles() const override;
    [[nodiscard]] core::Result<std::vector<std::uint8_t>> readFile(const std::string& path) const override;

private:
    std::filesystem::path rootPath_;
};

} // namespace w3editor::formats::archive