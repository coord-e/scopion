#ifndef SCOPION_AST_H_
#define SCOPION_AST_H_

#include "scopion/ast/attribute.h"
#include "scopion/ast/expr.h"
#include "scopion/ast/operators.h"
#include "scopion/ast/util.h"
#include "scopion/ast/value.h"
#include "scopion/ast/value_wrapper.h"

#include <iostream>

std::ostream &operator<<(std::ostream &os, scopion::ast::expr const &tree);

#endif // SCOPION_AST_H_
