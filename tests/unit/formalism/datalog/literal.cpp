#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/literal_data.hpp"
#include "tyr/formalism/datalog/literal_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>

namespace lifted_tests
{

template<typename Entity>
struct LiteralPublicView;

template<tyr::formalism::FactKind T>
struct LiteralPublicView<tyr::formalism::datalog::Literal<::tyr::LiftedTag, T>>
{
    using type = tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, T>;
};

template<typename Entity>
concept LiteralContract = std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>>
                          && std::totally_ordered<ygg::Data<Entity>> && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>>
                          && std::same_as<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>, typename LiteralPublicView<Entity>::type>
                          && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>& view) {
                                 data.index;
                                 data.atom;
                                 data.polarity;
                                 data.clear();
                                 view.get_index();
                                 view.get_atom();
                                 view.get_polarity();
                             };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>)
              { return (LiteralContract<Entities> && ...); }(tyr::formalism::datalog::LiteralTypes<::tyr::LiftedTag> {}));
static_assert(std::constructible_from<ygg::Data<tyr::formalism::datalog::Literal<::tyr::LiftedTag, tyr::formalism::StaticTag>>,
                                      tyr::formalism::datalog::AtomView<::tyr::LiftedTag, tyr::formalism::StaticTag>,
                                      bool>);
}

namespace ground_tests
{

template<typename Entity>
struct GroundLiteralPublicView;

template<tyr::formalism::FactKind T>
struct GroundLiteralPublicView<tyr::formalism::datalog::Literal<::tyr::GroundTag, T>>
{
    using type = tyr::formalism::datalog::LiteralView<::tyr::GroundTag, T>;
};

template<typename Entity>
concept GroundLiteralContract =
    std::constructible_from<ygg::Index<Entity>, ygg::uint_t> && std::totally_ordered<ygg::Index<Entity>> && std::totally_ordered<ygg::Data<Entity>>
    && std::totally_ordered<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>>
    && std::same_as<ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>, typename GroundLiteralPublicView<Entity>::type>
    && requires(ygg::Data<Entity>& data, const ygg::View<ygg::Index<Entity>, tyr::formalism::datalog::Repository>& view) {
           data.index;
           data.atom;
           data.polarity;
           data.clear();
           view.get_index();
           view.get_atom();
           view.get_polarity();
       };

static_assert([]<typename... Entities>(ygg::TypeList<Entities...>)
              { return (GroundLiteralContract<Entities> && ...); }(tyr::formalism::datalog::LiteralTypes<::tyr::GroundTag> {}));
static_assert(std::constructible_from<ygg::Data<tyr::formalism::datalog::Literal<::tyr::GroundTag, tyr::formalism::StaticTag>>,
                                      tyr::formalism::datalog::AtomView<::tyr::GroundTag, tyr::formalism::StaticTag>,
                                      bool>);
}
