#pragma once

#include <Core/Core.h>

#include <map>

namespace Vortex {
namespace Map {

template <typename T, typename K>
static T* findVal(std::map<K, T>& map, const K& key) {
    auto it = map.find(key);
    return (it == map.end()) ? nullptr : &it->second;
}

template <typename T, typename K>
static const K* findKey(std::map<K, T>& map, const T& val) {
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it->second == val) return &it->first;
    }
    return nullptr;
}

template <typename Map, typename T>
static typename Map::iterator findIt(Map& map, const T& val) {
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it->second == val) return it;
    }
    return map.end();
}

template <typename Map, typename T>
static void eraseVals(Map& map, const T& val) {
    for (auto it = map.begin(); it != map.end();) {
        if (it->second == val) {
            it = map.erase(it);
        } else
            ++it;
    }
}

};  // namespace Map
};  // namespace Vortex
