#ifndef SCOPION_PARSER_H_
#define SCOPION_PARSER_H_

#define BOOST_SPIRIT_USE_PHOENIX_V3 1

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include "AST/AST.h"

namespace scopion{
namespace parser {

    namespace qi = boost::spirit::qi;

    template <class Iterator>
    struct grammar :
        qi::grammar< Iterator, ast::expr(), qi::ascii::space_type >
    {
        template <class T>
        using rule_t = qi::rule< Iterator, T, qi::ascii::space_type>;

        std::array< rule_t< ast::expr() >, 13> rules;

        grammar() :
            grammar::base_type( rules.back() )
        {
            namespace phx = boost::phoenix;

            int i=-1;

            rules[i+1] %= qi::int_
                          | qi::bool_
                          | qi::as_string[ '"' >> *(qi::char_ - '"') >> '"' ][qi::_val = qi::_1]
                          | ( '(' >> rules.back() >> ')' )[qi::_val = qi::_1];

            i++;
            rules[i+1] %=    ( rules[i][qi::_val = phx::construct< ast::binary_op< ast::add > >( qi::_1, 1 )] >> "++" )
                          |  ( rules[i][qi::_val = phx::construct< ast::binary_op< ast::sub > >( qi::_1, 1 )] >> "--" )
                          //|  ( *(qi::char_)[qi::_val = qi::_1] >> '(' >> *( rules[i] % qi::char_(',') )[qi::_val = phx::construct< ast::binary_op< ast::call > >( qi::_val, qi::_1 )] >> ')' )
                          |  rules[i][qi::_val = qi::_1];

            i++;
            rules[i+1] %= rules[i][qi::_val = qi::_1]
                          | ( '!' >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::ixor > >( qi::_1, 1 )] )
                          | ( '~' >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::ixor > >( qi::_1, 1 )] )
                          | ( "++" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::add > >( qi::_1, 1 )] )
                          | ( "--" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::sub > >( qi::_1, 1 )] )
                          | ( ( qi::lit("+") - qi::lit("++") ) >> rules[i][qi::_val = qi::_1] )
                          | ( ( qi::lit("-") - qi::lit("--") ) >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::sub > >( 0, qi::_1 )] );

            i++;
            rules[i+1] %= rules[i][qi::_val = qi::_1]
                >> *(
                    ( '*' >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::mul > >( qi::_val, qi::_1 )] )
                    | ( '/' >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::div > >( qi::_val, qi::_1 )] )
                    | ( '%' >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::rem > >( qi::_val, qi::_1 )] )
                );

            i++;
            rules[i+1] %= rules[i][qi::_val = qi::_1]
                >> *(
                    ( '+' >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::add > >( qi::_val, qi::_1 )] )
                    | ( '-' >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::sub > >( qi::_val, qi::_1 )] )
                );

            i++;
            rules[i+1] %= rules[i][qi::_val = qi::_1]
                >> *(
                    ( ">>" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::shr > >( qi::_val, qi::_1 )] )
                    | ( "<<" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::shl > >( qi::_val, qi::_1 )] )
                );

            i++;
            rules[i+1] %= rules[i][qi::_val = qi::_1]
                >> *(
                    ( ">" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::gt > >( qi::_val, qi::_1 )] )
                    | ( "<" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::lt > >( qi::_val, qi::_1 )] )
                    | ( ">=" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::gtq > >( qi::_val, qi::_1 )] )
                    | ( "<=" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::ltq > >( qi::_val, qi::_1 )] )
                );

            i++;
            rules[i+1] %= rules[i][qi::_val = qi::_1]
                >> *(
                    ( "==" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::eeq > >( qi::_val, qi::_1 )] )
                    | ( "!=" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::neq > >( qi::_val, qi::_1 )] )
                );

            i++;
            rules[i+1] %= rules[i][qi::_val = qi::_1]
                >> *(
                    ( ( qi::lit("&") - qi::lit("&&") ) >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::iand > >( qi::_val, qi::_1 )] )
                );

            i++;
            rules[i+1] %= rules[i][qi::_val = qi::_1]
                >> *(
                    ( "^" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::ixor > >( qi::_val, qi::_1 )] )
                );

            i++;
            rules[i+1] %= rules[i][qi::_val = qi::_1]
                >> *(
                    ( ( qi::lit("|") - qi::lit("||") ) >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::ior > >( qi::_val, qi::_1 )] )
                );

            i++;
            rules[i+1] %= rules[i][qi::_val = qi::_1]
                >> *(
                    ( "&&" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::land > >( qi::_val, qi::_1 )] )
                );

            i++;
            rules[i+1] %= rules[i][qi::_val = qi::_1]
                >> *(
                    ( "||" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::lor > >( qi::_val, qi::_1 )] )
                );

            /*i++;
            rules[i+1] %= rules[i][qi::_val = qi::_1]
                >> *(
                    ( "=" >> rules[i][qi::_val = phx::construct< ast::binary_op< ast::assign > >( qi::_val, qi::_1 )] )
                );*/
            assert(i+2 == rules.size());
        }
    };

}; // namespace parser
}; //namespace scopion

#endif //SCOPION_PARSER_H_
