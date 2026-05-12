#pragma once

#include "formats/archive/IArchive.h"

namespace w3editor::formats::archive {

// MPQ Archive 是 .w3x/.w3m 的主入口；当前文件先固定接口边界，后续在实现中接入 StormLib。
// 任何地图保存或读取流程都应通过该抽象进入，不能让 UI 层直接调用外部解包工具。
class MpqArchive final : public IArchive {
public:
    explicit MpqArchive(std::filesystem::path mapPath);

    [[nodiscard]] core::Result<std::vector<ArchiveEntry>> listFiles() const override;
    [[nodiscard]] core::Result<std::vector<std::uint8_t>> readFile(const std::string& path) const override;

private:
    std::filesystem::path mapPath_;
};

} // namespace w3editor::formats::archive