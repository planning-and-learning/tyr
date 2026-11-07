#ifndef TYR_SERIALIZATION_TYPES_HPP_
#define TYR_SERIALIZATION_TYPES_HPP_

#include "tyr/serialization/serialization.hpp"

#include <type_traits>
#include <boost/mp11.hpp>

namespace tyr::serialization
{

template<typename T>
using FormalismIndexView = urd::index_view<T, formalism::planning::Repository>;
template<typename R>
using FormalismBindingView = ::ygg::formalism::RelationBindingView<ygg::formalism::RelationBindingIndex<R, formalism::ObjectTag>, formalism::planning::Repository>;

// Terms and function expressions are not among them: std::variants (of views and values), serialized where their
// holders hold them.
using FormalismValueViews = boost::mp11::mp_list<formalism::planning::FDRFactView<formalism::FluentTag>>;

using FormalismViews = boost::mp11::mp_append<boost::mp11::mp_transform<FormalismIndexView, formalism::planning::SymbolRepositoryTypes>,
                                             boost::mp11::mp_transform<FormalismBindingView, formalism::planning::RelationRepositoryTypes>,
                                             FormalismValueViews>;

using FormalismOwners =
    boost::mp11::mp_list<formalism::planning::PlanningDomain, formalism::planning::PlanningTask<LiftedTag>, formalism::planning::PlanningTask<GroundTag>>;

using RuntimeStates = boost::mp11::mp_list<planning::StateView<GroundTag>, planning::StateView<LiftedTag>>;
using RuntimeOwners = boost::mp11::mp_list<planning::Task<GroundTag>,
                                    planning::Task<LiftedTag>,
                                    planning::Node<GroundTag>,
                                    planning::Node<LiftedTag>,
                                    planning::LabeledNode<GroundTag>,
                                    planning::LabeledNode<LiftedTag>,
                                    planning::Plan<GroundTag>,
                                    planning::Plan<LiftedTag>>;
using SerializedTypes = boost::mp11::mp_append<FormalismViews, RuntimeStates, FormalismOwners, RuntimeOwners>;

template<typename T>
using HashableTypeList = std::conditional_t<urd::Hashable<T>, boost::mp11::mp_list<T>, boost::mp11::mp_list<>>;

using RegisteredTypes = boost::mp11::mp_apply<boost::mp11::mp_append, boost::mp11::mp_transform<HashableTypeList, SerializedTypes>>;

using ProjectionTypes = boost::mp11::mp_append<
    SerializedTypes,
    boost::mp11::mp_list<formalism::BooleanOperatorKind, formalism::ArithmeticOperatorKind, formalism::NumericEffectOperatorKind, formalism::OptimizationDirection>>;

}

#endif
