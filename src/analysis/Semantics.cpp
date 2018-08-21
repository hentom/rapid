#include "Semantics.hpp"

#include "Sort.hpp"
#include "Term.hpp"
#include "Formula.hpp"
#include "Theory.hpp"

namespace analysis {
    
    std::shared_ptr<const logic::Formula> Semantics::generateSemantics(const program::Program& program)
    {
        const auto& map = computeEndLocations(program);
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts;
        
        for(const auto& function : program.functions)
        {
            std::vector<std::shared_ptr<const logic::Formula>> conjunctsFunction;

            for (const auto& statement : function->statements)
            {
                auto semantics = generateSemantics(statement, map, function->intVariables, {});
                conjunctsFunction.push_back(semantics);
            }
            conjuncts.push_back(logic::Formulas::conjunction(conjunctsFunction));
        }
        
        return logic::Formulas::conjunction(conjuncts);
    }
    
    std::shared_ptr<const logic::Formula> Semantics::generateSemantics(const std::shared_ptr<const program::Statement> statement, const EndLocationMap& map, const std::vector<std::shared_ptr<const program::IntVariable>>& variables, std::vector<std::shared_ptr<const logic::Term>> iterators)
    {
        if (statement->type() == program::Statement::Type::IntAssignment)
        {
            auto castedStatement = std::static_pointer_cast<const program::IntAssignment>(statement);
            return generateSemantics(castedStatement, map, variables, iterators);
        }
        else if (statement->type() == program::Statement::Type::IfElse)
        {
            auto castedStatement = std::static_pointer_cast<const program::IfElse>(statement);
            return generateSemantics(castedStatement, map, variables, iterators);
        }
        else if (statement->type() == program::Statement::Type::WhileStatement)
        {
            auto castedStatement = std::static_pointer_cast<const program::WhileStatement>(statement);
            return generateSemantics(castedStatement, map, variables, iterators);
        }
        else
        {
            assert(statement->type() == program::Statement::Type::SkipStatement);
            auto castedStatement = std::static_pointer_cast<const program::SkipStatement>(statement);
            return generateSemantics(castedStatement, map, iterators);
        }
    }
    
    std::shared_ptr<const logic::Formula> Semantics::generateSemantics(const std::shared_ptr<const program::IntAssignment> intAssignment, const EndLocationMap& map, const std::vector<std::shared_ptr<const program::IntVariable>>& variables, std::vector<std::shared_ptr<const logic::Term>> iterators)
    {
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts;
        
        auto l1 = startTimePoint(intAssignment, iterators);
        auto l2 = map.at(intAssignment);

        // TODO: implement array-vars
        auto castedLhs = std::static_pointer_cast<const program::IntVariable>(intAssignment->lhs);
        
        // lhs(l2) = rhs(l1)
        auto eq = logic::Formulas::equality(castedLhs->toTerm(l2), intAssignment->rhs->toTerm(l1));
        conjuncts.push_back(eq);
    
        // forall other variables: v(l2) = v(l1)
        for (const auto& var : variables)
        {
            if (*var != *castedLhs)
            {
                auto eq = logic::Formulas::equality(var->toTerm(l2), var->toTerm(l1));
                conjuncts.push_back(eq);
            }
        }
        return logic::Formulas::conjunction(conjuncts);
    }
    
    std::shared_ptr<const logic::Formula> Semantics::generateSemantics(const std::shared_ptr<const program::IfElse> ifElse, const EndLocationMap& map, const std::vector<std::shared_ptr<const program::IntVariable>>& variables, std::vector<std::shared_ptr<const logic::Term>> iterators)
    {
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts;

        auto lStart = startTimePoint(ifElse, iterators);
        auto lEnd = map.at(ifElse);
        auto lLeftStart = startTimePoint(ifElse->ifStatements.front(), iterators);
        auto lLeftEnd = map.at(ifElse->ifStatements.back());
        auto lRightStart = startTimePoint(ifElse->elseStatements.front(), iterators);
        auto lRightEnd = map.at(ifElse->elseStatements.back());
        
        // Part 1: values at the beginning of any branch are the same as at the beginning of the ifElse-statement
        // TODO: sideconditions
        for (const auto& var : variables)
        {
            // v(lLeftStart) = v(lStart)
            auto eq = logic::Formulas::equality(var->toTerm(lLeftStart), var->toTerm(lStart));
            conjuncts.push_back(eq);
        }
        for (const auto& var : variables)
        {
            // v(lRightStart) = v(lStart)
            auto eq = logic::Formulas::equality(var->toTerm(lRightStart), var->toTerm(lStart));
            conjuncts.push_back(eq);
        }
        
        // Part 2: values at the end of the ifElse-statement are either values at the end of the left branch or the right branch,
        // depending on the branching condition
        auto c = ifElse->condition->toFormula(lStart);
        for (const auto& var : variables)
        {
            // condition(lStart) => v(lEnd)=v(lLeftEnd)
            auto eq1 = logic::Formulas::equality(var->toTerm(lEnd), var->toTerm(lLeftEnd));
            conjuncts.push_back(logic::Formulas::implication(c, eq1));
            
            // not condition(lStart) => v(lEnd)=v(lRightEnd)
            auto eq2 = logic::Formulas::equality(var->toTerm(lEnd), var->toTerm(lRightEnd));
            conjuncts.push_back(logic::Formulas::implication(logic::Formulas::negation(c), eq2));
        }
        
        // Part 3: collect all formulas describing semantics of branches
        for (const auto& statement : ifElse->ifStatements)
        {
            auto conjunct = generateSemantics(statement, map, variables, iterators);
            conjuncts.push_back(conjunct);
        }
        for (const auto& statement : ifElse->elseStatements)
        {
            auto conjunct = generateSemantics(statement, map, variables, iterators);
            conjuncts.push_back(conjunct);
        }
        
        return logic::Formulas::conjunction(conjuncts);
    }
    
    std::shared_ptr<const logic::Formula> Semantics::generateSemantics(const std::shared_ptr<const program::WhileStatement> whileStatement, const EndLocationMap& map, const std::vector<std::shared_ptr<const program::IntVariable>>& variables, std::vector<std::shared_ptr<const logic::Term>> iterators)
    {
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts;

        auto i = loopIterator(whileStatement);
        auto n = lastIteration(whileStatement);

        auto iteratorsIt = iterators;
        iteratorsIt.push_back(i);
        
        auto lStart0 = startTimePoint(whileStatement, iterators);
        auto lStartIt = logic::Terms::func(logic::Sorts::timeSort(), whileStatement->location, iteratorsIt);
        auto lBodyStartIt = startTimePoint(whileStatement->bodyStatements.front(), iteratorsIt);
        auto lEnd = map.at(whileStatement);
        

        // Part 1: values at the beginning of body are the same as at the beginning of the while-statement
        // TODO: sideconditions
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts1;
        for (const auto& var : variables)
        {
            // v(lBodyStartIt) = v(lStart(It))
            auto eq = logic::Formulas::equality(var->toTerm(lBodyStartIt), var->toTerm(lStartIt));
            conjuncts1.push_back(eq);
        }
        conjuncts.push_back(logic::Formulas::universal({i}, logic::Formulas::conjunction(conjuncts1)));
        
        // Part 2: collect all formulas describing semantics of body
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts2;
        for (const auto& statement : whileStatement->bodyStatements)
        {
            auto conjunct = generateSemantics(statement, map, variables, iteratorsIt);
            conjuncts2.push_back(conjunct);
        }
        conjuncts.push_back(logic::Formulas::universal({i}, logic::Formulas::conjunction(conjuncts2)));
        
        // Part 3: Define last iteration: Loop condition holds always before n, doesn't hold at n
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts3;
        auto iLessN = logic::Theory::timeSub(i, n);
        auto conditionAtI = whileStatement->condition->toFormula(i);
        auto imp = logic::Formulas::implication(iLessN, conditionAtI);
        conjuncts3.push_back(imp);
        conjuncts.push_back(logic::Formulas::universal({i}, logic::Formulas::conjunction(conjuncts3)));

        auto negConditionAtN = logic::Formulas::negation(whileStatement->condition->toFormula(n));
        conjuncts.push_back(negConditionAtN);
        
        // Part 4: The end-timepoint of the while-statement is the timepoint with location lStart and iteration n
        iterators.push_back(n);
        auto lStartN = logic::Terms::func(logic::Sorts::timeSort(), whileStatement->location, iterators);
        auto eq = logic::Formulas::equality(lEnd, lStartN);
        conjuncts.push_back(eq);

        return logic::Formulas::conjunction(conjuncts);
    }
    std::shared_ptr<const logic::Formula> Semantics::generateSemantics(const std::shared_ptr<const program::SkipStatement> skipStatement, const EndLocationMap& map, std::vector<std::shared_ptr<const logic::Term>> iterators)
    {
        std::vector<std::shared_ptr<const logic::Formula>> conjuncts;
        
        auto l1 = startTimePoint(skipStatement, iterators);
        auto l2 = map.at(skipStatement);

        // identify startTimePoint and endTimePoint
        auto eq = logic::Formulas::equality(l1, l2);
        return eq;
    }
    
# pragma mark - Start locations
    
    std::shared_ptr<const logic::Term> Semantics::startTimePoint(std::shared_ptr<const program::Statement> statement, std::vector<std::shared_ptr<const logic::Term>> iterators)
    {
        if (statement->type() == program::Statement::Type::WhileStatement)
        {
            iterators.push_back(logic::Theory::timeZero());
        }
        return logic::Terms::func(logic::Sorts::timeSort(), statement->location, iterators);
    }

# pragma mark - End location computation
    
    typedef std::unordered_map<const std::shared_ptr<const program::Statement>, std::shared_ptr<const logic::Term>, program::StatementSharedPtrHash> EndLocationMap;

    /*
     * compute end locations for each statement (used later for semantics)
     * first compute end location of statement, then recurse on children (since their end location could depend on end location of statement)
     */
    EndLocationMap Semantics::computeEndLocations(const program::Program& program)
    {
        EndLocationMap map;
        
        for(const auto& function : program.functions)
        {
            // for each statement except the first, set the end-location of the previous statement to the begin-location of this statement
            for(int i=1; i < function->statements.size(); ++i)
            {
                const auto& currentStatement = function->statements[i];
                const auto& lastStatement = function->statements[i-1];
                
                map[lastStatement] = startTimePoint(currentStatement, {});
            }
            // for the last statement, set the end-location to be the end-location of the function.
            auto t = logic::Terms::func(logic::Sorts::timeSort(), function->name + "_end", {});
            map[function->statements.back()] = t;
            
            // recurse on the statements
            for(const auto& statement : function->statements)
            {
                computeEndLocations(statement, map, {});
            }
        }
        
        return map;
    }
    
    void Semantics::computeEndLocations(const std::shared_ptr<const program::Statement> statement, EndLocationMap& map, std::vector<std::shared_ptr<const logic::Term>> iterators)
    {
        if (statement->type() == program::Statement::Type::IfElse)
        {
            auto castedStatement = std::static_pointer_cast<const program::IfElse>(statement);
            computeEndLocations(castedStatement, map, iterators);
        }
        else if (statement->type() == program::Statement::Type::WhileStatement)
        {
            auto castedStatement = std::static_pointer_cast<const program::WhileStatement>(statement);
            computeEndLocations(castedStatement, map, iterators);
        }
        else
        {
            // no recursion needed for these statements
            assert(statement->type() == program::Statement::Type::IntAssignment ||
                   statement->type() == program::Statement::Type::SkipStatement);
        }
    }

    void Semantics::computeEndLocations(const std::shared_ptr<const program::IfElse> ifElse, EndLocationMap& map, std::vector<std::shared_ptr<const logic::Term>> iterators)
    {
        // for each statement in the left branch except the first, set the end-location of the previous statement to the begin-location of this statement
        for(int i=1; i < ifElse->ifStatements.size(); ++i)
        {
            const auto& currentStatement = ifElse->ifStatements[i];
            const auto& lastStatement = ifElse->ifStatements[i-1];
            
            map[lastStatement] = startTimePoint(currentStatement, iterators);
        }
        // use a new location as end location for last statement of the left branch
        auto t1 = logic::Terms::func(logic::Sorts::timeSort(), ifElse->location + "_lEnd", iterators);
        map[ifElse->ifStatements.back()] = t1;
        
        // for each statement in the right branch except the first, set the end-location of the previous statement to the begin-location of this statement
        for(int i=1; i < ifElse->elseStatements.size(); ++i)
        {
            const auto& currentStatement = ifElse->elseStatements[i];
            const auto& lastStatement = ifElse->elseStatements[i-1];
            
            map[lastStatement] = startTimePoint(currentStatement, iterators);
        }
        // use a new location as end location for last statement of the right branch
        auto t2 = logic::Terms::func(logic::Sorts::timeSort(), ifElse->location + "_rEnd", iterators);
        map[ifElse->elseStatements.back()] = t2;

        // recurse on the statements
        for(const auto& statement : ifElse->ifStatements)
        {
            computeEndLocations(statement, map, iterators);
        }
        for(const auto& statement : ifElse->elseStatements)
        {
            computeEndLocations(statement, map, iterators);
        }
    }
    
    void Semantics::computeEndLocations(const std::shared_ptr<const program::WhileStatement> whileStatement, EndLocationMap& map, std::vector<std::shared_ptr<const logic::Term>> iterators)
    {
        // for the last statement in the body, set the end-location to be the start-location of the while-statement in the next iteration
        auto iteratorsCopy = iterators;
        auto t1 = loopIterator(whileStatement);
        auto t2 = logic::Theory::timeSucc(t1);
        iteratorsCopy.push_back(t2);
        
        auto t3 = logic::Terms::func(logic::Sorts::timeSort(), whileStatement->location, iteratorsCopy);
        map[whileStatement->bodyStatements.back()] = t3;
        
        // for each statement in the body except the first, set the end-location of the previous statement to the begin-location of this statement
        iterators.push_back(loopIterator(whileStatement));
        
        for(int i=1; i < whileStatement->bodyStatements.size(); ++i)
        {
            const auto& currentStatement = whileStatement->bodyStatements[i];
            const auto& lastStatement = whileStatement->bodyStatements[i-1];
            
            map[lastStatement] = startTimePoint(currentStatement, iterators);
        }
        
        // recurse on the statements
        for(const auto& statement : whileStatement->bodyStatements)
        {
            computeEndLocations(statement, map, iterators);
        }
    }
    
    std::shared_ptr<const logic::LVariable> Semantics::loopIterator(std::shared_ptr<const program::WhileStatement> whileStatement)
    {
        return logic::Terms::var(logic::Sorts::timeSort(), "It" + whileStatement->location);
    }
    
    std::shared_ptr<const logic::FuncTerm> Semantics::lastIteration(std::shared_ptr<const program::WhileStatement> whileStatement)
    {
        return logic::Terms::func(logic::Sorts::timeSort(), "n" + whileStatement->location, {});
    }
}
