#pragma once

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace expanded {

// Expanded state that belongs to one save: a JSON object in expanded.json, beside the save's save.dat.
inline constexpr const char *kSaveFile = "expanded.json";

// Forgets the state, for a new game.
void SaveClear();

// Whether the state holds nothing.
bool SaveEmpty();

// Writes the state to the save folder's expanded.json, or removes the file when the state is empty.
bool SaveWrite(const std::filesystem::path &folder);

// Reads the save folder's expanded.json. A save without one has an empty state; one that is not a JSON
// object, or is too large, gives an empty state and false.
bool SaveRead(const std::filesystem::path &folder);

// Removes the save folder's expanded.json; true when it is gone.
bool SaveRemove(const std::filesystem::path &folder);

// One value of the state, under its key: `const SaveValue<int> kStage{"quest.stage", 0};`.
// T is bool, int, float, std::string or std::vector<int>.
template <class T>
class SaveValue {
public:
    SaveValue(const char *key, T fallback = T{}) : key_(key), fallback_(std::move(fallback)) {}

    // The save's value, or the fallback where it has none or one that is not a T.
    T Get() const;

    void Set(const T &value) const;

    // Removes the value, so Get gives the fallback.
    void Reset() const;

    bool Has() const;

private:
    const char *key_;
    T           fallback_;
};

extern template class SaveValue<bool>;
extern template class SaveValue<int>;
extern template class SaveValue<float>;
extern template class SaveValue<std::string>;
extern template class SaveValue<std::vector<int>>;

} // namespace expanded
