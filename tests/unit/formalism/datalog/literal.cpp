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
concept LiteralContract = std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>>
                          && std::totally_ordered<urd::data<Entity>> && std::totally_ordered<urd::index_view<Entity, tyr::formalism::datalog::Repository>>
                          && std::same_as<urd::index_view<Entity, tyr::formalism::datalog::Repository>, typename LiteralPublicView<Entity>::type>
                          && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::datalog::Repository>& view) {
                                 data.index;
                                 data.atom;
                                 data.polarity;
                                 view.index();
                                 view.get_atom();
                                 view.get_polarity();
                             };

static_assert([]<typename... Entities>(boost::mp11::mp_list<Entities...>)
              { return (LiteralContract<Entities> && ...); }(tyr::formalism::datalog::LiteralTypes<::tyr::LiftedTag> {}));
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
    std::constructible_from<urd::index<Entity>, ygg::uint_t> && std::totally_ordered<urd::index<Entity>> && std::totally_ordered<urd::data<Entity>>
    && std::totally_ordered<urd::index_view<Entity, tyr::formalism::datalog::Repository>>
    && std::same_as<urd::index_view<Entity, tyr::formalism::datalog::Repository>, typename GroundLiteralPublicView<Entity>::type>
    && requires(urd::data<Entity>& data, const urd::index_view<Entity, tyr::formalism::datalog::Repository>& view) {
           data.index;
           data.atom;
           data.polarity;
           view.index();
           view.get_atom();
           view.get_polarity();
       };

static_assert([]<typename... Entities>(boost::mp11::mp_list<Entities...>)
              { return (GroundLiteralContract<Entities> && ...); }(tyr::formalism::datalog::LiteralTypes<::tyr::GroundTag> {}));
}
