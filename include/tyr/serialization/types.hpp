#ifndef TYR_SERIALIZATION_TYPES_HPP_
#define TYR_SERIALIZATION_TYPES_HPP_

#include "tyr/serialization/formalism/planning/planning.hpp"

#include <yggdrasil/core/type_list.hpp>

namespace tyr::serialization
{

template<typename T>
using FormalismIndexView = ygg::View<ygg::Index<T>, ::tyr::formalism::planning::Repository>;

using FormalismValueViews = ygg::TypeList<::tyr::formalism::planning::TermView,
                                          ::tyr::formalism::planning::FunctionExpressionView<::tyr::LiftedTag>,
                                          ::tyr::formalism::planning::FunctionExpressionView<::tyr::GroundTag>,
                                          ::tyr::formalism::planning::LiftedArithmeticOperatorView,
                                          ::tyr::formalism::planning::GroundArithmeticOperatorView,
                                          ::tyr::formalism::planning::LiftedBooleanOperatorView,
                                          ::tyr::formalism::planning::GroundBooleanOperatorView,
                                          ::tyr::formalism::planning::NumericEffectOperatorView<::tyr::LiftedTag, ::tyr::formalism::FluentTag>,
                                          ::tyr::formalism::planning::NumericEffectOperatorView<::tyr::LiftedTag, ::tyr::formalism::AuxiliaryTag>,
                                          ::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::FluentTag>,
                                          ::tyr::formalism::planning::NumericEffectOperatorView<::tyr::GroundTag, ::tyr::formalism::AuxiliaryTag>,
                                          ::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag>>;

using FormalismViews = ygg::ConcatTypeListsT<ygg::MapTypeListT<FormalismIndexView, ::tyr::formalism::planning::BuilderTypes>, FormalismValueViews>;

using FormalismOwners =
    ygg::TypeList<::tyr::formalism::planning::PlanningDomain, ::tyr::formalism::planning::PlanningTask, ::tyr::formalism::planning::PlanningFDRTask>;

}

#endif
