/*
 * Copyright (c) 2020-2021 The reone project contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#include <boost/noncopyable.hpp>

namespace reone {

/**
 * Utility class for caching objects. Requires a function which computes an object by key.
 */
template <class K, class V>
class MemoryCache : boost::noncopyable {
public:
    MemoryCache(std::function<std::shared_ptr<V>(K)> compute) : _compute(compute) {
        if (!compute) {
            throw std::invalid_argument("compute must not be empty");
        }
    }

    void invalidate() {
        _objects.clear();
    }

    std::shared_ptr<V> get(K key) {
        auto maybeObject = _objects.find(key);
        if (maybeObject != _objects.end()) return maybeObject->second;

        auto object = _compute(key);

        return _objects.insert(make_pair(key, move(object))).first->second;
    }

private:
    std::function<std::shared_ptr<V>(K)> _compute;

    std::unordered_map<K, std::shared_ptr<V>> _objects;
};

} // namespace reone
