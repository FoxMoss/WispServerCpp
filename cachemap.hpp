#pragma once

#include <cstring>
#include <functional>
#include <map>
#include <optional>
template <typename KeyT, typename ValT> class CacheMap {
public:
  CacheMap(std::function<ValT(KeyT)> func, ValT nullValueC) {
    undefinedValueFunc = func;
    nullValue = nullValueC;
  }
  ~CacheMap() {}
  ValT &operator[](KeyT key) {
    if (internalMap.find(key) != internalMap.end()) {
      return internalMap[key];
    }
    internalMap[key] = undefinedValueFunc(key);
    return internalMap[key];
  }
  std::optional<KeyT> find(ValT val) {
    for (auto keyContender = internalMap.begin();
         keyContender != internalMap.end(); keyContender++) {
      if (keyContender->second == val) {
        return keyContender->first;
      }
    }
    return {};
  }
  void openKey(KeyT key) { internalMap[key] = undefinedValueFunc(key); }
  void deleteKey(KeyT key) { internalMap[key] = nullValue; }

private:
  std::map<KeyT, ValT> internalMap;
  std::function<ValT(KeyT)> undefinedValueFunc;
  ValT nullValue;
};

inline bool isValidInt(char *buf) {
  size_t buflen = strlen(buf);

  for (size_t i = 0; i < buflen; i++) {
    if (buf[i] > '9' || buf[i] < '0') {
      return false;
    }
  }
  return true;
}
