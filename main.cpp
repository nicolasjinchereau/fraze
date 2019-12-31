/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#include <iostream>
#include <string>
#include "Lexer.h"
#include "Parser.h"
using namespace std;

int main(int argc, char** argv)
{
    auto filename = "test.src";
    Parser parser(filename);
    auto translationUnit = parser.ParseTranslationUnit();

    std::stringstream stream;
    translationUnit->Print(stream, 0, 2);

    cout << stream.str() << endl;

    return 0;
}
