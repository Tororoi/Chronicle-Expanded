#include "expanded/core/save.hpp"

#include <nlohmann/json.hpp>

#include <climits>
#include <cstdint>
#include <cstdio>
#include <string>
#include <system_error>
#include <type_traits>

#include "platform/files.hpp"
#include "platform/paths.hpp"

namespace expanded {

namespace {

using Json = nlohmann::ordered_json;

// Far more than the state should ever hold.
constexpr std::size_t kMaxSize = 0x100000;

Json g_state = Json::object();

// Whether a JSON value reads as T without changing it: no fraction into an int, no number into a bool.
template <class T>
bool Holds(const Json &value) {
    if constexpr (std::is_same_v<T, bool>) {
        return value.is_boolean();
    } else if constexpr (std::is_same_v<T, int>) {
        return value.is_number_integer() && value.get<std::int64_t>() >= INT32_MIN && value.get<std::int64_t>() <= INT32_MAX;
    } else if constexpr (std::is_same_v<T, float>) {
        return value.is_number();
    } else if constexpr (std::is_same_v<T, std::string>) {
        return value.is_string();
    } else {
        if (!value.is_array()) {
            return false;
        }
        for (const Json &item : value) {
            if (!Holds<int>(item)) {
                return false;
            }
        }
        return true;
    }
}

} // namespace

void SaveClear() {
    g_state = Json::object();
}

bool SaveEmpty() {
    return g_state.empty();
}

bool SaveWrite(const std::filesystem::path &folder) {
    if (g_state.empty()) {
        return SaveRemove(folder);
    }
    std::string text = g_state.dump(4) + "\n";
    return FilesWrite(folder / kSaveFile, text.data(), text.size(), true) == FilesResult::kWritten;
}

bool SaveRead(const std::filesystem::path &folder) {
    g_state = Json::object();
    std::filesystem::path path = folder / kSaveFile;
    std::error_code       error;
    if (!std::filesystem::exists(path, error)) {
        return true;
    }
    std::string text(kMaxSize + 1, '\0');
    text.resize(FilesRead(path, text.data(), text.size()));
    Json json = text.size() > kMaxSize ? Json() : Json::parse(text, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        std::fprintf(stderr, "expanded: ignoring %s, which is not a JSON object\n", PathsDisplay(path).c_str());
        return false;
    }
    g_state = std::move(json);
    return true;
}

bool SaveRemove(const std::filesystem::path &folder) {
    std::error_code error;
    std::filesystem::remove(folder / kSaveFile, error);
    return !error;
}

template <class T>
T SaveValue<T>::Get() const {
    auto value = g_state.find(key_);
    return value != g_state.end() && Holds<T>(*value) ? value->template get<T>() : fallback_;
}

template <class T>
void SaveValue<T>::Set(const T &value) const {
    g_state[key_] = value;
}

template <class T>
void SaveValue<T>::Reset() const {
    g_state.erase(key_);
}

template <class T>
bool SaveValue<T>::Has() const {
    return g_state.contains(key_);
}

template class SaveValue<bool>;
template class SaveValue<int>;
template class SaveValue<float>;
template class SaveValue<std::string>;
template class SaveValue<std::vector<int>>;

} // namespace expanded
