#include "ctype.hpp"
namespace backend {

namespace ctype {

namespace detail {
make_type_base_visitor::make_type_base_visitor(void *p)
    : ptr(p){}

type_base make_type_base(void *ptr, const type_base &other) {
    return boost::apply_visitor(make_type_base_visitor(ptr), other);
}

}

type_t::type_t(const type_t &other)
    : super_t(detail::make_type_base(this, other)) {}
    
monotype_t::monotype_t(const std::string &name)
        : type_t(*this),
          m_name(name) {}
    
const std::string& monotype_t::name(void) const {
    return m_name;
}

std::shared_ptr<monotype_t> int32_mt = std::make_shared<monotype_t>("int");
std::shared_ptr<monotype_t> int64_mt = std::make_shared<monotype_t>("long");
std::shared_ptr<monotype_t> uint32_mt = std::make_shared<monotype_t>("unsigned int");
std::shared_ptr<monotype_t> uint64_mt = std::make_shared<monotype_t>("unsigned long");
std::shared_ptr<monotype_t> float32_mt = std::make_shared<monotype_t>("float");
std::shared_ptr<monotype_t> float64_mt = std::make_shared<monotype_t>("double");
std::shared_ptr<monotype_t> bool_mt = std::make_shared<monotype_t>("bool");
std::shared_ptr<monotype_t> void_mt = std::make_shared<monotype_t>("void");

sequence_t::sequence_t(const std::shared_ptr<type_t> &sub)
    : monotype_t(*this, "stored_sequence"), m_sub(sub) {}

const type_t& sequence_t::sub() const {
    return *m_sub;
}

tuple_t::tuple_t(std::vector<std::shared_ptr<type_t> > && sub)
    : monotype_t(*this, "Tuple"), m_sub(std::move(sub)) {}

tuple_t::const_iterator tuple_t::begin() const {
    return boost::make_indirect_iterator(m_sub.cbegin());
}

tuple_t::const_iterator tuple_t::end() const {
    return boost::make_indirect_iterator(m_sub.cend());
}

fn_t::fn_t(const std::shared_ptr<tuple_t> args,
           const std::shared_ptr<type_t> result)
    : monotype_t(*this, "Fn"), m_args(args), m_result(result) {}
const tuple_t& fn_t::args() const {
    return *m_args;
}
const type_t& fn_t::result() const {
    return *m_result;
}

polytype_t::polytype_t() : type_t(*this) {}


cuarray_t::cuarray_t(const std::shared_ptr<type_t> sub) :
    sequence_t(*this, "sp_cuarray_var", sub) {}


templated_t::templated_t(std::shared_ptr<type_t> base, std::vector<std::shared_ptr<type_t> > && sub)
    : type_t(*this), m_base(base), m_sub(std::move(sub)) {}

const type_t& templated_t::base() const {
    return *m_base;
}
    
templated_t::const_iterator templated_t::begin() const {
    return boost::make_indirect_iterator(m_sub.cbegin());
}

templated_t::const_iterator templated_t::end() const {
    return boost::make_indirect_iterator(m_sub.cend());
}

}
}
