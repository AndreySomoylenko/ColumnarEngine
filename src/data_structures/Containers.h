#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>

template <typename Key, typename Value, typename Compare = std::less<Key>>
using FlatMap = std::map<Key, Value, Compare>;

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using HashFlatMap = std::unordered_map<Key, Value, Hash, KeyEqual>;

template <typename Key, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using HashFlatSet = std::unordered_set<Key, Hash, KeyEqual>;

template <typename Key, typename Compare = std::less<Key>>
using FlatSet = std::set<Key, Compare>;

template <typename Key, typename Compare = std::less<Key>>
using FlatMultiSet = std::multiset<Key, Compare>;

using EnabledRaws = std::optional<HashFlatSet<size_t>>;
