#include "formats/archive/DirectoryArchive.h"
#include "formats/archive/IArchive.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

int expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return 1;
    }
    return 0;
}

} // namespace

int main() {
    // 测试只覆盖目录 Archive 的确定行为；真实 .w3x/.w3m 读取必须等 StormLib 接入后再补独立用例。
    const auto root = std::filesystem::temp_directory_path() / "w3editor_archive_tests";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "nested");

    {
        std::ofstream file(root / "war3map.w3e", std::ios::binary);
        file << "W3E!";
    }
    {
        std::ofstream file(root / "nested" / "data.bin", std::ios::binary);
        file << "data";
    }

    w3editor::formats::archive::DirectoryArchive archive(root);
    const auto entries = archive.listFiles();
    if (const auto failed = expect(entries.hasValue(), "DirectoryArchive should list temp files")) {
        return failed;
    }
    if (const auto failed = expect(entries.value().size() == 2, "DirectoryArchive should list two files")) {
        return failed;
    }

    const auto w3e = archive.readFile("war3map.w3e");
    if (const auto failed = expect(w3e.hasValue(), "DirectoryArchive should read war3map.w3e")) {
        return failed;
    }
    if (const auto failed = expect(w3e.value().size() == 4, "war3map.w3e fixture size should be 4")) {
        return failed;
    }

    const auto missing = archive.readFile("missing.txt");
    if (const auto failed = expect(!missing.hasValue(), "missing file should return failure")) {
        return failed;
    }

    std::filesystem::remove_all(root);
    return 0;
}