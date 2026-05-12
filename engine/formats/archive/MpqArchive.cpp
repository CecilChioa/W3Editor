#include "formats/archive/MpqArchive.h"

namespace w3editor::formats::archive {

MpqArchive::MpqArchive(std::filesystem::path mapPath)
    : mapPath_(std::move(mapPath)) {}

// 这里暂时只做输入校验和明确失败，确保 .w3x/.w3m 已经走到 MPQ 主路径。
// 下一步接入 StormLib 时，只替换内部实现，不改变 IArchive 对上层暴露的行为。
core::Result<std::vector<ArchiveEntry>> MpqArchive::listFiles() const {
    if (!std::filesystem::exists(mapPath_)) {
        return core::Result<std::vector<ArchiveEntry>>::failure("mpq-list", "map file does not exist: " + mapPath_.string());
    }
    if (!hasWarcraftMapExtension(mapPath_)) {
        return core::Result<std::vector<ArchiveEntry>>::failure("mpq-list", "expected .w3x/.w3m map file: " + mapPath_.string());
    }

    return core::Result<std::vector<ArchiveEntry>>::failure(
        "mpq-list",
        "MPQ reading is not implemented yet; next step is wiring StormLib behind IArchive");
}

// readFile 与 listFiles 保持同样的占位策略：先锁定错误边界，避免调用方误以为已完成解包。
core::Result<std::vector<std::uint8_t>> MpqArchive::readFile(const std::string& path) const {
    if (!std::filesystem::exists(mapPath_)) {
        return core::Result<std::vector<std::uint8_t>>::failure("mpq-read", "map file does not exist: " + mapPath_.string());
    }
    if (!hasWarcraftMapExtension(mapPath_)) {
        return core::Result<std::vector<std::uint8_t>>::failure("mpq-read", "expected .w3x/.w3m map file: " + mapPath_.string());
    }

    return core::Result<std::vector<std::uint8_t>>::failure(
        "mpq-read",
        "MPQ file extraction is not implemented yet for: " + path);
}

} // namespace w3editor::formats::archive