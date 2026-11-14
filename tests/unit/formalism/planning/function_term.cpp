#include "tyr/formalism/copy.hpp"
#include "tyr/formalism/function_term_data.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <string>

namespace tyr::tests::planning_function_term
{

namespace lifted_tests
{

template<typename Entity>
concept FunctionTermContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                               && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, tyr::formalism::planning::Repository>>
                               && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::planning::Repository>& view) {
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
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTerm<::tyr::LiftedTag, tyr::formalism::StaticTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionTermView<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(FunctionTermContract<tyr::formalism::FunctionTerm<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTerm<::tyr::LiftedTag, tyr::formalism::FluentTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionTermView<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(FunctionTermContract<tyr::formalism::FunctionTerm<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTerm<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionTermView<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>);
}

namespace ground_tests
{

template<typename Entity>
concept GroundFunctionTermContract =
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::planning::Repository>>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::planning::Repository>& view) {
           data.index;
           data.row;
           { data == data } -> std::same_as<bool>;
           view.index();
           view.get_function();
           view.get_row();
           view.get_objects();
           view.get_key();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

static_assert(GroundFunctionTermContract<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::StaticTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(GroundFunctionTermContract<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::FluentTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(GroundFunctionTermContract<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);
static_assert(std::same_as<urd::index_view<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);

TEST(TyrFormalismPlanningGroundFunctionTerm, ExposesRepositoryView)
{
    auto repository = tyr::formalism::planning::Repository();

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

    auto ground_function_term_data = urd::data<tyr::formalism::FunctionTerm<::tyr::GroundTag, tyr::formalism::FluentTag>>(binding.data());
    const auto [ground_function_term, ground_function_term_created] = ygg::formalism::insert(repository, ground_function_term_data);
    ASSERT_TRUE(ground_function_term_created);

    EXPECT_EQ(ground_function_term.get_function().index(), function.index());
    const auto objects = ground_function_term.get_objects();
    ASSERT_EQ(objects.size(), 1);
    EXPECT_EQ(objects[0].index(), object.index());
    EXPECT_EQ(fmt::format("{}", binding), "(fuel truck)");
    EXPECT_EQ(fmt::format("{}", ground_function_term), "(fuel truck)");
}

}

TEST(TyrFormalismPlanningCopy, RemapsLiftedFunctionTerms)
{
    auto source = tyr::formalism::planning::Repository();
    auto destination = tyr::formalism::planning::Repository();
    using Function = tyr::formalism::Function<tyr::formalism::FluentTag>;
    auto function_data = urd::data<Function>(std::string("fuel"), 1);
    const auto function = tyr::formalism::planning::insert(source, function_data).first;
    auto other_data = urd::data<Function>(std::string("distance"), 1);
    (void) tyr::formalism::planning::insert(destination, other_data);
    const auto target_function = tyr::formalism::planning::insert(destination, function_data).first;
    ASSERT_NE(function.index(), target_function.index());

    auto term_data = urd::data<tyr::formalism::FunctionTerm<tyr::LiftedTag, tyr::formalism::FluentTag>> {};
    term_data.function = function.index();
    term_data.terms.push_back(tyr::formalism::Term(tyr::formalism::ParameterIndex(0)));
    const auto term = tyr::formalism::planning::insert(source, term_data).first;
    auto builder = tyr::formalism::planning::Builder {};
    auto context = tyr::formalism::WriteContext { builder, destination };
    const auto [copied, inserted] = tyr::formalism::copy(term, context);
    EXPECT_TRUE(inserted);
    EXPECT_EQ(copied.get_function(), target_function);
    EXPECT_EQ(urd::to_stored(copied.get_terms()[0]), urd::to_stored(term.get_terms()[0]));
    EXPECT_FALSE(tyr::formalism::copy(term, context).second);
}

}  // namespace tyr::tests::planning_function_term
