from enum import Enum
from typing import Any, cast


AttributeValue = bool | int | float | str | None


class AttributeType(Enum):
    BOOL = "bool"
    FLOAT = "float"
    INT = "int"
    STR = "str"


class AttributeCompare(Enum):
    STRICT_EQUALITY = "strict_equality"
    LOWER_IS_BETTER = "lower_is_better"
    HIGHER_IS_BETTER = "higher_is_better"
    UNDEFINED = "undefined"


def require_mapping(value: object, name: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ValueError(f"'{name}' must be an object.")
    return cast(dict[str, Any], value)


def require_non_empty_string(value: object, name: str) -> str:
    if not isinstance(value, str) or not value:
        raise ValueError(f"'{name}' must be a non-empty string.")
    return value


def validate_attributes(attributes: object) -> None:
    attributes = require_mapping(attributes, "attributes")

    for name, config in attributes.items():
        require_non_empty_string(name, "attribute name")
        config = require_mapping(config, f"attributes.{name}")

        try:
            AttributeType(config["type"])
        except KeyError as error:
            raise ValueError(f"Attribute '{name}' is missing required field 'type'.") from error
        except ValueError as error:
            valid_values = ", ".join(attribute_type.value for attribute_type in AttributeType)
            raise ValueError(f"Attribute '{name}' has invalid type '{config['type']}'. Expected one of: {valid_values}.") from error

        try:
            AttributeCompare(config["compare"])
        except KeyError as error:
            raise ValueError(f"Attribute '{name}' is missing required field 'compare'.") from error
        except ValueError as error:
            valid_values = ", ".join(attribute_compare.value for attribute_compare in AttributeCompare)
            raise ValueError(f"Attribute '{name}' has invalid compare '{config['compare']}'. Expected one of: {valid_values}.") from error


def validate_suite(suite: object) -> None:
    suite = require_mapping(suite, "suite")
    if "prefix" in suite:
        require_non_empty_string(suite["prefix"], "prefix")

    try:
        domains = require_mapping(suite["domains"], "domains")
    except KeyError as error:
        raise ValueError("Suite is missing required field 'domains'.") from error

    if not domains:
        raise ValueError("'domains' must contain at least one domain.")

    validate_attributes(suite.get("attributes", {}))

    for domain_name, domain_config in domains.items():
        require_non_empty_string(domain_name, "domain name")
        domain_config = require_mapping(domain_config, f"domains.{domain_name}")

        try:
            require_non_empty_string(domain_config["domain_file"], f"domains.{domain_name}.domain_file")
        except KeyError as error:
            raise ValueError(f"Domain '{domain_name}' is missing required field 'domain_file'.") from error

        try:
            tasks = require_mapping(domain_config["tasks"], f"domains.{domain_name}.tasks")
        except KeyError as error:
            raise ValueError(f"Domain '{domain_name}' is missing required field 'tasks'.") from error

        if not tasks:
            raise ValueError(f"Domain '{domain_name}' must contain at least one task.")

        for task_name, task_file in tasks.items():
            require_non_empty_string(task_name, f"domains.{domain_name} task name")
            require_non_empty_string(task_file, f"domains.{domain_name}.tasks.{task_name}")


def validate_attribute_value(attribute_name: str, attribute_config: dict[str, Any], value: object) -> None:
    normalize_attribute_value(attribute_name, attribute_config, value)


def normalize_attribute_value(attribute_name: str, attribute_config: dict[str, Any], value: object) -> AttributeValue:
    if value is None:
        return None

    attribute_type = AttributeType(attribute_config["type"])

    if attribute_type == AttributeType.BOOL:
        if not isinstance(value, bool):
            raise ValueError(f"Attribute '{attribute_name}' expected bool value, got {type(value).__name__}.")
        return value

    if attribute_type == AttributeType.FLOAT:
        if not (isinstance(value, (int, float)) and not isinstance(value, bool)):
            raise ValueError(f"Attribute '{attribute_name}' expected float value, got {type(value).__name__}.")
        return value

    if attribute_type == AttributeType.INT:
        if isinstance(value, bool):
            raise ValueError(f"Attribute '{attribute_name}' expected int value, got bool.")
        if isinstance(value, int):
            return value
        if isinstance(value, float) and value.is_integer():
            return int(value)
        raise ValueError(f"Attribute '{attribute_name}' expected int value, got {type(value).__name__}.")

    if attribute_type == AttributeType.STR:
        if not isinstance(value, str):
            raise ValueError(f"Attribute '{attribute_name}' expected str value, got {type(value).__name__}.")
        return value

    raise ValueError(f"Attribute '{attribute_name}' has unsupported type '{attribute_config['type']}'.")
