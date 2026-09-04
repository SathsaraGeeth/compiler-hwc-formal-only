#pragma once

#include <optional>
#include <string>
#include <utility>

namespace vir::runtime {

enum class State { completed, retry, failed };

template<class T>
class Result {
public:
    static Result completed(T value) {
        return Result(State::completed, std::move(value), {});
    }
    static Result retry() { return Result(State::retry, std::nullopt, {}); }
    static Result failed(std::string message) {
        return Result(State::failed, std::nullopt, std::move(message));
    }

    State state() const noexcept { return state_; }
    bool ready() const noexcept { return state_ == State::completed; }
    const T& value() const { return value_.value(); }
    T& value() { return value_.value(); }
    const std::string& error() const noexcept { return error_; }

private:
    Result(State state, std::optional<T> value, std::string error)
        : state_(state), value_(std::move(value)), error_(std::move(error)) {}
    State state_;
    std::optional<T> value_;
    std::string error_;
};

template<>
class Result<void> {
public:
    static Result completed() { return Result(State::completed, {}); }
    static Result retry() { return Result(State::retry, {}); }
    static Result failed(std::string message) {
        return Result(State::failed, std::move(message));
    }
    State state() const noexcept { return state_; }
    bool ready() const noexcept { return state_ == State::completed; }
    const std::string& error() const noexcept { return error_; }
private:
    Result(State state, std::string error)
        : state_(state), error_(std::move(error)) {}
    State state_;
    std::string error_;
};

} // namespace vir::runtime
