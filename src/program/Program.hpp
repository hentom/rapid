#ifndef __Program__
#define __Program__

#include <memory>
#include <vector>

#include "Expression.hpp"
#include "Assignment.hpp"

namespace program
{
    class Function
    {
    public:
        Function(std::string name, std::vector<std::shared_ptr<const IntVariable>> intVariables, std::vector<std::shared_ptr<const Statement>> statements) : name(name), intVariables(std::move(intVariables)), statements(std::move(statements)) {}
        
        const std::string name;
        const std::vector<std::shared_ptr<const IntVariable>> intVariables;
        const std::vector<std::shared_ptr<const Statement>> statements;
    };
    std::ostream& operator<<(std::ostream& ostr, const Function& p);

    class Program
    {
    public:
        Program(std::vector< std::shared_ptr<const Function>> functions) : functions(std::move(functions)) {}
        
        const std::vector< std::shared_ptr<const Function>> functions;
    };
    std::ostream& operator<<(std::ostream& ostr, const Program& p);
}

#endif
