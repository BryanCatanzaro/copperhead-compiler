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

#include <prelude/runtime/tags.h>
#include <prelude/runtime/tag_malloc_and_free.h>

namespace copperhead {

namespace detail {

struct apply_malloc
    : public boost::static_visitor<void*> {
    const size_t m_ctr;
    apply_malloc(const size_t& ctr) : m_ctr(ctr) {}

    template<typename Tag>
    void* operator()(const Tag& t) const {
        return thrust::detail::tag_malloc(Tag(), m_ctr);
    }
};

struct apply_free
    : public boost::static_visitor<> {
    void* m_p;
    apply_free(void* p) : m_p(p) {}

    template<typename Tag>
    void operator()(const Tag& t) const {
        thrust::detail::tag_free(Tag(), m_p);
    }
};

}

class chunk {
  private:
    system_variant m_s;
    void* m_d;
    size_t m_r;
  public:
    chunk(const system_variant &s,
          size_t r) : m_s(s), m_d(NULL), m_r(r) {
    }
    ~chunk() {
        if (m_d != NULL) {
            boost::apply_visitor(
                detail::apply_free(m_d),
                m_s);
        }
    }
private:
    //Not copyable
    chunk(const chunk&);
    chunk& operator=(const chunk&);
public:
    void* ptr() {
        if (m_d == NULL) {
            //Lazy allocation - only allocate when pointer is requested
            m_d = boost::apply_visitor(
                detail::apply_malloc(m_r),
                m_s);
        } 
        return m_d;
    }
    size_t size() const {
        return m_r;
    }

};

}
