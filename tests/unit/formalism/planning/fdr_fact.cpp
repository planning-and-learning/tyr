#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <concepts>

template<typename Entity>
concept FdrFactContract = std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::borrowed_view<Entity, tyr::formalism::planning::Repository>>
                          && requires(urd::data<Entity>& data, const urd::borrowed_view<Entity, tyr::formalism::planning::Repository>& view) {
                                 data.variable;
                                 data.value;
                                 { data == data } -> std::same_as<bool>;
                                 view.get_variable();
                                 view.get_value();
                                 view.has_value();
                                 {
                                     view.get_atom_index()
                                 } -> std::same_as<std::optional<urd::index<tyr::formalism::Atom<tyr::GroundTag, tyr::formalism::FluentTag>>>>;
                                 {
                                     view.get_atom()
                                 } -> std::same_as<std::optional<tyr::formalism::planning::AtomView<tyr::GroundTag, tyr::formalism::FluentTag>>>;
                                 { view == view } -> std::same_as<bool>;
                                 { view < view } -> std::same_as<bool>;
                             };

static_assert(FdrFactContract<tyr::formalism::planning::FDRFact<tyr::formalism::FluentTag>>);
static_assert(std::same_as<urd::borrowed_view<tyr::formalism::planning::FDRFact<tyr::formalism::FluentTag>, tyr::formalism::planning::Repository>,
                           tyr::formalism::planning::FDRFactView<tyr::formalism::FluentTag>>);
// A fact computed on the fly is held by its view; it reads and compares as a stored one.
static_assert(std::totally_ordered<urd::owning_view<tyr::formalism::planning::FDRFact<tyr::formalism::FluentTag>, tyr::formalism::planning::Repository>>);
