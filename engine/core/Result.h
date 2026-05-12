#pragma once

#include <string>
#include <utility>
#include <variant>

namespace w3editor::core {

// 统一错误对象：stage 用于自动化日志和 CLI 输出定位失败阶段，message 面向人工排查。
struct Error {
    std::string stage;
    std::string message;
};

// Result 用于格式解析、Archive 读写和 CLI 链路的显式错误返回。
// 核心层不依赖 UI 弹窗，也不在底层直接退出进程，调用方必须检查返回值并决定如何记录或展示错误。
template <typename T>
class Result {
public:
    static Result success(T value) {
        return Result(std::move(value));
    }

    static Result failure(std::string stage, std::string message) {
        return Result(Error{std::move(stage), std::move(message)});
    }

    [[nodiscard]] bool hasValue() const {
        return std::holds_alternative<T>(data_);
    }

    [[nodiscard]] explicit operator bool() const {
        return hasValue();
    }

    [[nodiscard]] const T& value() const {
        return std::get<T>(data_);
    }

    [[nodiscard]] T& value() {
        return std::get<T>(data_);
    }

    [[nodiscard]] const Error& error() const {
        return std::get<Error>(data_);
    }

private:
    explicit Result(T value) : data_(std::move(value)) {}
    explicit Result(Error error) : data_(std::move(error)) {}

    std::variant<T, Error> data_;
};

// 无返回值操作的 Result 变体，避免用 bool 丢失失败阶段和错误信息。
class VoidResult {
public:
    static VoidResult success() {
        return VoidResult(true, {});
    }

    static VoidResult failure(std::string stage, std::string message) {
        return VoidResult(false, Error{std::move(stage), std::move(message)});
    }

    [[nodiscard]] bool hasValue() const {
        return ok_;
    }

    [[nodiscard]] explicit operator bool() const {
        return ok_;
    }

    [[nodiscard]] const Error& error() const {
        return error_;
    }

private:
    VoidResult(bool ok, Error error) : ok_(ok), error_(std::move(error)) {}

    bool ok_ = false;
    Error error_;
};

} // namespace w3editor::core