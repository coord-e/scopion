#include <llvm/Support/raw_ostream.h>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <fstream>
#include <functional>

#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_LIST_SIZE 50
#include <boost/spirit/include/qi.hpp>

#include "AST/AST.h"
#include "Parser/Parser.h"
#include "Assembly/Assembly.h"


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

        scopion::parser::grammar< std::string::const_iterator > grammar;
        std::vector< scopion::ast::expr > asts;

        int line = 0;
        bool err = false;
        for( auto const& i : code ) {
            scopion::ast::expr tree;

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

        auto* main_func = llvm::Function::Create(
            llvm::FunctionType::get( builder.getInt32Ty(), false ),
            llvm::Function::ExternalLinkage,
            "main",
            module.get()
        );

        builder.SetInsertPoint( llvm::BasicBlock::Create( context, "entry", main_func ) );

        auto* printf_func = make_printf( module, builder );
        auto* format = builder.CreateGlobalStringPtr( "%d\n" );

        scopion::assembly asm_obj( builder );
        for( auto const& i : asts ) {
            std::vector< llvm::Value* > args = {
                format, boost::apply_visitor( asm_obj, i )
            };
            builder.CreateCall( printf_func, llvm::ArrayRef< llvm::Value* >( args ) );
        }

        builder.CreateRet( llvm::ConstantInt::get( builder.getInt32Ty(), 0 ) );

        std::cout << print_llvm_lang( module );
    }
    catch( std::exception const& e ) {
        std::cerr << e.what() << std::endl;
    }
}
