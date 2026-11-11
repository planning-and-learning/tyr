#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>

using Term = tyr::formalism::Term;

// A term is the variant itself, stored as it is; read through a repository it is the variant of the object's view and
// the parameter.
static_assert(std::totally_ordered<Term>);
static_assert(std::same_as<tyr::formalism::planning::TermView, std::variant<tyr::formalism::planning::ObjectView, tyr::formalism::ParameterIndex>>);
static_assert(std::same_as<tyr::formalism::datalog::TermView, std::variant<tyr::formalism::datalog::ObjectView, tyr::formalism::ParameterIndex>>);

TEST(TyrFormalismTerm, PreservesParameterAlternative)
{
    const auto term = Term(tyr::formalism::ParameterIndex(3));
    ASSERT_TRUE(holds_alternative<tyr::formalism::ParameterIndex>(term));
    EXPECT_EQ(get<tyr::formalism::ParameterIndex>(term), tyr::formalism::ParameterIndex(3));
}

TEST(TyrFormalismTerm, ReadoutFollowsReferencedObjectsToTheirLayer)
{
    auto parent = tyr::formalism::planning::Repository();
    auto object_data = urd::data<tyr::formalism::Object>(std::string("parent"));
    const auto object = tyr::formalism::planning::insert(parent, object_data).first;
    auto child = tyr::formalism::planning::Repository(&parent);
    object_data.name = "child";
    const auto child_object = tyr::formalism::planning::insert(child, object_data).first;

    const auto parent_term = urd::resolve(Term(object.index()), child);
    const auto child_term = urd::resolve(Term(child_object.index()), child);
    const auto parameter = urd::resolve(Term(tyr::formalism::ParameterIndex(0)), child);
    EXPECT_EQ(&std::get<tyr::formalism::planning::ObjectView>(parent_term).context(), &parent);
    EXPECT_EQ(&std::get<tyr::formalism::planning::ObjectView>(child_term).context(), &child);
    EXPECT_EQ(std::get<tyr::formalism::ParameterIndex>(parameter), tyr::formalism::ParameterIndex(0));
    EXPECT_EQ(urd::to_stored(child_term), Term(child_object.index()));
}
