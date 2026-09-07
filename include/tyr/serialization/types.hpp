#ifndef TYR_SERIALIZATION_TYPES_HPP_
#define TYR_SERIALIZATION_TYPES_HPP_

#include "tyr/serialization/serialization.hpp"

#include <type_traits>
#include <yggdrasil/core/concepts.hpp>
#include <yggdrasil/core/type_list.hpp>

namespace tyr::serialization
{

template<typename T>
using FormalismIndexView = ygg::View<ygg::Index<T>, formalism::planning::Repository>;

using FormalismValueViews = ygg::TypeList<formalism::planning::TermView,
                                          formalism::planning::FunctionExpressionView<LiftedTag>,
                                          formalism::planning::FunctionExpressionView<GroundTag>,
                                          formalism::planning::ArithmeticOperatorView<LiftedTag>,
                                          formalism::planning::ArithmeticOperatorView<GroundTag>,
                                          formalism::planning::BooleanOperatorView<LiftedTag>,
                                          formalism::planning::BooleanOperatorView<GroundTag>,
                                          formalism::planning::NumericEffectOperatorView<LiftedTag, formalism::FluentTag>,
                                          formalism::planning::NumericEffectOperatorView<LiftedTag, formalism::AuxiliaryTag>,
                                          formalism::planning::NumericEffectOperatorView<GroundTag, formalism::FluentTag>,
                                          formalism::planning::NumericEffectOperatorView<GroundTag, formalism::AuxiliaryTag>,
                                          formalism::planning::FDRFactView<formalism::FluentTag>>;

using FormalismViews = ygg::ConcatTypeListsT<ygg::MapTypeListT<FormalismIndexView, formalism::planning::BuilderTypes>, FormalismValueViews>;

using FormalismOwners = ygg::TypeList<formalism::planning::PlanningDomain, formalism::planning::PlanningTask, formalism::planning::PlanningFDRTask>;

using RuntimeStates = ygg::TypeList<planning::StateView<GroundTag>, planning::StateView<LiftedTag>>;
using RuntimeOwners = ygg::TypeList<planning::Task<GroundTag>,
                                   planning::Task<LiftedTag>,
                                   planning::Node<GroundTag>,
                                   planning::Node<LiftedTag>,
                                   planning::LabeledNode<GroundTag>,
                                   planning::LabeledNode<LiftedTag>,
                                   planning::Plan<GroundTag>,
                                   planning::Plan<LiftedTag>>;
using SerializedTypes = ygg::ConcatTypeListsT<FormalismViews, RuntimeStates, FormalismOwners, RuntimeOwners>;

template<typename T>
using HashableTypeList = std::conditional_t<ygg::Hashable<T>, ygg::TypeList<T>, ygg::TypeList<>>;

using RegisteredTypes = ygg::ApplyTypeListT<ygg::ConcatTypeListsT, ygg::MapTypeListT<HashableTypeList, SerializedTypes>>;

using ProjectionTypes = ygg::ConcatTypeListsT<SerializedTypes,
                                             ygg::TypeList<formalism::BooleanOperatorKind,
                                                           formalism::ArithmeticOperatorKind,
                                                           formalism::NumericEffectOperatorKind,
                                                           formalism::OptimizationDirection>>;

}

#endif
