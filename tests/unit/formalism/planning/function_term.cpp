#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/parameters_data.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/variable_data.hpp"

#include <concepts>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <string>
#include <urd/elements/copy.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/copy.hpp>
#include <yggdrasil/formalism/declarations.hpp>
#include <yggdrasil/formalism/row_data.hpp>
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
                                      view.function();
                                      view.terms();
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
                                            view.function();
                                            view.row();
                                            view.row().template resolve<tyr::formalism::Object>();
                                            view.row().relation().index();
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
    const auto [function, function_created] = repository.insert(function_data);
    ASSERT_TRUE(function_created);

    auto object_data = urd::data<tyr::formalism::Object>(std::string("truck"));
    const auto [object, object_created] = repository.insert(object_data);
    ASSERT_TRUE(object_created);

    auto binding_data = urd::data<ygg::Row<tyr::formalism::Function<tyr::formalism::FluentTag>>> {};
    binding_data.relation = function.index();
    binding_data.values.push_back(object.index().value);
    const auto [binding, binding_created] = repository.insert(binding_data);
    ASSERT_TRUE(binding_created);

    auto ground_function_term_data = urd::data<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::FluentTag>>(binding.index());
    const auto [ground_function_term, ground_function_term_created] = repository.insert(ground_function_term_data);
    ASSERT_TRUE(ground_function_term_created);

    EXPECT_EQ(ground_function_term.function().index(), function.index());
    const auto objects = ground_function_term.row().template resolve<tyr::formalism::Object>();
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
    const auto function = source.insert(function_data).first;
    auto other_data = urd::data<Function>(std::string("distance"), 1);
    (void) destination.insert(other_data);
    const auto target_function = destination.insert(function_data).first;
    ASSERT_NE(function.index(), target_function.index());

    auto variable_data = urd::data<tyr::formalism::Variable>(urd::flat_string(std::string("?x")));
    auto parameters_data = urd::data<tyr::formalism::Parameters> {};
    parameters_data.variables.push_back(source.insert(variable_data).first.index());
    auto term_data = urd::data<tyr::formalism::FunctionTerm<tyr::LiftedTag, tyr::formalism::FluentTag>> {};
    term_data.parameters = source.insert(parameters_data).first.index();
    term_data.function = function.index();
    term_data.terms.push_back(tyr::formalism::Term(tyr::formalism::ParameterIndex(0)));
    const auto term = source.insert(term_data).first;
    auto builder = tyr::formalism::Builder {};
    const auto copied = destination.view(urd::copy(term, destination, builder));
    EXPECT_EQ(copied.function(), target_function);
    EXPECT_EQ(urd::as_member<tyr::formalism::Term>(copied.terms()[0]), urd::as_member<tyr::formalism::Term>(term.terms()[0]));
    EXPECT_EQ(urd::copy(term, destination, builder), copied.index());  // interned once
}

}  // namespace tyr::tests::planning_function_term
