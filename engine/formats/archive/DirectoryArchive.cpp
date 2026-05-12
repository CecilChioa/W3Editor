#include "formats/archive/DirectoryArchive.h"

#include <algorithm>
#include <fstream>

namespace w3editor::formats::archive {
namespace {

// War3 包内路径固定使用 UTF-8 风格的正斜杠相对路径；目录调试模式也要模拟这一点，
// 否则后续 w3e、资源索引和 MPQ 实现之间会出现路径语义不一致。
std::string toArchivePath(const std::filesystem::path& path) {
    const auto utf8Value = path.generic_u8string();
    auto value = std::string(utf8Value.begin(), utf8Value.end());
    if (!value.empty() && value.front() == '/') {
        value.erase(value.begin());
    }
    return value;
}

} // namespace

DirectoryArchive::DirectoryArchive(std::filesystem::path rootPath)
    : rootPath_(std::move(rootPath)) {}

core::Result<std::vector<ArchiveEntry>> DirectoryArchive::listFiles() const {
    if (!std::filesystem::exists(rootPath_)) {
        return core::Result<std::vector<ArchiveEntry>>::failure("archive-files", "archive root does not exist: " + rootPath_.string());
    }
    if (!std::filesystem::is_directory(rootPath_)) {
        return core::Result<std::vector<ArchiveEntry>>::failure("archive-files", "archive root is not a directory: " + rootPath_.string());
    }

    std::vector<ArchiveEntry> entries;
    for (const auto& item : std::filesystem::recursive_directory_iterator(rootPath_)) {
        if (!item.is_regular_file()) {
            continue;
        }

        const auto relativePath = std::filesystem::relative(item.path(), rootPath_);
        entries.push_back(ArchiveEntry{toArchivePath(relativePath), item.file_size()});
    }

    std::ranges::sort(entries, {}, &ArchiveEntry::path);
    return core::Result<std::vector<ArchiveEntry>>::success(std::move(entries));
}

core::Result<std::vector<std::uint8_t>> DirectoryArchive::readFile(const std::string& path) const {
    const auto fullPath = rootPath_ / std::filesystem::path(path);
    if (!std::filesystem::exists(fullPath) || !std::filesystem::is_regular_file(fullPath)) {
        return core::Result<std::vector<std::uint8_t>>::failure("archive-read", "file not found in directory archive: " + path);
    }

    std::ifstream stream(fullPath, std::ios::binary);
    if (!stream) {
        return core::Result<std::vector<std::uint8_t>>::failure("archive-read", "failed to open file: " + fullPath.string());
    }

    stream.seekg(0, std::ios::end);
    const auto size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    if (!data.empty()) {
        stream.read(reinterpret_cast<char*>(data.data()), size);
    }

    if (!stream && !stream.eof()) {
        return core::Result<std::vector<std::uint8_t>>::failure("archive-read", "failed to read file: " + fullPath.string());
    }

    return core::Result<std::vector<std::uint8_t>>::success(std::move(data));
}

bool hasWarcraftMapExtension(const std::filesystem::path& path) {
    const auto extension = path.extension().string();
    return extension == ".w3x" || extension == ".w3m" || extension == ".W3X" || extension == ".W3M";
}

} // namespace w3editor::formats::archive