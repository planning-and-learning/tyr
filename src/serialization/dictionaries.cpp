#include "tyr/serialization/dictionaries.hpp"

#include <algorithm>

namespace tyr::serialization
{

void Dictionaries::check_valid() const
{
    if (m_failed)
        throw std::logic_error("Serialization failed; create a new dictionary registry");
}

void Dictionaries::add_kind(std::string_view type, size_t id, std::string name)
{
    auto [entry, inserted] = m_enums.emplace(type, boost::json::array {});
    auto& rows = entry->value().as_array();
    const auto position = std::ranges::find_if(rows, [id](const auto& row) { return row.as_object().at("id").as_uint64() >= id; });
    if (position == rows.end() || position->as_object().at("id").as_uint64() != id)
        rows.insert(position, boost::json::object {{"id", id}, {"name", std::move(name)}});
}

boost::json::object Dictionaries::tables() const
{
    check_valid();
    boost::json::object result;
    for (const auto& table : m_tables)
        result[table.name] = boost::json::object {{"prefix", table.prefix}, {"rows", table.rows}};
    return result;
}

boost::json::object Dictionaries::enums() const
{
    check_valid();
    return m_enums;
}

}
