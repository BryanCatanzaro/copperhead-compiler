/*
 *   Copyright 2012      NVIDIA Corporation
 * 
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 * 
 *       http://www.apache.org/licenses/LICENSE-2.0
 * 
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 * 
 */

#pragma once

#include <vector>
#include <map>
#include <memory>
#include <utility>
#include <prelude/runtime/chunk.hpp>
#include "type.hpp"
#include "ctype.hpp"

namespace copperhead {

typedef std::map<detail::fake_system_tag,
                 std::pair<std::vector<std::shared_ptr<chunk> >,
                           bool> > data_map;

struct cuarray {
    data_map m_d;
    std::vector<size_t> m_l;
    std::shared_ptr<backend::type_t> m_t;
    std::shared_ptr<backend::ctype::type_t> m_ct;
    size_t m_o;
};

}
