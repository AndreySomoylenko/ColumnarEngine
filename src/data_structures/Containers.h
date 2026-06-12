#pragma once

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include <cstddef>
#include <functional>
#include <optional>

template <typename Key, typename Value, typename Compare = std::less<Key>>
using FlatMap = boost::container::flat_map<Key, Value, Compare>;

template <typename Key, typename Value, typename Hash = boost::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using HashFlatMap =
    boost::unordered_flat_map<Key, Value, Hash, KeyEqual>;

template <typename Key, typename Hash = boost::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using HashFlatSet = boost::unordered_flat_set<Key, Hash, KeyEqual>;

template <typename Key, typename Compare = std::less<Key>>
using FlatSet = boost::container::flat_set<Key, Compare>;

template <typename Key, typename Compare = std::less<Key>>
using FlatMultiSet = boost::container::flat_multiset<Key, Compare>;

using EnabledRaws = std::optional<HashFlatSet<size_t>>;
