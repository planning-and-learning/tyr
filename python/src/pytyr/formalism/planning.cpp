/*
 * Copyright (C) 2025-2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "planning/datas.hpp"
#include "planning/domains.hpp"
#include "planning/indices.hpp"
#include "planning/invariants.hpp"
#include "planning/module.hpp"
#include "planning/mutable.hpp"
#include "planning/views.hpp"

#include <nanobind/stl/chrono.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <tyr/tyr.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::formalism::planning
{
using ygg::bind_fixed_uint;

namespace
{
template<typename T>
auto bind_get_or_create_canonical()
{
    return [](Repository& self, ygg::Data<T>& data)
    {
        canonicalize(data);
        return self.template get_or_create<T>(data).first;
    };
}

template<typename T>
auto bind_get_or_create_relation()
{
    return [](Repository& self, const ygg::Data<RelationBinding<T>>& data) { return self.template get_or_create<T>(data).first; };
}

template<typename T>
auto bind_create()
{
    return [](Repository& self, const ygg::Data<T>& data) { return make_view(data, self); };
}
}

/**
 * bind_module_definitions
 */

void bind_module_definitions(nb::module_& m)
{
    {
        nb::class_<Parser>(m, "Parser")
            .def(nb::init<const fs::path&, const loki::ParserOptions&>(), "domain_filepath"_a, "parser_options"_a)
            .def(nb::init<const std::string&, const fs::path&, const loki::ParserOptions&>(), "domain_description"_a, "domain_filepath"_a, "parser_options"_a)
            .def("parse_task", nb::overload_cast<const fs::path&, const loki::ParserOptions&>(&Parser::parse_task), "task_filepath"_a, "parser_options"_a)
            .def("parse_task",
                 nb::overload_cast<const std::string&, const fs::path&, const loki::ParserOptions&>(&Parser::parse_task),
                 "task_description"_a,
                 "task_filepath"_a,
                 "parser_options"_a)
            .def("get_domain", &Parser::get_domain);
    }

    auto minimize_cls = nb::class_<Minimize>(m, "Minimize")
                            .def(nb::init<>())
                            .def("__str__", [](const Minimize& self) { return ygg::to_string(self); })
                            .def("__repr__", [](const Minimize& self) { return ygg::to_string(self); });
    ygg::add_comparison(minimize_cls);

    auto maximize_cls = nb::class_<Maximize>(m, "Maximize")
                            .def(nb::init<>())
                            .def("__str__", [](const Maximize& self) { return ygg::to_string(self); })
                            .def("__repr__", [](const Maximize& self) { return ygg::to_string(self); });
    ygg::add_comparison(maximize_cls);

    /**
     * Common
     */

    bind_fixed_uint<FDRValue>(m, "FDRValue");

    /**
     * Invariants
     */

    invariant::bind_invariants(m);

    /**
     * Mutable
     */

    bind_mutable(m);

    /**
     * Domains
     */

    bind_variable_domains(m);

    /**
     * ygg::Index
     */

    bind_indices(m);

    /**
     * ygg::Data
     */

    bind_datas(m);

    /**
     * Views
     */

    bind_views(m);

    /**
     * RepositoryFactory
     */

    nb::class_<RepositoryFactory>(m, "RepositoryFactory")  //
        .def(nb::new_([]() { return std::make_shared<RepositoryFactory>(); }))
        .def("create_repository", &RepositoryFactory::create_shared, "parent_repository"_a = nullptr, nb::keep_alive<0, 2>());

    /**
     * Repository
     */

    {
        auto cls =
            nb::class_<Repository>(m, "Repository")  //
                .def("create", bind_create<Term>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
                .def("create", bind_create<FunctionExpression>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
                .def("create", bind_create<GroundFunctionExpression>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
                .def("create", bind_create<BooleanOperator<ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
                .def("create", bind_create<BooleanOperator<ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
                .def("create", bind_create<ArithmeticOperator<ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
                .def("create", bind_create<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
                .def("create", bind_create<NumericEffectOperator<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
                .def("create", bind_create<NumericEffectOperator<AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
                .def("create", bind_create<GroundNumericEffectOperator<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
                .def("create", bind_create<GroundNumericEffectOperator<AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())

                .def("get_or_create", bind_get_or_create_canonical<Object>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Variable>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_relation<Predicate<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_relation<Predicate<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_relation<Predicate<DerivedTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_relation<Function<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_relation<Function<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_relation<Function<AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_relation<Action>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_relation<Axiom>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Predicate<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Predicate<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Predicate<DerivedTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Atom<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Atom<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Atom<DerivedTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundAtom<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundAtom<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundAtom<DerivedTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Literal<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Literal<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Literal<DerivedTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundLiteral<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundLiteral<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundLiteral<DerivedTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<FDRVariable<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Function<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Function<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Function<AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<FunctionTerm<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<FunctionTerm<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<FunctionTerm<AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>())

                .def("get_or_create", bind_get_or_create_canonical<GroundFunctionTerm<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundFunctionTerm<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundFunctionTerm<AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundFunctionTermValue<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundFunctionTermValue<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundFunctionTermValue<AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>())

                .def("get_or_create", bind_get_or_create_canonical<UnaryOperator<Sub, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Add, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Sub, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Mul, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Div, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Eq, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Ne, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Ge, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Gt, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Le, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Lt, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<MultiOperator<Add, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<MultiOperator<Mul, ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())

                .def("get_or_create", bind_get_or_create_canonical<NumericEffect<Assign, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<NumericEffect<Increase, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<NumericEffect<Decrease, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<NumericEffect<ScaleUp, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<NumericEffect<ScaleDown, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<NumericEffect<Increase, AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>())

                .def("get_or_create", bind_get_or_create_canonical<ConjunctiveCondition>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<ConjunctiveEffect>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<ConditionalEffect>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Action>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Axiom>(), "data"_a, nb::keep_alive<0, 1>())

                .def("get_or_create", bind_get_or_create_canonical<UnaryOperator<Sub, ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create",
                     bind_get_or_create_canonical<BinaryOperator<Add, ygg::Data<GroundFunctionExpression>>>(),
                     "data"_a,
                     nb::keep_alive<0, 1>())
                .def("get_or_create",
                     bind_get_or_create_canonical<BinaryOperator<Sub, ygg::Data<GroundFunctionExpression>>>(),
                     "data"_a,
                     nb::keep_alive<0, 1>())
                .def("get_or_create",
                     bind_get_or_create_canonical<BinaryOperator<Mul, ygg::Data<GroundFunctionExpression>>>(),
                     "data"_a,
                     nb::keep_alive<0, 1>())
                .def("get_or_create",
                     bind_get_or_create_canonical<BinaryOperator<Div, ygg::Data<GroundFunctionExpression>>>(),
                     "data"_a,
                     nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Eq, ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Ne, ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Ge, ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Gt, ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Le, ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<BinaryOperator<Lt, ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<MultiOperator<Add, ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<MultiOperator<Mul, ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>())

                .def("get_or_create", bind_get_or_create_canonical<GroundNumericEffect<Assign, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundNumericEffect<Increase, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundNumericEffect<Decrease, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundNumericEffect<ScaleUp, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundNumericEffect<ScaleDown, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundNumericEffect<Increase, AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>())

                .def("get_or_create", bind_get_or_create_canonical<GroundConjunctiveCondition>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundConjunctiveEffect>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundConditionalEffect>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundAction>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<GroundAxiom>(), "data"_a, nb::keep_alive<0, 1>())

                .def("get_or_create", bind_get_or_create_canonical<Metric>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Domain>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<Task>(), "data"_a, nb::keep_alive<0, 1>())
                .def("get_or_create", bind_get_or_create_canonical<FDRTask>(), "data"_a, nb::keep_alive<0, 1>());
    }

    /**
     * FDRContext
     */

    nb::class_<FDRContext>(m, "FDRContext")  //
        .def(nb::new_([](RepositoryPtr repository) { return std::make_shared<FDRContext>(std::move(repository)); }), "repository"_a)
        .def(nb::new_([](const std::vector<std::vector<GroundAtomView<FluentTag>>>& ground_mutex_groups, RepositoryPtr repository)
                      { return std::make_shared<FDRContext>(ground_mutex_groups, std::move(repository)); }),
             "ground_mutex_groups"_a,
             "repository"_a)
        .def("get_fact", nb::overload_cast<GroundAtomView<FluentTag>>(&FDRContext::get_fact), "atom"_a, nb::keep_alive<0, 1>())
        .def("get_variables", &FDRContext::get_variables);

    /**
     * PlanningDomain
     */

    {
        nb::class_<PlanningDomain>(m, "PlanningDomain")  //
            .def(nb::init<DomainView, RepositoryPtr, RepositoryFactoryPtr>(), "domain"_a, "repository"_a, "repository_factory"_a)
            .def("get_domain", &PlanningDomain::get_domain, nb::keep_alive<0, 1>())
            .def("get_repository", &PlanningDomain::get_repository)
            .def("get_repository_factory", &PlanningDomain::get_repository_factory);
    }

    {
        nb::class_<PlanningTask>(m, "PlanningTask")  //
            .def(nb::new_([](TaskView task, FDRContextPtr fdr_context, RepositoryPtr repository, PlanningDomain planning_domain)
                          { return PlanningTask(task, std::move(fdr_context), std::move(repository), std::move(planning_domain)); }),
                 "task"_a,
                 "fdr_context"_a,
                 "repository"_a,
                 "planning_domain"_a)
            .def("get_task", &PlanningTask::get_task, nb::keep_alive<0, 1>())
            .def("get_repository", &PlanningTask::get_repository)
            .def("get_fdr_context", &PlanningTask::get_fdr_context, nb::rv_policy::reference_internal)
            .def("get_domain", &PlanningTask::get_domain, nb::rv_policy::reference_internal)
            .def("get_variable_domains", &PlanningTask::get_variable_domains_view, nb::rv_policy::reference_internal);
    }

    {
        nb::class_<PlanningFDRTask>(m, "PlanningFDRTask")  //
            .def("get_task", &PlanningFDRTask::get_task, nb::keep_alive<0, 1>())
            .def("get_repository", &PlanningFDRTask::get_repository)
            .def("get_fdr_context", &PlanningFDRTask::get_fdr_context, nb::rv_policy::reference_internal)
            .def("get_domain", &PlanningFDRTask::get_domain, nb::rv_policy::reference_internal);
    }
}

}
