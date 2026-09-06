#ifndef TYR_SERIALIZATION_TYPES_HPP_
#define TYR_SERIALIZATION_TYPES_HPP_

#include "tyr/serialization/formalism/planning/planning.hpp"

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

}

#endif
