#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/repository.hpp"

#include <cista/containers/variant.h>
#include <concepts>
#include <gtest/gtest.h>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <variant>

using Term = tyr::formalism::Term;

// A term is the variant itself, stored as it is; read through a repository it is the variant of the object's view and
// the parameter.
static_assert(std::totally_ordered<Term>);
static_assert(std::same_as<tyr::formalism::TermView, std::variant<tyr::formalism::ObjectView, tyr::formalism::ParameterIndex>>);
static_assert(std::same_as<tyr::formalism::TermView, std::variant<tyr::formalism::ObjectView, tyr::formalism::ParameterIndex>>);

TEST(TyrFormalismTerm, PreservesParameterAlternative)
{
    const auto term = Term(tyr::formalism::ParameterIndex(3));
    ASSERT_TRUE(holds_alternative<tyr::formalism::ParameterIndex>(term));
    EXPECT_EQ(get<tyr::formalism::ParameterIndex>(term), tyr::formalism::ParameterIndex(3));
}

TEST(TyrFormalismTerm, ReadoutFollowsReferencedObjectsToTheirLayer)
{
    auto parent = tyr::formalism::Repository();
    auto object_data = urd::data<tyr::formalism::Object>(std::string("parent"));
    const auto object = parent.insert(object_data).first;
    auto child = tyr::formalism::Repository(&parent);
    object_data.name = "child";
    const auto child_object = child.insert(object_data).first;

    const auto parent_term = urd::resolve(Term(object.index()), child);
    const auto child_term = urd::resolve(Term(child_object.index()), child);
    const auto parameter = urd::resolve(Term(tyr::formalism::ParameterIndex(0)), child);
    EXPECT_EQ(&std::get<tyr::formalism::ObjectView>(parent_term).context(), &parent);
    EXPECT_EQ(&std::get<tyr::formalism::ObjectView>(child_term).context(), &child);
    EXPECT_EQ(std::get<tyr::formalism::ParameterIndex>(parameter), tyr::formalism::ParameterIndex(0));
    EXPECT_EQ(urd::as_member<tyr::formalism::Term>(child_term), Term(child_object.index()));
}
