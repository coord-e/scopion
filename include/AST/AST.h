#ifndef SCOPION_AST_H_
#define SCOPION_AST_H_

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_LIST_SIZE 50
#define BOOST_SPIRIT_USE_PHOENIX_V3 1

#include <boost/variant.hpp>
#include <iostream>

namespace scopion{

namespace ast {

    struct add;
    struct sub;
    struct mul;
    struct div;
    struct rem;
    struct shl;
    struct shr;
    struct iand;
    struct ior;
    struct ixor;
    struct land;
    struct lor;
    struct eeq;
    struct neq;
    struct gt;
    struct lt;
    struct gtq;
    struct ltq;
    //struct assign;

    template <class Op>
    struct binary_op;

    using expr = boost::variant<
        int,
        bool,
        boost::recursive_wrapper< std::string >, //TODO: figure out why std::string needs wrapper to work
        boost::recursive_wrapper< binary_op< add > >,
        boost::recursive_wrapper< binary_op< sub > >,
        boost::recursive_wrapper< binary_op< mul > >,
        boost::recursive_wrapper< binary_op< div > >,
        boost::recursive_wrapper< binary_op< rem > >,
        boost::recursive_wrapper< binary_op< shl > >,
        boost::recursive_wrapper< binary_op< shr > >,
        boost::recursive_wrapper< binary_op< iand > >,
        boost::recursive_wrapper< binary_op< ior > >,
        boost::recursive_wrapper< binary_op< ixor > >,
        boost::recursive_wrapper< binary_op< land > >,
        boost::recursive_wrapper< binary_op< lor > >,
        boost::recursive_wrapper< binary_op< eeq > >,
        boost::recursive_wrapper< binary_op< neq > >,
        boost::recursive_wrapper< binary_op< gt > >,
        boost::recursive_wrapper< binary_op< lt > >,
        boost::recursive_wrapper< binary_op< gtq > >,
        boost::recursive_wrapper< binary_op< ltq > >
        //boost::recursive_wrapper< binary_op< assign > >
    >;

    template <class Op>
    struct binary_op
    {
        expr lhs;
        expr rhs;

        binary_op(expr const& lhs_, expr const& rhs_) :
            lhs( lhs_ ), rhs( rhs_ )
        {
          if(lhs_.which() == 0 && rhs_.which() == 0){
            std::cout << "/** Hello from binop ctor **/" << std::endl;
            std::cout << "lhs = " << boost::get<int>(lhs_) << ", rhs = " << boost::get<int>(rhs_) << std::endl;
          }
        }
    };

}; // namespace ast
}; //namespace scopion

#endif //SCOPION_AST_H_
