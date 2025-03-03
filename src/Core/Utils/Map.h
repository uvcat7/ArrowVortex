#pragma once

#include <Precomp.h>

namespace AV {
namespace Map {

template <typename T, typename K>
T* find(std::map<K,T>& map, const K& key)
{
	auto it = map.find(key);
	return (it == map.end()) ? nullptr : &it->second;
}

template <typename Map, typename T>
void eraseVals(Map& map, const T& val)
{
	for (auto it = map.begin(); it != map.end();)
	{
		if (it->second == val)
		{
			it = map.erase(it);
		}
		else ++it;
	}
}

} // namespace Map
} // namespace AV