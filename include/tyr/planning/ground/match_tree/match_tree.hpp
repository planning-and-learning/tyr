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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_MATCH_TREE_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_MATCH_TREE_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include <urd/repr/queues/queue.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/repository.hpp"

#include <memory>
#include <optional>
#include <vector>
#include <urd/elements/flat.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning::match_tree
{

template<typename Tag>
class MatchTree
{
private:
    using NodeView = urd::resolved_t<Node<Tag>, Repository<Tag>>;

    struct Definition
    {
        explicit Definition(const formalism::planning::Repository& context) : repository(static_cast<ygg::uint_t>(0), context) {}

        Repository<Tag> repository;  // Constant index 0 is sufficient because match-tree node views are never compared.
        std::optional<Node<Tag>> root;
    };

    struct Evaluator
    {
        urd::Store<urd::queue<NodeView>, urd::lifo> stack;
    };

    explicit MatchTree(std::shared_ptr<const Definition> definition);

    std::shared_ptr<const Definition> m_definition;
    Evaluator m_evaluator;

public:
    MatchTree(urd::Store<urd::sequence<urd::view<Tag, formalism::planning::Repository>>> elements, const formalism::planning::Repository& context);
    ~MatchTree();

    static MatchTreePtr<Tag> create(urd::Store<urd::sequence<urd::view<Tag, formalism::planning::Repository>>> elements,
                                    const formalism::planning::Repository& context);

    [[nodiscard]] MatchTreePtr<Tag> make_worker() const;

    MatchTree(const MatchTree& other) = delete;
    MatchTree& operator=(const MatchTree& other) = delete;
    MatchTree(MatchTree&& other) = delete;
    MatchTree& operator=(MatchTree&& other) = delete;

    void generate(const StateContext<GroundTag>& state, urd::Store<urd::sequence<urd::view<Tag, formalism::planning::Repository>>>& out_applicable_elements);
};

}

#endif
