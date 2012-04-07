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
#include "node.hpp"
#include "type.hpp"
#include "ctype.hpp"
#include "utility/isinstance.hpp"
#include "utility/markers.hpp"
#include "utility/snippets.hpp"
#include "utility/initializers.hpp"
#include "py_printer.hpp"
#include "rewriter.hpp"
#include "prelude/runtime/tags.h"

namespace backend {

/*!
  \addtogroup rewriters
  @{
*/

//! A rewrite pass which constructs the entry point wrapper
/*! The entry point is a little special. It needs to operate on
  containers that are held by the broader context of the program,
  whereas the rest of the program operates solely on views.  This pass
  adds a wrapper which operates on containers, derives views, and then
  calls the body of the entry point.
  
*/
class wrap
    : public rewriter
{
private:
    const copperhead::system_variant& m_target;
    const std::string& m_entry_point;
    bool m_wrapping;
    std::shared_ptr<const procedure> m_wrapper;
public:
    //! Constructor
/*! 
  
  \param entry_point Name of the entry point procedure
*/
    wrap(const copperhead::system_variant&, const std::string& entry_point);
    
    using rewriter::operator();
    //! Rewrite rule for \p procedure nodes
    result_type operator()(const procedure &n);
    //! Rewrite rule for \p ret nodes
    result_type operator()(const ret& n);
    //! Rewrite rule for \p suite nodes
    result_type operator()(const suite&n);
};

/*!
  @}
*/

}
