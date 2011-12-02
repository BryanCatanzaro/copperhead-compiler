#include "thrust/rewrites.hpp"

using std::string;
using std::stringstream;
using std::shared_ptr;
using std::make_shared;
using std::static_pointer_cast;
using std::vector;
using std::map;
using backend::utility::make_vector;
using backend::utility::make_map;

namespace backend {

thrust_rewriter::result_type thrust_rewriter::map_rewrite(const bind& n) {
    //The rhs must be an apply
    assert(detail::isinstance<apply>(n.rhs()));
    const apply& rhs = boost::get<const apply&>(n.rhs());
    //The rhs must apply a "map"
    assert(rhs.fn().id().substr(0, 3) == string("map"));
    const tuple& ap_args = rhs.args();
    //Map must have arguments
    assert(ap_args.begin() != ap_args.end());
    auto init = ap_args.begin();
    shared_ptr<ctype::type_t> fn_t;

    if (detail::isinstance<apply>(*init)) {
        //Function instantiation
        const apply& fn_inst = boost::get<const apply&>(
            *init);
        if (detail::isinstance<templated_name>(fn_inst.fn())) {
            const templated_name& tn =
                boost::get<const templated_name&>(fn_inst.fn());
            const ctype::tuple_t& tt =
                tn.template_types();
            vector<shared_ptr<ctype::type_t> > ttc;
            for(auto i = tt.p_begin();
                i != tt.p_end();
                i++) {
                ttc.push_back(*i);
            }
            shared_ptr<ctype::monotype_t> base =
                make_shared<ctype::monotype_t>(tn.id());
            fn_t = make_shared<ctype::polytype_t>(
                std::move(ttc), base);
        } else {
            assert(detail::isinstance<name>(fn_inst.fn()));
            string fn_id = fn_inst.fn().id();
            fn_t = make_shared<ctype::monotype_t>(fn_id);
        }
    } else {
        //We must be dealing with a closure
        assert(detail::isinstance<closure>(*init));

        const closure& close = boost::get<const closure&>(
            *init);
        int arity = close.args().arity();
        //The closure must enclose something
        assert(arity > 0);
        stringstream ss;
        ss << "closure" << arity;
        string closure_t_name = ss.str();
        shared_ptr<ctype::monotype_t> closure_mt =
            make_shared<ctype::monotype_t>(closure_t_name);
        vector<shared_ptr<ctype::type_t> > cts;
        for(auto i = close.args().begin();
            i != close.args().end();
            i++) {
            cts.push_back(i->p_ctype());
        }
        //Can only deal with names in the body of a closure
        assert(detail::isinstance<name>(close.body()));

        const name& body = boost::get<const name&>(close.body());
        string body_fn = detail::fnize_id(body.id());
        cts.push_back(
            make_shared<ctype::monotype_t>(
                body_fn));
        fn_t = make_shared<ctype::polytype_t>(
            std::move(cts),
            closure_mt);
    }
    vector<shared_ptr<ctype::type_t> > arg_types;
    for(auto i = init+1; i != ap_args.end(); i++) {
        //Assert we're looking at a name
        assert(detail::isinstance<name>(*i));
        arg_types.push_back(
            make_shared<ctype::monotype_t>(
                detail::typify(boost::get<const name&>(*i).id())));
    }
    shared_ptr<ctype::polytype_t> thrust_tupled =
        make_shared<ctype::polytype_t>(
            std::move(arg_types),
            make_shared<ctype::monotype_t>("thrust::tuple"));
    shared_ptr<ctype::polytype_t> transform_t =
        make_shared<ctype::polytype_t>(
            make_vector<shared_ptr<ctype::type_t> >
            (fn_t)(thrust_tupled),
            make_shared<ctype::monotype_t>("transformed_sequence"));
    shared_ptr<apply> n_rhs =
        static_pointer_cast<apply>(get_node_ptr(n.rhs()));
    //Can only handle names on the LHS
    assert(detail::isinstance<name>(n.lhs()));
    const name& lhs = boost::get<const name&>(n.lhs());
    shared_ptr<name> n_lhs = make_shared<name>(lhs.id(),
                                                         lhs.p_type(),
                                                         transform_t);
    auto result = make_shared<bind>(n_lhs, n_rhs);
    return result;
        
}

thrust_rewriter::result_type thrust_rewriter::indices_rewrite(const bind& n) {
    //The rhs must be an apply
    assert(detail::isinstance<apply>(n.rhs()));
    const apply& rhs = boost::get<const apply&>(n.rhs());
    //The rhs must apply "indices"
    assert(rhs.fn().id() == string("indices"));
    const tuple& ap_args = rhs.args();
    //Indices must have arguments
    assert(ap_args.begin() != ap_args.end());
    const ctype::type_t& arg_t = ap_args.begin()->ctype();
    //Argument must have Seq[Int] type
    assert(detail::isinstance<ctype::sequence_t>(arg_t));
        
    shared_ptr<ctype::monotype_t> index_t =
        make_shared<ctype::monotype_t>("index_sequence");
    shared_ptr<apply> n_rhs =
        static_pointer_cast<apply>(get_node_ptr(n.rhs()));
    //Can only handle names on the LHS
    assert(detail::isinstance<name>(n.lhs()));
    const name& lhs = boost::get<const name&>(n.lhs());
    shared_ptr<name> n_lhs =
        make_shared<name>(lhs.id(),
                               lhs.p_type(),
                               index_t);
    auto result = make_shared<bind>(n_lhs, n_rhs);
    return result;
}

thrust_rewriter::thrust_rewriter() :
    m_lut(make_map<string, rewrite_fn>
          (string("map1"), &backend::thrust_rewriter::map_rewrite)
          (string("map2"), &backend::thrust_rewriter::map_rewrite)
          (string("map3"), &backend::thrust_rewriter::map_rewrite)
          (string("map4"), &backend::thrust_rewriter::map_rewrite)
          (string("map5"), &backend::thrust_rewriter::map_rewrite)
          (string("map6"), &backend::thrust_rewriter::map_rewrite)
          (string("map7"), &backend::thrust_rewriter::map_rewrite)
          (string("map8"), &backend::thrust_rewriter::map_rewrite)
          (string("map9"), &backend::thrust_rewriter::map_rewrite)
          (string("map10"), &backend::thrust_rewriter::map_rewrite)
          (string("indices"), &backend::thrust_rewriter::indices_rewrite))  {}

thrust_rewriter::result_type thrust_rewriter::operator()(const bind& n) {
    const expression& rhs = n.rhs();
    if (!detail::isinstance<apply>(rhs)) {
        return get_node_ptr(n);
    }
    const apply& rhs_apply = boost::get<const apply&>(rhs);
    const name& fn_name = rhs_apply.fn();
    auto it_delegate = m_lut.find(fn_name.id());
    if (it_delegate != m_lut.end()) {
        return (it_delegate->second)(n);
    } else {
        return get_node_ptr(n);
    }
}




}
