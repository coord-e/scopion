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

        scopion::assembly asmb( outbin );
        asmb.IRGen(asts);

        char *tmpname = strdup("/tmp/tmpfileXXXXXX");
        mkstemp(tmpname);
        std::string tmpstr(tmpname);
        std::ofstream f(tmpstr);
        f << asmb.getIR();
        f.close();
        system(("llc " + tmpstr).c_str());
        system(("gcc " + tmpstr + ".s -o " + std::string(outbin)).c_str());
    }
    catch( std::exception const& e ) {
        std::cerr << e.what() << std::endl;
    }
}
