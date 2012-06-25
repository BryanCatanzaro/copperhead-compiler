#include "cpp_printer.hpp"

using std::endl;
using std::string;


namespace backend
{

cpp_printer::cpp_printer(
    const copperhead::system_variant& t,
    const string &entry_point,
    const registry& globals,
    std::ostream &os)
    : py_printer(os),
      m_t(t),
      entry(entry_point),
      tp(t, os),
      m_in_rhs(false),
      m_in_struct(false) {
    const std::map<ident, fn_info>& fns = globals.fns();
    for(auto i = fns.cbegin();
        i != fns.cend();
        i++) {
        declared.insert(std::get<0>(i->first));
    }
    declared.insert("_" + entry_point);
        
}
    

void cpp_printer::operator()(const backend::name &n) {
    if ((!declared.exists(n.id())) && !m_in_rhs) {
        boost::apply_visitor(tp, n.ctype());
        m_os << " ";
        declared.insert(n.id());
    }
    m_os << n.id();
}

void cpp_printer::operator()(const templated_name &n) {
    m_os << n.id();
    m_os << "<";
    detail::list(tp, n.template_types());
    m_os << " >";
}
    
void cpp_printer::operator()(const literal &n) {
    //If we've calculated a type for this literal, print it.
    if ((detail::isinstance<ctype::monotype_t>(n.ctype())) &&
        (detail::up_get<monotype_t>(n.ctype()).name() != "void")) {
        m_os << "(";
        boost::apply_visitor(tp, n.ctype());
        m_os << ")";
    }
    m_os << n.id();
}

void cpp_printer::operator()(const tuple &n) {
    open();
    detail::list(*this, n);
    close();
}

void cpp_printer::operator()(const apply &n) {
    boost::apply_visitor(*this, n.fn());
    (*this)(n.args());
}
void cpp_printer::operator()(const closure &n) {
    m_os << "closure<";

    //If we're printing before functorization, the body of the closure
    //is a name, otherwise it's a closure.  If it's none of the above,
    //it's an error
    if (detail::isinstance<name>(n.body())) {
        boost::apply_visitor(*this, n.body());
    } else if (detail::isinstance<apply>(n.body())) {
        boost::apply_visitor(*this,
                             boost::get<const apply&>(n.body()).fn());
    } else {
        //Invalid AST - body of closure must be a name or an apply
        assert(false);
    }
    m_os << ", thrust::tuple<";
    for(auto i = n.args().begin();
        i != n.args().end();
        i++) {
        boost::apply_visitor(tp, i->ctype());
        if (std::next(i) != n.args().end()) {
            m_os << ", ";
        }
    }
    
    m_os << "> >(";
    boost::apply_visitor(*this, n.body());
    m_os << ", thrust::make_tuple(";
    for(auto i = n.args().begin();
        i != n.args().end();
        i++) {
        boost::apply_visitor(*this, *i);
        if (std::next(i) != n.args().end()) {
            m_os << ", ";
        }
    }
    m_os << "))";
}
    
void cpp_printer::operator()(const conditional &n) {
    m_os << "if (";
    boost::apply_visitor(*this, n.cond());
    m_os << ") {" << endl;
    indent();
    declared.begin_scope();
    boost::apply_visitor(*this, n.then());
    dedent();
    declared.end_scope();
    indentation();
    m_os << "} else {" << endl;
    indent();
    declared.begin_scope();
    boost::apply_visitor(*this, n.orelse());
    dedent();
    declared.end_scope();
    indentation();
    m_os << "}" << endl;
}
void cpp_printer::operator()(const ret &n) {
    m_os << "return ";
    boost::apply_visitor(*this, n.val());
    m_os << ";";
}
void cpp_printer::operator()(const bind &n) {
    m_in_rhs = false;
    boost::apply_visitor(*this, n.lhs());
    m_os << " = ";
    m_in_rhs = true;
    boost::apply_visitor(*this, n.rhs());
    m_os << ";";
    m_in_rhs = false;
}
void cpp_printer::operator()(const call &n) {
    const literal& fn = n.sub().fn();
    this->cpp_printer::operator()(fn);
    boost::apply_visitor(*this, n.sub().args());
    m_os << ";";
}
void cpp_printer::print_proc_return(const ctype::monotype_t& mt,
                                     const procedure& n) {
    //If this procedure has a type, print the return type
    //Procedures with no types are only generated by the compiler
    //For things like the BOOST_PYTHON_MODULE declaration
    if (string("void") != mt.name()) {
        if (n.place().size() > 0) {
            m_os << n.place() << " ";
        }
        assert(detail::isinstance<ctype::fn_t>(mt));
        const ctype::fn_t &n_f = boost::get<const ctype::fn_t&>(mt);
        const ctype::type_t& ret_t = n_f.result();
        boost::apply_visitor(tp, ret_t);
        m_os << " ";
    }
}


void cpp_printer::operator()(const procedure &n) {
    const string& proc_id = n.id().id();
    declared.insert(proc_id);
    declared.begin_scope();
    const ctype::type_t& n_t = n.ctype();
    
    if (detail::isinstance<ctype::polytype_t>(n_t)) {
        //If this procedure is polymorphic, we may need to print a
        //template declaration.

        const ctype::polytype_t& n_pt =
            detail::up_get<ctype::polytype_t>(n_t);

        //We don't need to print it if we're inside a structure
        //In that case, the template declaration is printed outside
        //the structure.
        //This is just a convention: we've made our structs polymorphic
        //And the functions inside monomorphic.  This will need to
        //change if we want to print something like:
        //
        //template<typename A>
        //struct fn_foo {
        //  template<typename B>
        //  B operator()(A in) {
        //    return in;
        //  }
        //};
        //For now, if we're in a struct and the function is
        //polymorphic, we'll just assume the template variables were
        //declared externally.
        
        if (!m_in_struct) {
            print_template_decl(n_pt.begin(),
                                n_pt.end());
        }
        //Print the return type
        const ctype::monotype_t& n_mt =
            n_pt.monotype();
        print_proc_return(n_mt, n);
        
    } else if (detail::isinstance<ctype::monotype_t>(n_t)) {
        //We have a monomorphic procedure, print the return type
        const ctype::monotype_t& n_mt =
            detail::up_get<ctype::monotype_t>(n_t);
        print_proc_return(n_mt, n);
    } 

        
    
    (*this)(n.id());
    (*this)(n.args());
    if (n.stmts().size() > 0) {
        //This procedure has a body
        m_os << " {" << endl;
        indent();
        (*this)(n.stmts());
        dedent();
        indentation();
        m_os << "}" << endl;
    } else {
        //This procedure is just a stub
        m_os << ';' << endl;
    }
    declared.end_scope();
}
void cpp_printer::operator()(const suite &n) {
    for(auto i = n.begin();
        i != n.end();
        i++) {
        indentation();
        boost::apply_visitor(*this, *i);
        m_os << endl;
    }
}

void cpp_printer::operator()(const structure &n) {
    indentation();
    declared.insert(n.id().id());
    //Do we have a templated struct?
    //If so, print the template declaration
    if (n.begin() != n.end()) {
        print_template_decl(n.begin(), n.end());
    }
    m_os << "struct ";
    m_os << n.id().id();
    m_os << " {" << endl;
    indent();
    m_in_struct = true;
    (*this)(n.stmts());
    m_in_struct = false;
    dedent();
    m_os << "};" << endl;
}
    
void cpp_printer::operator()(const include &n) {
    m_os << "#include " << n.open();
    boost::apply_visitor(*this, n.id());
    m_os << n.close() << endl;
}

void cpp_printer::operator()(const typedefn &n) {
    m_os << "typedef ";
    boost::apply_visitor(tp, n.origin());
    m_os << " ";
    boost::apply_visitor(tp, n.rename());
    m_os << ";";
}

void cpp_printer::operator()(const namespace_block& n) {
    m_os << "namespace ";
    m_os << n.name();
    m_os << " {" << endl;
    boost::apply_visitor(*this, n.stmts());
    m_os << "}" << endl;
}
    
void cpp_printer::operator()(const string &s) {
    m_os << s;
}


}
