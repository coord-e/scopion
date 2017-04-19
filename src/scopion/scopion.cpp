#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <fstream>
#include <functional>
#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_LIST_SIZE 50
#include <boost/variant.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/range/adaptor/indexed.hpp>

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
    struct assign;

    template <class Op>
    struct binary_op;

    using expr = boost::variant<
        int,
        bool,
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
        boost::recursive_wrapper< binary_op< ltq > >,
        boost::recursive_wrapper< binary_op< assign > >
    >;

    template <class Op>
    struct binary_op
    {
        expr lhs;
        expr rhs;

        binary_op(expr const& lhs_, expr const& rhs_) :
            lhs( lhs_ ), rhs( rhs_ )
        { }
    };

}; // namespace ast

namespace parser {

    namespace qi = boost::spirit::qi;

    template <class Iterator>
    struct arith_grammar :
        qi::grammar< Iterator, ast::expr(), qi::ascii::space_type >
    {
        template <class T>
        using rule_t = qi::rule< Iterator, T, qi::ascii::space_type >;

        /*std::array< std::map< std::string, ast::expr() >, 6 > operators{ //The higher priority, the higher index
          { {"|", ast::ior::value} },
          { {"^", ast::ixor::value} },
          { {"&", ast::iand::value} },
          { {">>", ast::shr::value}, {"<<", ast::shl::value} },
          { {"-", ast::sub::value}, {"+", ast::add::value} },
          { {"*", ast::mul::value}, {"/", ast::div::value}, {"%", ast::rem::value} },
        };*/

        std::array< rule_t< ast::expr() >, 11> rules;

        arith_grammar() :
            arith_grammar::base_type( rules.back() )
        {
            namespace phx = boost::phoenix;

            for(auto i=0; i<rules.size(); i++){
              switch(i){
                case 0:
                  rules[i] %= qi::int_ | qi::bool_ | qi::char_ | ( '(' >> rules.back() >> ')' )[qi::_val = qi::_1];
                  break;

                case 1:
                  rules[i] %= rules[i-1][qi::_val = qi::_1]
                      >> *(
                          ( '*' >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::mul > >( qi::_val, qi::_1 )] )
                          | ( '/' >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::div > >( qi::_val, qi::_1 )] )
                          | ( '%' >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::rem > >( qi::_val, qi::_1 )] )
                      );
                  break;

                case 2:
                  rules[i] %= rules[i-1][qi::_val = qi::_1]
                      >> *(
                          ( '+' >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::add > >( qi::_val, qi::_1 )] )
                          | ( '-' >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::sub > >( qi::_val, qi::_1 )] )
                      );
                  break;

                case 3:
                  rules[i] %= rules[i-1][qi::_val = qi::_1]
                      >> *(
                          ( ">>" >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::shr > >( qi::_val, qi::_1 )] )
                          | ( "<<" >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::shl > >( qi::_val, qi::_1 )] )
                      );
                  break;

                case 4:
                  rules[i] %= rules[i-1][qi::_val = qi::_1]
                      >> *(
                          ( ">" >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::gt > >( qi::_val, qi::_1 )] )
                          | ( "<" >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::lt > >( qi::_val, qi::_1 )] )
                          | ( ">=" >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::gtq > >( qi::_val, qi::_1 )] )
                          | ( "<=" >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::ltq > >( qi::_val, qi::_1 )] )
                      );
                  break;

                case 5:
                  rules[i] %= rules[i-1][qi::_val = qi::_1]
                      >> *(
                          ( "==" >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::eeq > >( qi::_val, qi::_1 )] )
                          | ( "!=" >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::neq > >( qi::_val, qi::_1 )] )
                      );
                  break;

                case 6:
                  rules[i] %= rules[i-1][qi::_val = qi::_1]
                      >> *(
                          ( ( qi::lit("&") - qi::lit("&&") ) >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::iand > >( qi::_val, qi::_1 )] )
                      );
                  break;

                case 7:
                  rules[i] %= rules[i-1][qi::_val = qi::_1]
                      >> *(
                          ( "^" >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::ixor > >( qi::_val, qi::_1 )] )
                      );
                  break;

                case 8:
                  rules[i] %= rules[i-1][qi::_val = qi::_1]
                      >> *(
                          ( ( qi::lit("|") - qi::lit("||") ) >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::ior > >( qi::_val, qi::_1 )] )
                      );
                  break;

                case 9:
                  rules[i] %= rules[i-1][qi::_val = qi::_1]
                      >> *(
                          ( "&&" >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::land > >( qi::_val, qi::_1 )] )
                      );
                  break;

                case 10:
                  rules[i] %= rules[i-1][qi::_val = qi::_1]
                      >> *(
                          ( "||" >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::lor > >( qi::_val, qi::_1 )] )
                      );
                  break;

                /*case 11:
                  rules[i] %= rules[i-1][qi::_val = qi::_1]
                      >> *(
                          ( "=" >> rules[i-1][qi::_val = phx::construct< ast::binary_op< ast::assign > >( qi::_val, qi::_1 )] )
                      );
                  break;*/
              }
            }

        }
    };

}; // namespace parser

class asemmbly :
    public boost::static_visitor< llvm::Value* >
{
    llvm::IRBuilder<>& builder_;

public:
    asemmbly(llvm::IRBuilder<>& builder) :
        boost::static_visitor< llvm::Value* >(),
        builder_( builder )
    { }

    llvm::Value* operator()(int value)
    {
        return llvm::ConstantInt::get( builder_.getInt32Ty(), value );
    }

    template <class Op>
    llvm::Value* operator()(ast::binary_op< Op > const& op)
    {
        llvm::Value* lhs = boost::apply_visitor( *this, op.lhs );
        llvm::Value* rhs = boost::apply_visitor( *this, op.rhs );

        return apply_op( op, lhs, rhs );
    }

private:
    llvm::Value* apply_op(ast::binary_op< ast::add > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateAdd( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::sub > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateSub( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::mul > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateMul( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::div > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateSDiv( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::rem > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateSRem( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::shl > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateShl( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::shr > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateLShr( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::iand > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateAnd( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::ior > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateOr( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::ixor > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateXor( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::land > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateAnd(builder_.CreateICmpNE( lhs, llvm::Constant::getNullValue(builder_.getInt1Ty())),
                                  builder_.CreateICmpNE( rhs, llvm::Constant::getNullValue(builder_.getInt1Ty())));
    }

    llvm::Value* apply_op(ast::binary_op< ast::lor > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateOr(builder_.CreateICmpNE( lhs, llvm::Constant::getNullValue(builder_.getInt1Ty())),
                                 builder_.CreateICmpNE( rhs, llvm::Constant::getNullValue(builder_.getInt1Ty())));
    }

    llvm::Value* apply_op(ast::binary_op< ast::eeq > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateICmpEQ( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::neq > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateICmpNE( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::gt > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateICmpSGT( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::lt > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateICmpSLT( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::gtq > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateICmpSGE( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::ltq > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        return builder_.CreateICmpSLE( lhs, rhs );
    }

    llvm::Value* apply_op(ast::binary_op< ast::assign > const&, llvm::Value* lhs, llvm::Value* rhs)
    {
        builder_.CreateAlloca(builder_.getInt32Ty());
        return rhs;
    }
};

std::vector< std::string > read_file(char const* filename)
{
    std::ifstream ifs( filename, std::ios::binary );
    if( ifs.fail() ) {
        return {};
    }

    std::vector< std::string > result;
    for( std::string line; std::getline( ifs, line ); ) {
        result.push_back( line );
    }

    return result;
}

std::string print_llvm_lang(std::unique_ptr< llvm::Module > const& module)
{
    std::string result;
    llvm::raw_string_ostream stream( result );

    module->print( stream, nullptr );

    return result;
}

void make_main_func(llvm::LLVMContext& context, std::unique_ptr< llvm::Module >& module, llvm::IRBuilder<>& builder)
{
    auto* main_func = llvm::Function::Create(
        llvm::FunctionType::get( builder.getInt32Ty(), false ),
        llvm::Function::ExternalLinkage,
        "main",
        module.get()
    );

    builder.SetInsertPoint( llvm::BasicBlock::Create( context, "entry", main_func ) );
}

llvm::Constant* make_printf(std::unique_ptr< llvm::Module >& module, llvm::IRBuilder<>& builder)
{
    std::vector< llvm::Type* > args = {
        builder.getInt8Ty()->getPointerTo()
    };

    return module->getOrInsertFunction(
        "printf",
        llvm::FunctionType::get( builder.getInt32Ty(), llvm::ArrayRef< llvm::Type* >( args ), true )
    );
}

void make_ret_main(llvm::IRBuilder<>& builder)
{
    builder.CreateRet( llvm::ConstantInt::get( builder.getInt32Ty(), 0 ) );
}

int main(int argc, char* argv[])
{
    try {
        if( argc != 2 ) {
            throw std::runtime_error( "invalid arguments" );
        }

        auto const code = read_file( argv[1] );
        if( code.empty() ) {
            throw std::runtime_error( "cannot read the file" );
        }

        parser::arith_grammar< std::string::const_iterator > grammar;
        std::vector< ast::expr > asts;

        int line = 0;
        bool err = false;
        for( auto const& i : code ) {
            ast::expr tree;

            ++line;
            if( !boost::spirit::qi::phrase_parse( i.begin(), i.end(), grammar, boost::spirit::qi::ascii::space, tree ) ) {
                std::cerr << argv[1] << ":" << line << ": parse error" << std::endl;
                err = true;
                continue;
            }

            asts.push_back( tree );
        }

        if( err ) {
            throw std::runtime_error( "detected errors" );
        }

        llvm::LLVMContext context;
        std::unique_ptr< llvm::Module > module( new llvm::Module( "arith", context ) );
        llvm::IRBuilder<> builder( context );

        make_main_func( context, module, builder );

        auto* printf_func = make_printf( module, builder );
        auto* format = builder.CreateGlobalStringPtr( "%d\n" );

        asemmbly asm_obj( builder );
        for( auto const& i : asts ) {
            std::vector< llvm::Value* > args = {
                format, boost::apply_visitor( asm_obj, i )
            };
            builder.CreateCall( printf_func, llvm::ArrayRef< llvm::Value* >( args ) );
        }

        make_ret_main( builder );

        std::cout << print_llvm_lang( module );
    }
    catch( std::exception const& e ) {
        std::cerr << e.what() << std::endl;
    }
}
