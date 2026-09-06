#include "tyr/formalism/planning/function_term_data.hpp"
#include "tyr/formalism/planning/function_term_index.hpp"
#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include <concepts>
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include <gtest/gtest.h>
#include <string>

namespace lifted_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept FunctionTermContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                               && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                               && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
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

static_assert(FunctionTermContract<fp::FunctionTerm<::tyr::LiftedTag, f::StaticTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::FunctionTerm<::tyr::LiftedTag, f::StaticTag>>, fp::Repository>, fp::FunctionTermView<::tyr::LiftedTag, f::StaticTag>>);
static_assert(FunctionTermContract<fp::FunctionTerm<::tyr::LiftedTag, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::FunctionTerm<::tyr::LiftedTag, f::FluentTag>>, fp::Repository>, fp::FunctionTermView<::tyr::LiftedTag, f::FluentTag>>);
static_assert(FunctionTermContract<fp::FunctionTerm<::tyr::LiftedTag, f::AuxiliaryTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::FunctionTerm<::tyr::LiftedTag, f::AuxiliaryTag>>, fp::Repository>, fp::FunctionTermView<::tyr::LiftedTag, f::AuxiliaryTag>>);

}

namespace ground_tests
{

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

template<typename Entity>
concept GroundFunctionTermContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                                     && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, fp::Repository>>
                                     && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, fp::Repository>& view) {
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

static_assert(GroundFunctionTermContract<fp::FunctionTerm<::tyr::GroundTag, f::StaticTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::FunctionTerm<::tyr::GroundTag, f::StaticTag>>, fp::Repository>, fp::FunctionTermView<::tyr::GroundTag, f::StaticTag>>);
static_assert(GroundFunctionTermContract<fp::FunctionTerm<::tyr::GroundTag, f::FluentTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::FunctionTerm<::tyr::GroundTag, f::FluentTag>>, fp::Repository>, fp::FunctionTermView<::tyr::GroundTag, f::FluentTag>>);
static_assert(GroundFunctionTermContract<fp::FunctionTerm<::tyr::GroundTag, f::AuxiliaryTag>>);
static_assert(std::same_as<ygg::View<ygg::Index<fp::FunctionTerm<::tyr::GroundTag, f::AuxiliaryTag>>, fp::Repository>, fp::FunctionTermView<::tyr::GroundTag, f::AuxiliaryTag>>);

TEST(TyrFormalismPlanningGroundFunctionTerm, ExposesRepositoryView)
{
    auto repository = fp::RepositoryFactory().create();

    auto function_data = ygg::Data<f::Function<f::FluentTag>>(std::string("fuel"), 1);
    canonicalize(function_data);
    const auto [function, function_created] = repository.get_or_create(function_data);
    ASSERT_TRUE(function_created);

    auto object_data = ygg::Data<f::Object>(std::string("truck"));
    canonicalize(object_data);
    const auto [object, object_created] = repository.get_or_create(object_data);
    ASSERT_TRUE(object_created);

    auto binding_data = ygg::Data<f::RelationBinding<f::Function<f::FluentTag>>> {};
    binding_data.relation = function.get_index();
    binding_data.objects.push_back(object.get_index());
    canonicalize(binding_data);
    const auto [binding, binding_created] = repository.get_or_create(binding_data);
    ASSERT_TRUE(binding_created);

    auto ground_function_term_data = ygg::Data<fp::FunctionTerm<::tyr::GroundTag, f::FluentTag>>(binding.get_index());
    canonicalize(ground_function_term_data);
    const auto [ground_function_term, ground_function_term_created] = repository.get_or_create(ground_function_term_data);
    ASSERT_TRUE(ground_function_term_created);

    EXPECT_EQ(ground_function_term.get_function().get_index(), function.get_index());
    const auto objects = ground_function_term.get_objects();
    ASSERT_EQ(objects.size(), 1);
    EXPECT_EQ(objects[0].get_index(), object.get_index());
    EXPECT_EQ(fmt::format("{}", binding), "(fuel truck)");
    EXPECT_EQ(fmt::format("{}", ground_function_term), "(fuel truck)");
}

}
