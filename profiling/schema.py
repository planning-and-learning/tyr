# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Schema of profiling suites and of the benchmark attributes they declare."""

from collections.abc import Callable
from enum import Enum
from typing import Any, cast

AttributeValue = bool | int | float | str | None


class AttributeType(Enum):
    """Value type of a benchmark attribute."""

    BOOL = "bool"
    FLOAT = "float"
    INT = "int"
    STR = "str"


class AttributeCompare(Enum):
    """How a profiling comparison judges an attribute's change between two runs."""

    STRICT_EQUALITY = "strict_equality"
    LOWER_IS_BETTER = "lower_is_better"
    HIGHER_IS_BETTER = "higher_is_better"
    UNDEFINED = "undefined"


def require_mapping(value: object, name: str) -> dict[str, Any]:
    """Return ``value`` as a JSON object, or raise TypeError naming the field ``name``."""
    if not isinstance(value, dict):
        msg = f"'{name}' must be an object."
        raise TypeError(msg)
    return cast("dict[str, Any]", value)


def require_non_empty_string(value: object, name: str) -> str:
    """Return ``value`` as a non-empty string, or raise ValueError naming the field ``name``."""
    if not isinstance(value, str) or not value:
        msg = f"'{name}' must be a non-empty string."
        raise ValueError(msg)
    return value


def validate_attributes(attributes: object) -> None:
    """Raise ValueError (TypeError for a non-object) unless every attribute declares a valid type and compare rule."""
    attributes = require_mapping(attributes, "attributes")

    for name, raw_config in attributes.items():
        require_non_empty_string(name, "attribute name")
        config = require_mapping(raw_config, f"attributes.{name}")

        try:
            AttributeType(config["type"])
        except KeyError as error:
            msg = f"Attribute '{name}' is missing required field 'type'."
            raise ValueError(msg) from error
        except ValueError as error:
            valid_values = ", ".join(attribute_type.value for attribute_type in AttributeType)
            msg = f"Attribute '{name}' has invalid type '{config['type']}'. Expected one of: {valid_values}."
            raise ValueError(msg) from error

        try:
            AttributeCompare(config["compare"])
        except KeyError as error:
            msg = f"Attribute '{name}' is missing required field 'compare'."
            raise ValueError(msg) from error
        except ValueError as error:
            valid_values = ", ".join(attribute_compare.value for attribute_compare in AttributeCompare)
            msg = f"Attribute '{name}' has invalid compare '{config['compare']}'. Expected one of: {valid_values}."
            raise ValueError(msg) from error


def validate_suite(suite: object) -> None:
    """Raise ValueError (TypeError for a non-object) unless ``suite`` is a well-formed profiling suite."""
    suite = require_mapping(suite, "suite")
    if "prefix" in suite:
        require_non_empty_string(suite["prefix"], "prefix")

    try:
        domains = require_mapping(suite["domains"], "domains")
    except KeyError as error:
        msg = "Suite is missing required field 'domains'."
        raise ValueError(msg) from error

    if not domains:
        msg = "'domains' must contain at least one domain."
        raise ValueError(msg)

    validate_attributes(suite.get("attributes", {}))

    for domain_name, raw_domain_config in domains.items():
        require_non_empty_string(domain_name, "domain name")
        domain_config = require_mapping(raw_domain_config, f"domains.{domain_name}")

        try:
            require_non_empty_string(domain_config["domain_file"], f"domains.{domain_name}.domain_file")
        except KeyError as error:
            msg = f"Domain '{domain_name}' is missing required field 'domain_file'."
            raise ValueError(msg) from error

        try:
            tasks = require_mapping(domain_config["tasks"], f"domains.{domain_name}.tasks")
        except KeyError as error:
            msg = f"Domain '{domain_name}' is missing required field 'tasks'."
            raise ValueError(msg) from error

        if not tasks:
            msg = f"Domain '{domain_name}' must contain at least one task."
            raise ValueError(msg)

        for task_name, task_file in tasks.items():
            require_non_empty_string(task_name, f"domains.{domain_name} task name")
            require_non_empty_string(task_file, f"domains.{domain_name}.tasks.{task_name}")


def validate_attribute_value(attribute_name: str, attribute_config: dict[str, Any], value: object) -> None:
    """Raise ValueError unless ``value`` fits the attribute's declared type."""
    normalize_attribute_value(attribute_name, attribute_config, value)


def _as_bool(value: object) -> AttributeValue:
    return value if isinstance(value, bool) else None


def _as_float(value: object) -> AttributeValue:
    return value if isinstance(value, (int, float)) and not isinstance(value, bool) else None


def _as_int(value: object) -> AttributeValue:
    if isinstance(value, float) and value.is_integer():
        return int(value)
    return value if isinstance(value, int) and not isinstance(value, bool) else None


def _as_str(value: object) -> AttributeValue:
    return value if isinstance(value, str) else None


# Each returns the value in its attribute type, or None if the value does not have that type.
_NORMALIZERS: dict[AttributeType, Callable[[object], AttributeValue]] = {
    AttributeType.BOOL: _as_bool,
    AttributeType.FLOAT: _as_float,
    AttributeType.INT: _as_int,
    AttributeType.STR: _as_str,
}


def normalize_attribute_value(attribute_name: str, attribute_config: dict[str, Any], value: object) -> AttributeValue:
    """Return ``value`` in the attribute's declared type (integral floats become ints); None stays None.

    Raises:
        ValueError: If the value does not have the declared type.
    """
    if value is None:
        return None

    attribute_type = AttributeType(attribute_config["type"])
    normalized = _NORMALIZERS[attribute_type](value)
    if normalized is None:
        msg = f"Attribute '{attribute_name}' expected {attribute_type.value} value, got {type(value).__name__}."
        raise ValueError(msg)
    return normalized
