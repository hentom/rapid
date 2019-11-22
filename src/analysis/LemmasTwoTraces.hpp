#ifndef __LemmasTwoTraces__
#define __LemmasTwoTraces__

#include <vector>
#include <memory>
#include <unordered_set>

#include "ProgramTraverser.hpp"
#include "Problem.hpp"
#include "Program.hpp"

namespace analysis {
    
    /*
     * We use the lemmas in this header to cover the inductive reasoning which relates values and timepoints of two execution-traces.
     */
    
    /*
     * LEMMA 1
     * If a variable has the same value (or if an array has at one position the same value) in both traces at some iteration,
     * and if that equality is preserved in an interval, then the equality holds during any point of the interval (including the end of the interval).
     *
     * Soundness: This lemma is exactly the instantiation of the following standard induction axiom scheme with P(it) := v(l(it),t1)=v(l(it),t2)
     * forall boundL,boundR.
     *    =>
     *       and
     *          P(boundL)
     *          forall it.
     *             =>
     *                and
     *                   boundL<=it<boundR
     *                   P(it)
     *                P(s(it))
     *       forall it.
     *          =>
     *             boundL<=it<=boundR
     *             P(it)
     *
     * Variations:
     *  The currently used version is the most general one, and allows arbitrary intervals.
     *  More specific versions could fix the left bound to 0 and/or the right bound to the last iteration of one trace (wlog. trace 1).
     *
     * Why is this lemma useful
     *  This lemma is central for reasoning about relational properties.
     */
    class EqualityPreservationTracesLemmas : public ProgramTraverser<std::vector<std::shared_ptr<const logic::ProblemItem>>>
    {
    public:
        using ProgramTraverser::ProgramTraverser; // inherit initializer, note: doesn't allow additional members in subclass!
        
    private:
        virtual void generateOutputFor(const program::WhileStatement* statement,  std::vector<std::shared_ptr<const logic::ProblemItem>>& items) override;
    };
    
    /*
     * LEMMA 2
     * if all variables used in the loop iteration have the same value in both traces in each iteration,
     * then the loops terminate after the same number of steps
     *
     * =>
     *    and
     *       EqVC
     *       IH(0)
     *       IC
     *    n(t1)=n(t2)
     * where:
     * - EqVC :=
     *   "forall" const variables v
     *      v(t1) = v(t2)
     * - IH(it) :=
     *   "forall" non-const variables v
     *      v(l(it),t1) = v(l(it),t2)
     * - IC :=
     *   forall it.
     *      =>
     *         and
     *            it<n(t1)
     *            it<n(t2)
     *            IH(it)
     *         IH(s(it))
     *
     * Soundness:
     * This lemma follows from
     * - the semantics
     * - inductionAxiom2 instantiated with the IH EqV(it)
     * - the theory axioms
     *   1) totality of Nat
     *   2) TODO: do we need further theory axioms?
     *
     * Why is this lemma useful?
     * Most relational properties only hold, if the number of iterations of the involved loops is the same in both traces.
     */
    class NEqualLemmas : public ProgramTraverser<std::vector<std::shared_ptr<const logic::ProblemItem>>>
    {
    public:
        NEqualLemmas(
            const program::Program& program,
            std::unordered_map<std::string, std::vector<std::shared_ptr<const program::Variable>>> locationToActiveVars,
            bool twoTraces,
            std::vector<std::shared_ptr<const logic::Axiom>> programSemantics) :
            ProgramTraverser<std::vector<std::shared_ptr<const logic::ProblemItem>>>(program, locationToActiveVars, twoTraces), programSemantics(programSemantics) {}

    private:
        std::vector<std::shared_ptr<const logic::Axiom>> programSemantics;

        virtual void generateOutputFor(const program::WhileStatement* statement,  std::vector<std::shared_ptr<const logic::ProblemItem>>& items) override;
        void computeVariablesContainedInLoopCondition(std::shared_ptr<const program::BoolExpression> expr, std::unordered_set<std::shared_ptr<const program::Variable>>& variables);
        void computeVariablesContainedInLoopCondition(std::shared_ptr<const program::IntExpression> expr, std::unordered_set<std::shared_ptr<const program::Variable>>& variables);
    };
    

    
}

#endif

