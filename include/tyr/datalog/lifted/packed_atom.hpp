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

#ifndef TYR_DATALOG_LIFTED_PACKED_ATOM_HPP_
#define TYR_DATALOG_LIFTED_PACKED_ATOM_HPP_

#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>
#include <yggdrasil/containers/packed_object_row.hpp>

namespace tyr::datalog
{

/// A lifted atom compiled against the packed layout of its predicate: the binding of a ground instance follows from the
/// rule's object binding, without grounding the terms or looking the binding up in the repository.
template<formalism::FactKind F>
class PackedAtom
{
public:
    using Binding = formalism::datalog::PredicateBindingIndex<F>;

    /// nullopt if the bindings of the atom's predicate are interned or its layout is not set yet.
    static std::optional<PackedAtom> compile(formalism::datalog::AtomView<LiftedTag, F> atom, const formalism::datalog::Repository& repository)
    {
        using Relation = formalism::Predicate<F>;
        if constexpr (formalism::datalog::Repository::stores_bindings<Relation>)
            return std::nullopt;
        else
        {
            const auto relation = atom.get_predicate().get_index();
            const auto* layout = repository.get_relation_repository<Relation>().find_layout(relation);
            if (!layout)
                return std::nullopt;

            auto result = PackedAtom(relation);
            auto position = size_t(0);
            for (const auto term : atom.get_terms())
            {
                const auto field = layout->field_encoding(position++);
                visit(
                    [&]<typename Alternative>(Alternative arg)
                    {
                        if constexpr (std::same_as<Alternative, formalism::ParameterIndex>)
                            result.m_parameters.push_back(Parameter { ygg::uint_t(arg), field });
                        else
                        {
                            const auto object = ygg::uint_t(arg.get_index());
                            const auto object_position = position_of(field, object);
                            if (object_position >= field.radix)
                                result.m_encodable = false;
                            else
                                result.m_base += object_position * field.stride;
                        }
                    },
                    term.get_variant());
            }
            return result;
        }
    }

    bool uses_parameter(ygg::uint_t parameter) const noexcept
    {
        return std::ranges::any_of(m_parameters, [&](const auto& entry) { return entry.index == parameter; });
    }

    template<typename Callback>
    void for_each_parameter(Callback&& callback) const
    {
        for (const auto& entry : m_parameters)
            callback(entry.index);
    }

    /// The binding of the instance under the rule's object binding, or nullopt if an object lies outside its argument's domain.
    std::optional<Binding> encode(std::span<const ygg::Index<formalism::Object>> binding) const noexcept
    {
        if (!m_encodable)
            return std::nullopt;
        auto row = m_base;
        for (const auto& parameter : m_parameters)
        {
            const auto object_position = position_of(parameter.field, ygg::uint_t(binding[parameter.index]));
            if (object_position >= parameter.field.radix)
                return std::nullopt;
            row += object_position * parameter.field.stride;
        }
        return Binding { m_relation, ygg::Index<ygg::formalism::Row>(static_cast<ygg::uint_t>(row)) };
    }

private:
    using Layout = ygg::PackedRowLayout<ygg::Index<formalism::Object>>;
    using FieldEncoding = typename Layout::FieldEncoding;

    struct Parameter
    {
        ygg::uint_t index;
        FieldEncoding field;
    };

    /// The object's position in the field's domain; at least the radix if it is not in the domain.
    static std::uint64_t position_of(const FieldEncoding& field, ygg::uint_t object) noexcept
    {
        if (field.identity)
            return object;
        return object < field.position_of.size() ? field.position_of[object] : field.radix;
    }

    explicit PackedAtom(ygg::Index<formalism::Predicate<F>> relation) : m_relation(relation) {}

    ygg::Index<formalism::Predicate<F>> m_relation;
    std::uint64_t m_base = 0;  ///< The rank contribution of the constant arguments.
    bool m_encodable = true;   ///< False if a constant argument lies outside its domain.
    std::vector<Parameter> m_parameters;
};

}

#endif
