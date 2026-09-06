#ifndef TYR_SERIALIZATION_DICTIONARIES_HPP_
#define TYR_SERIALIZATION_DICTIONARIES_HPP_

#include "tyr/serialization/serializer.hpp"

#include <any>
#include <boost/json.hpp>
#include <cstdint>
#include <filesystem>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::serialization
{

template<typename T>
struct EnumTraits;

namespace detail
{
template<DictionaryValue T>
auto identity(const T& value)
{
    if constexpr (requires { value.get_state_repository(); })
        return value.identifying_members();
    else
    {
        const auto* context = &value.get_context();
        if constexpr (requires { context->get_canonical_context(value.get_handle()); })
            context = &context->get_canonical_context(value.get_handle());
        // Addresses distinguish independent repository factories; they never appear in the output.
        return std::make_pair(reinterpret_cast<std::uintptr_t>(context), value.get_handle());
    }
}

template<DictionaryValue T>
using Identity = decltype(identity(std::declval<const T&>()));

template<DictionaryValue T>
using Index = std::unordered_map<Identity<T>, size_t, ygg::Hash<Identity<T>>, ygg::EqualTo<Identity<T>>>;
}

/// Referenced formalism repositories (including their parents) must outlive this object.
class Dictionaries
{
    struct Table
    {
        std::string name;
        std::string prefix;
        boost::json::array rows;
        std::any index;
    };

    std::vector<Table> m_tables;
    std::unordered_map<std::type_index, size_t> m_types;
    boost::json::object m_enums;
    bool m_started = false;
    bool m_failed = false;

    void check_valid() const;

    class Archive
    {
        Dictionaries& m_dictionaries;
        std::string m_type_name;

    public:
        boost::json::object fields;

        Archive(Dictionaries& dictionaries, std::string name) : m_dictionaries(dictionaries), m_type_name(std::move(name)) {}

        template<typename T>
        void field(std::string_view name, const T& value)
        {
            fields[name] = m_dictionaries.encode(value);
        }

        template<typename Variant>
        void variant(const Variant& value)
        {
            // Visit the underlying variant directly: ygg::visit is noexcept, while serialization can throw.
            std::visit([&](const auto& alternative)
            {
                using Alternative = std::remove_cvref_t<decltype(alternative)>;
                using Context = std::remove_cvref_t<decltype(value.get_context())>;
                auto write = [&](const auto& item)
                {
                    const auto kind = value.index_variant().index();
                    m_dictionaries.add_kind(m_type_name, kind, type_name<std::remove_cvref_t<decltype(item)>>());
                    fields["kind"] = kind;
                    fields["value"] = m_dictionaries.encode(item);
                };
                if constexpr (ygg::ViewConcept<Alternative, Context>)
                    write(ygg::make_view(alternative, value.get_context()));
                else
                    write(alternative);
            }, value.index_variant());
        }
    };

    void add_kind(std::string_view type, size_t id, std::string name);

    template<Serializable T>
    boost::json::object body(const T& value)
    {
        Archive archive(*this, Serializer<T>::name());
        Serializer<T>::save(archive, value);
        return std::move(archive.fields);
    }

    template<typename T>
    boost::json::value encode(const T& value)
    {
        if constexpr (Serializable<T>)
        {
            if constexpr (DictionaryValue<T>)
            {
                if (const auto found = m_types.find(typeid(T)); found != m_types.end())
                {
                    auto& table = m_tables[found->second];
                    auto& index = std::any_cast<detail::Index<T>&>(table.index);
                    const auto [entry, inserted] = index.try_emplace(detail::identity(value), table.rows.size());
                    const auto id = entry->second;
                    const auto reference = table.prefix + std::to_string(id);
                    if (inserted)
                    {
                        table.rows.emplace_back(nullptr);
                        // Descendants may append to this table. Keep the index, not a reference to its row.
                        auto row = body(value);
                        table.rows[id] = std::move(row);
                    }
                    return boost::json::value(reference);
                }
            }
            return body(value);
        }
        else if constexpr (std::same_as<T, bool>)
            return value;
        else if constexpr (std::integral<T>)
        {
            if constexpr (std::is_signed_v<T>)
                return static_cast<std::int64_t>(value);
            else
                return static_cast<std::uint64_t>(value);
        }
        else if constexpr (std::floating_point<T>)
            return static_cast<double>(value);
        else if constexpr (PrimitiveSerializable<T>)
            return encode(PrimitiveSerializer<T>::value(value));
        else if constexpr (std::is_enum_v<T>)
        {
            const auto id = static_cast<std::underlying_type_t<T>>(value);
            add_kind(EnumTraits<T>::name(), id, EnumTraits<T>::label(value));
            return id;
        }
        else if constexpr (std::same_as<T, std::filesystem::path>)
            return boost::json::value(value.generic_string());
        else if constexpr (requires { std::string_view(value); })
            return boost::json::value(std::string_view(value));
        else if constexpr (requires { value.has_value(); *value; })
            return value.has_value() ? encode(*value) : boost::json::value(nullptr);
        else if constexpr (std::ranges::input_range<const T>)
        {
            boost::json::array array;
            for (const auto& item : value)
                array.push_back(encode(item));
            return array;
        }
        else if constexpr (requires { std::tuple_size<T>::value; })
        {
            boost::json::array array;
            std::apply([&](const auto&... item) { (array.push_back(encode(item)), ...); }, value);
            return array;
        }
        else
            static_assert(sizeof(T) == 0, "Missing serializer for field type");
    }

public:
    template<DictionaryValue T>
    void register_table(std::string name, std::string prefix)
    {
        check_valid();
        if (m_started)
            throw std::logic_error("Register tables before serialization begins");
        if (name.empty() || prefix.empty() || (prefix.back() >= '0' && prefix.back() <= '9'))
            throw std::invalid_argument("Table name and prefix must be nonempty; prefix must end in a non-digit");
        if (m_types.contains(typeid(T)))
            throw std::invalid_argument("Type already has a dictionary table");
        for (const auto& table : m_tables)
            if (table.name == name || table.prefix == prefix)
                throw std::invalid_argument("Table names and prefixes must be unique");
        const auto position = m_tables.size();
        m_tables.push_back({std::move(name), std::move(prefix), {}, detail::Index<T> {}});
        m_types.emplace(typeid(T), position);
    }

    template<Serializable T>
    boost::json::value serialize(const T& value)
    {
        check_valid();
        m_started = true;
        try { return encode(value); }
        catch (...) { m_failed = true; throw; }
    }

    template<DictionaryValue T>
    boost::json::array table() const
    {
        check_valid();
        const auto found = m_types.find(typeid(T));
        if (found == m_types.end())
            throw std::invalid_argument("Type has no registered dictionary table");
        return m_tables[found->second].rows;
    }

    boost::json::object tables() const;
    boost::json::object enums() const;
};

}

#endif
