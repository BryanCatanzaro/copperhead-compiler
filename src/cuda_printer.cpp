#include "cuda_printer.hpp"

namespace backend
{

cuda_printer::cuda_printer(const std::string &entry_point,
                           const registry& globals,
                           std::ostream &os)
    : py_printer(os), entry(entry_point), tp(os), m_in_rhs(false) {
    const std::map<ident, fn_info>& fns = globals.fns();
    for(auto i = fns.cbegin();
        i != fns.cend();
        i++) {
        declared.insert(std::get<0>(i->first));
    }
    declared.insert(detail::mark_generated_id(entry_point));
        
}
    

void cuda_printer::operator()(const backend::name &n) {
    if ((!declared.exists(n.id())) && !m_in_rhs) {
        boost::apply_visitor(tp, n.ctype());
        m_os << " ";
        declared.insert(n.id());
    }
    m_os << n.id();
}

void cuda_printer::operator()(const templated_name &n) {
    m_os << n.id();
    m_os << "<";
    detail::list(tp, n.template_types());
    m_os << ">";
}
    
void cuda_printer::operator()(const literal &n) {
    m_os << n.id();
}

void cuda_printer::operator()(const tuple &n) {
    open();
    detail::list(*this, n);
    close();
}

void cuda_printer::operator()(const apply &n) {
    boost::apply_visitor(*this, n.fn());
    (*this)(n.args());
}
void cuda_printer::operator()(const closure &n) {
    m_os << "closure";
    int arity = n.args().arity();
    assert(arity > 0);
    m_os << arity << "<";
    for(auto i = n.args().begin();
        i != n.args().end();
        i++) {
        boost::apply_visitor(tp, i->ctype());
        m_os << ", ";
    }
    //Can only deal with names in the body of a closure
    assert(detail::isinstance<name>(n.body()));
    const name& body = boost::get<const name&>(n.body());
    std::string body_fn = detail::fnize_id(body.id());
    m_os << body_fn << ">(";
    for(auto i = n.args().begin();
        i != n.args().end();
        i++) {
        boost::apply_visitor(*this, *i);
        m_os << ", ";
    }
       
    m_os << body_fn << "())";
}
void cuda_printer::operator()(const conditional &n) {
    m_os << "if (";
    boost::apply_visitor(*this, n.cond());
    m_os << ") {" << std::endl;
    indent();
    declared.begin_scope();
    boost::apply_visitor(*this, n.then());
    dedent();
    declared.end_scope();
    indentation();
    m_os << "} else {" << std::endl;
    indent();
    declared.begin_scope();
    boost::apply_visitor(*this, n.orelse());
    dedent();
    declared.end_scope();
    indentation();
    m_os << "}" << std::endl;
}
void cuda_printer::operator()(const ret &n) {
    m_os << "return ";
    boost::apply_visitor(*this, n.val());
    m_os << ";";
}
void cuda_printer::operator()(const bind &n) {
    m_in_rhs = false;
    boost::apply_visitor(*this, n.lhs());
    m_os << " = ";
    m_in_rhs = true;
    boost::apply_visitor(*this, n.rhs());
    m_os << ";";
    m_in_rhs = false;
}
void cuda_printer::operator()(const call &n) {
    const literal& fn = n.sub().fn();
    this->cuda_printer::operator()(fn);
    boost::apply_visitor(*this, n.sub().args());
    m_os << ";";
}
void cuda_printer::operator()(const procedure &n) {
    const std::string& proc_id = n.id().id();
    declared.insert(proc_id);
    declared.begin_scope();
    const ctype::type_t &n_t = n.ctype();
    //If this procedure has a type, print the return type
    //Procedures with no types are only generated by the compiler
    //For things like the BOOST_PYTHON_MODULE declaration
    if (detail::isinstance<ctype::monotype_t>(n_t)) {
        assert(detail::isinstance<ctype::monotype_t>(n_t));
        const ctype::monotype_t& n_mt =
            detail::up_get<ctype::monotype_t>(n_t);
        if (std::string("void") !=
            n_mt.name()) {
            if (n.place().size() > 0) {
                m_os << n.place() << " ";
            }
            assert(detail::isinstance<ctype::fn_t>(n_t));
            const ctype::fn_t &n_f = boost::get<const ctype::fn_t&>(n_t);
            const ctype::type_t& ret_t = n_f.result();
            boost::apply_visitor(tp, ret_t);
            m_os << " ";
        }
    }
    (*this)(n.id());
    (*this)(n.args());
    if (n.stmts().size() > 0) {
        m_os << " {" << std::endl;
        indent();
        (*this)(n.stmts());
        dedent();
        indentation();
        m_os << "}" << std::endl;
    } else {
        m_os << ';' << std::endl;
    }
    declared.end_scope();
}
void cuda_printer::operator()(const suite &n) {
    for(auto i = n.begin();
        i != n.end();
        i++) {
        indentation();
        boost::apply_visitor(*this, *i);
        m_os << std::endl;
    }
}

void cuda_printer::operator()(const structure &n) {
    indentation();
    declared.insert(n.id().id());
    m_os << "struct ";
    m_os << n.id().id();
    m_os << " {" << std::endl;
    indent();
    (*this)(n.stmts());
    dedent();
    m_os << "};" << std::endl;
}
    
void cuda_printer::operator()(const include &n) {
    m_os << "#include " << n.open();
    boost::apply_visitor(*this, n.id());
    m_os << n.close() << std::endl;
}

void cuda_printer::operator()(const typedefn &n) {
    m_os << "typedef ";
    boost::apply_visitor(tp, n.origin());
    m_os << " ";
    boost::apply_visitor(tp, n.rename());
    m_os << ";";
}
    
void cuda_printer::operator()(const std::string &s) {
    m_os << s;
}


}
