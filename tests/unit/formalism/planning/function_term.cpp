#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/copy.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/function_term_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>
#include <gtest/gtest.h>
#include <string>

namespace lifted_tests
{

template<typename Entity>
concept FunctionTermContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>& view) {
           data.index;
           data.function;
           data.terms;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_index();
           view.get_function();
           view.get_terms();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

static_assert(FunctionTermContract<tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(std::same_as<
              ygg::View<ygg::Index<tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, tyr::formalism::StaticTag>>, tyr::formalism::planning::Repository>,
              tyr::formalism::planning::FunctionTermView<::tyr::LiftedTag, tyr::formalism::StaticTag>>);
static_assert(FunctionTermContract<tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<
              ygg::View<ygg::Index<tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, tyr::formalism::FluentTag>>, tyr::formalism::planning::Repository>,
              tyr::formalism::planning::FunctionTermView<::tyr::LiftedTag, tyr::formalism::FluentTag>>);
static_assert(FunctionTermContract<tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>,
                                     tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionTermView<::tyr::LiftedTag, tyr::formalism::AuxiliaryTag>>);
}

namespace ground_tests
{

template<typename Entity>
concept GroundFunctionTermContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::planning::Repository>& view) {
           data.index;
           data.binding;
           data.clear();
           { data == data } -> std::same_as<bool>;
           view.get_index();
           view.get_function();
           view.get_row();
           view.get_objects();
           view.get_key();
           { view == view } -> std::same_as<bool>;
           { view < view } -> std::same_as<bool>;
       };

static_assert(GroundFunctionTermContract<tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(std::same_as<
              ygg::View<ygg::Index<tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, tyr::formalism::StaticTag>>, tyr::formalism::planning::Repository>,
              tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, tyr::formalism::StaticTag>>);
static_assert(GroundFunctionTermContract<tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(std::same_as<
              ygg::View<ygg::Index<tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, tyr::formalism::FluentTag>>, tyr::formalism::planning::Repository>,
              tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, tyr::formalism::FluentTag>>);
static_assert(GroundFunctionTermContract<tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>,
                                     tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FunctionTermView<::tyr::GroundTag, tyr::formalism::AuxiliaryTag>>);

TEST(TyrFormalismPlanningGroundFunctionTerm, ExposesRepositoryView)
{
    auto repository = tyr::formalism::planning::RepositoryFactory().create();

    auto function_data = ygg::Data<tyr::formalism::Function<tyr::formalism::FluentTag>>(std::string("fuel"), 1);
    canonicalize(function_data);
    const auto [function, function_created] = repository.insert(function_data);
    ASSERT_TRUE(function_created);

    auto object_data = ygg::Data<tyr::formalism::Object>(std::string("truck"));
    canonicalize(object_data);
    const auto [object, object_created] = repository.insert(object_data);
    ASSERT_TRUE(object_created);

    auto binding_data = ygg::Data<tyr::formalism::RelationBinding<tyr::formalism::Function<tyr::formalism::FluentTag>>> {};
    binding_data.relation = function.get_index();
    binding_data.objects.push_back(object.get_index());
    canonicalize(binding_data);
    const auto [binding, binding_created] = repository.insert(binding_data);
    ASSERT_TRUE(binding_created);

    auto ground_function_term_data = ygg::Data<tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, tyr::formalism::FluentTag>>(binding.get_index());
    canonicalize(ground_function_term_data);
    const auto [ground_function_term, ground_function_term_created] = repository.insert(ground_function_term_data);
    ASSERT_TRUE(ground_function_term_created);

    EXPECT_EQ(ground_function_term.get_function().get_index(), function.get_index());
    const auto objects = ground_function_term.get_objects();
    ASSERT_EQ(objects.size(), 1);
    EXPECT_EQ(objects[0].get_index(), object.get_index());
    EXPECT_EQ(fmt::format("{}", binding), "(fuel truck)");
    EXPECT_EQ(fmt::format("{}", ground_function_term), "(fuel truck)");
}

}

TEST(TyrFormalismPlanningCopy, RemapsLiftedFunctionTerms)
{
    auto factory = tyr::formalism::planning::RepositoryFactory {};
    auto source = factory.create();
    auto destination = factory.create();
    using Function = tyr::formalism::Function<tyr::formalism::FluentTag>;
    auto function_data = ygg::Data<Function>(std::string("fuel"), 1);
    const auto function = tyr::formalism::planning::insert(source, function_data).first;
    auto other_data = ygg::Data<Function>(std::string("distance"), 1);
    (void) tyr::formalism::planning::insert(destination, other_data);
    const auto target_function = tyr::formalism::planning::insert(destination, function_data).first;
    ASSERT_NE(function.get_index(), target_function.get_index());

    auto term_data = ygg::Data<tyr::formalism::planning::FunctionTerm<tyr::LiftedTag, tyr::formalism::FluentTag>> {};
    term_data.function = function.get_index();
    term_data.terms.push_back(ygg::Data<tyr::formalism::Term>(tyr::formalism::ParameterIndex(0)));
    const auto term = tyr::formalism::planning::insert(source, term_data).first;
    auto builder = tyr::formalism::planning::Builder {};
    auto context = tyr::formalism::planning::CopyContext { builder, destination };
    const auto [copied, inserted] = tyr::formalism::planning::copy(term, context);
    EXPECT_TRUE(inserted);
    EXPECT_EQ(copied.get_function(), target_function);
    EXPECT_EQ(copied.get_terms()[0].get_data(), term.get_terms()[0].get_data());
    EXPECT_FALSE(tyr::formalism::planning::copy(term, context).second);
}
