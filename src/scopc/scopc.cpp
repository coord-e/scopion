#include <stdexcept>
#include <iostream>
#include <memory>
#include <cstdio>
#include <fstream>
#include <functional>

#include "AST/AST.h"
#include "Parser/Parser.h"
#include "Assembly/Assembly.h"

int main(int argc, char* argv[])
{
    try {
        if( argc != 3 ) {
            throw std::runtime_error( "invalid arguments" );
        }

        const std::string outbin(argv[2]);

        std::ifstream ifs(argv[1]);
        if (ifs.fail())
        {
            std::cerr << "Failed to open: " << argv[1] << std::endl;
            throw std::runtime_error( "Failed to start parsing." );
        }

        std::string code((std::istreambuf_iterator<char>(ifs)),
            std::istreambuf_iterator<char>());

        auto asts = scopion::parser::parse(code);

        scopion::assembly asmb( outbin );
        asmb.IRGen(asts);

        char* tmpname = strdup("/tmp/tmpfileXXXXXX");
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
