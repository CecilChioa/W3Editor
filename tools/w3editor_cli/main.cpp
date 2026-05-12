#include "formats/archive/DirectoryArchive.h"
#include "formats/archive/MpqArchive.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

using w3editor::formats::archive::DirectoryArchive;
using w3editor::formats::archive::IArchive;
using w3editor::formats::archive::MpqArchive;
using w3editor::formats::archive::hasWarcraftMapExtension;

void configureConsoleEncoding() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

void printUsage() {
    std::cout << "w3editor_cli usage:\n"
              << "  w3editor_cli --open <map-or-directory> [--debug|-d] [--log|-l <dir>] [--tmp <dir>]\n"
              << "  w3editor_cli --open <map-or-directory> --file <archive-path> [--out|-o <dir>] [--dump <dir>]\n"
              << "  w3editor_cli --close\n";
}

// CLI 初期不保留旧子命令兼容；这里仅解析当前规则允许的直观参数和短别名。
std::string valueAfterAny(int argc, char** argv, const std::vector<std::string>& names) {
    for (int index = 1; index + 1 < argc; ++index) {
        for (const auto& name : names) {
            if (argv[index] == name) {
                return argv[index + 1];
            }
        }
    }
    return {};
}

std::string valueAfter(int argc, char** argv, const std::string& name) {
    return valueAfterAny(argc, argv, {name});
}

bool hasFlagAny(int argc, char** argv, const std::vector<std::string>& names) {
    for (int index = 1; index < argc; ++index) {
        for (const auto& name : names) {
            if (argv[index] == name) {
                return true;
            }
        }
    }
    return false;
}

bool hasFlag(int argc, char** argv, const std::string& name) {
    return hasFlagAny(argc, argv, {name});
}

std::unique_ptr<IArchive> openArchive(const std::filesystem::path& path) {
    if (std::filesystem::is_directory(path)) {
        return std::make_unique<DirectoryArchive>(path);
    }
    if (hasWarcraftMapExtension(path)) {
        return std::make_unique<MpqArchive>(path);
    }
    return nullptr;
}

// CLI 统一输出 stage/message，方便后续结构化日志直接复用同一套错误边界。
int printError(const w3editor::core::Error& error) {
    std::cerr << "error stage=" << error.stage << " message=" << error.message << '\n';
    return 1;
}

int writeBinaryFile(const std::filesystem::path& outputPath, const std::vector<std::uint8_t>& data) {
    const auto parentPath = outputPath.parent_path();
    if (!parentPath.empty()) {
        std::filesystem::create_directories(parentPath);
    }

    std::ofstream output(outputPath, std::ios::binary);
    if (!output) {
        std::cerr << "error stage=write-output message=failed to open output file\n";
        return 1;
    }

    if (!data.empty()) {
        output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }

    if (!output) {
        std::cerr << "error stage=write-output message=failed to write output file\n";
        return 1;
    }

    return 0;
}

std::filesystem::path outputFilePath(const std::filesystem::path& outputDirectory, const std::string& archivePath) {
    auto fileName = std::filesystem::path(archivePath).filename();
    if (fileName.empty()) {
        fileName = "archive-file.bin";
    }
    return outputDirectory / fileName;
}

void createDirectoryIfProvided(const std::string& path) {
    if (!path.empty()) {
        std::filesystem::create_directories(path);
    }
}

} // namespace

int main(int argc, char** argv) {
    configureConsoleEncoding();

    if (argc < 2) {
        printUsage();
        return 1;
    }

    if (hasFlag(argc, argv, "--close")) {
        std::cout << "closed\n";
        return 0;
    }

    const auto mapPath = valueAfter(argc, argv, "--open");
    if (mapPath.empty()) {
        printUsage();
        return 1;
    }

    const auto debugEnabled = hasFlagAny(argc, argv, {"--debug", "-d"});
    const auto logDirectory = valueAfterAny(argc, argv, {"--log", "-l"});
    const auto dumpDirectory = valueAfter(argc, argv, "--dump");
    const auto temporaryDirectory = valueAfter(argc, argv, "--tmp");

    // 当前阶段先把目录参数固定为“创建并保留”的行为，后续结构化日志、dump 和临时文件直接接入这些目录。
    createDirectoryIfProvided(logDirectory);
    createDirectoryIfProvided(dumpDirectory);
    createDirectoryIfProvided(temporaryDirectory);

    if (debugEnabled) {
        std::cerr << "debug map=" << mapPath;
        if (!logDirectory.empty()) {
            std::cerr << " log=" << logDirectory;
        }
        if (!dumpDirectory.empty()) {
            std::cerr << " dump=" << dumpDirectory;
        }
        if (!temporaryDirectory.empty()) {
            std::cerr << " tmp=" << temporaryDirectory;
        }
        std::cerr << '\n';
    }

    auto archive = openArchive(mapPath);
    if (!archive) {
        std::cerr << "error stage=open message=unsupported archive path: " << mapPath << '\n';
        return 1;
    }

    const auto filePath = valueAfter(argc, argv, "--file");
    if (filePath.empty()) {
        const auto entries = archive->listFiles();
        if (!entries) {
            return printError(entries.error());
        }

        for (const auto& entry : entries.value()) {
            std::cout << entry.path << '\t' << entry.size << '\n';
        }
        return 0;
    }

    const auto data = archive->readFile(filePath);
    if (!data) {
        return printError(data.error());
    }

    const auto outputDirectory = valueAfterAny(argc, argv, {"--out", "-o"});
    if (!outputDirectory.empty()) {
        return writeBinaryFile(outputFilePath(outputDirectory, filePath), data.value());
    }

    std::cout.write(reinterpret_cast<const char*>(data.value().data()), static_cast<std::streamsize>(data.value().size()));
    return 0;
}