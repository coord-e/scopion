#include <llvm/Support/raw_ostream.h>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <cstdio>
#include <fstream>
#include <functional>

#include "AST/AST.h"
#include "Parser/Parser.h"
#include "Assembly/Assembly.h"

#include <boost/spirit/include/qi.hpp>

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

template <class Op>
void printExpr2(scopion::ast::binary_op< Op > const& op, int l){
  switch(op.lhs.which()){
    case 0:
     std::cout << std::string(l, ' ') << "lhs(int)=" << boost::get<int>(op.lhs) << ", ";
     break;
   case 1:
    std::cout << std::string(l, ' ') << "lhs(bool)" << boost::get<bool>(op.lhs) << ", ";
    break;
  case 2:
   std::cout << std::string(l, ' ') << "lhs(str)=" << boost::get<std::string>(op.lhs) << ", ";
   break;
   default:
    printExpr2(op.lhs, l+1);
    break;
  }

switch(op.rhs.which()){
  case 0:
   std::cout << "rhs(int)=" << boost::get<int>(op.rhs) << std::endl;
   break;
 case 1:
  std::cout << "rhs(bool)=" << boost::get<bool>(op.rhs) << std::endl;
  break;
case 2:
 std::cout << "rhs(str)=" << boost::get<std::string>(op.rhs) << std::endl;
 break;
 default:
  printExpr2(op.rhs, l+1);
  break;
    }
}



void printExpr(int v){
  std::cout << "ohInt" << v << std::endl;
}

void printExpr(bool v){
  std::cout << "ohBool" << v << std::endl;
}

void printExpr(std::string v){
  std::cout << "ohStr" << v << std::endl;
}

template <class Op>
void printExpr(Op const& op){
  std::cout << "/*********** hello from printexpr. ***********/ " << std::endl;
    switch(op.which()){
      case 0:
       std::cout << "detected int lhs value=" << boost::get<int>(op) << std::endl;
       break;
     case 1:
      std::cout << "detected bool lhs value=" << boost::get<bool>(op) << std::endl;
      break;
    case 2:
     std::cout << "detected string lhs value=" << boost::get<std::string>(op) << std::endl;
     break;
     default:
      printExpr2(op, 0);
      break;
    }
}

int main(int argc, char* argv[])
{
    try {
        if( argc != 3 ) {
            throw std::runtime_error( "invalid arguments" );
        }

        const std::string outbin(argv[2]);

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
        std::unique_ptr< llvm::Module > module( new llvm::Module( outbin, context ) );
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
          //if(i.which() == 3) std::cout << "lhs=" << boost::get< scopion::ast::binary_op< scopion::ast::add > >(i).lhs << ", rhs=" << boost::get< scopion::ast::binary_op< scopion::ast::add > >(i).rhs << std::endl;
            std::vector< llvm::Value* > args = {
                format, boost::apply_visitor( asm_obj, i )
            };
            builder.CreateCall( printf_func, llvm::ArrayRef< llvm::Value* >( args ) );
        }

        builder.CreateRet( llvm::ConstantInt::get( builder.getInt32Ty(), 0 ) );

        char *tmpname = strdup("/tmp/tmpfileXXXXXX");
        mkstemp(tmpname);
        std::string tmpstr(tmpname);
        std::ofstream f(tmpstr);
        f << print_llvm_lang( module );
        f.close();
        system(("llc " + tmpstr).c_str());
        system(("gcc " + tmpstr + ".s -o " + std::string(outbin)).c_str());
    }
    catch( std::exception const& e ) {
        std::cerr << e.what() << std::endl;
    }
}
