#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/parameters_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/variable_data.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <string>
#include <urd/elements/copy.hpp>
#include <yggdrasil/formalism/copy.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::tests::planning_function_term
{

namespace lifted_tests
{

template<typename Entity>
concept FunctionTermContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                               && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, tyr::formalism::Repository>>
                               && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::Repository>& view) {
                                      data.index;
                                      data.function;
                                      data.terms;
                                      { data == data } -> std::same_as<bool>;
                                      view.index();
                                      view.get_function();
                                      view.get_terms();
                                      { view == view } -> std::same_as<bool>;
                                      { view < view } -> std::same_as<bool>;
                                  };

static_assert(FunctionTermContract<tyr::formalism::FunctionTerm<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTerm<::tyr::LiftedTag, tyr::formalism::StaticTag>, tyr::formalism::Repository>,
                           tyr::formalism::FunctionTermView<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(FunctionTermContract<tyr::formalism::FunctionTerm<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTerm<::tyr::LiftedTag, tyr::formalism::FluentTag>, tyr::formalism::Repository>,
                           tyr::formalism::FunctionTermView<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(FunctionTermContract<tyr::formalism::FunctionTerm<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTerm<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>, tyr::formalism::Repository>,
                           tyr::formalism::FunctionTermView<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>);
}

namespace ground_tests
{

template<typename Entity>
concept GroundFunctionTermContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                                     && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, tyr::formalism::Repository>>
                                     && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::Repository>& view) {
                                            data.index;
                                            data.row;
                                            { data == data } -> std::same_as<bool>;
                                            view.index();
                                            view.get_function();
                                            view.get_row();
                                            view.get_row().template resolve<tyr::formalism::Object>();
                                            view.get_row().get_relation().index();
                                            { view == view } -> std::same_as<bool>;
                                            { view < view } -> std::same_as<bool>;
                                        };

static_assert(GroundFunctionTermContract<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::StaticTag>, tyr::formalism::Repository>,
                           tyr::formalism::FunctionTermView<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(GroundFunctionTermContract<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::FluentTag>, tyr::formalism::Repository>,
                           tyr::formalism::FunctionTermView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(GroundFunctionTermContract<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>, tyr::formalism::Repository>,
                           tyr::formalism::FunctionTermView<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);

TEST(TyrFormalismPlanningGroundFunctionTerm, ExposesRepositoryView)
{
    auto repository = tyr::formalism::Repository();

    auto function_data = urd::data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("fuel"), 1);
    const auto [function, function_created] = ygg::formalism::insert(repository, function_data);
    ASSERT_TRUE(function_created);

    auto object_data = urd::data<tyr::formalism::Object>(std::string("truck"));
    const auto [object, object_created] = ygg::formalism::insert(repository, object_data);
    ASSERT_TRUE(object_created);

    auto binding_data = urd::data<ygg::Row<tyr::formalism::Function<tyr::formalism::FluentTag>>> {};
    binding_data.relation = function.index();
    binding_data.values.push_back(object.index().value);
    const auto [binding, binding_created] = ygg::formalism::insert(repository, binding_data);
    ASSERT_TRUE(binding_created);

    auto ground_function_term_data = urd::data<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::FluentTag>>(binding.index());
    const auto [ground_function_term, ground_function_term_created] = ygg::formalism::insert(repository, ground_function_term_data);
    ASSERT_TRUE(ground_function_term_created);

    EXPECT_EQ(ground_function_term.get_function().index(), function.index());
    const auto objects = ground_function_term.get_row().template resolve<tyr::formalism::Object>();
    ASSERT_EQ(objects.size(), 1);
    EXPECT_EQ(objects[0].index(), object.index());
    EXPECT_EQ(fmt::format("{}", binding), "(fuel truck)");
    EXPECT_EQ(fmt::format("{}", ground_function_term), "(fuel truck)");
}

}

TEST(TyrFormalismPlanningCopy, RemapsLiftedFunctionTerms)
{
    auto source = tyr::formalism::Repository();
    auto destination = tyr::formalism::Repository();
    using Function = tyr::formalism::Function<tyr::formalism::FluentTag>;
    auto function_data = urd::data<Function>(std::string("fuel"), 1);
    const auto function = tyr::formalism::insert(source, function_data).first;
    auto other_data = urd::data<Function>(std::string("distance"), 1);
    (void) tyr::formalism::insert(destination, other_data);
    const auto target_function = tyr::formalism::insert(destination, function_data).first;
    ASSERT_NE(function.index(), target_function.index());

    auto variable_data = urd::data<tyr::formalism::Variable>(urd::flat_string(std::string("?x")));
    auto parameters_data = urd::data<tyr::formalism::Parameters> {};
    parameters_data.variables.push_back(tyr::formalism::insert(source, variable_data).first.index());
    auto term_data = urd::data<tyr::formalism::FunctionTerm<tyr::LiftedTag, tyr::formalism::FluentTag>> {};
    term_data.parameters = tyr::formalism::insert(source, parameters_data).first.index();
    term_data.function = function.index();
    term_data.terms.push_back(tyr::formalism::Term(tyr::formalism::ParameterIndex(0)));
    const auto term = tyr::formalism::insert(source, term_data).first;
    auto builder = tyr::formalism::Builder {};
    const auto copied = destination.view(urd::copy(term, destination, builder));
    EXPECT_EQ(copied.get_function(), target_function);
    EXPECT_EQ(urd::stored<tyr::formalism::Term>(copied.get_terms()[0]), urd::stored<tyr::formalism::Term>(term.get_terms()[0]));
    EXPECT_EQ(urd::copy(term, destination, builder), copied.index());  // interned once
}

}  // namespace tyr::tests::planning_function_term
