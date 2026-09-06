from typing import TypeAlias, TypedDict
from .formalism import planning as schema
from .planning import State


Row: TypeAlias = (
    schema.Object | schema.Variable | schema.StaticPredicate | schema.StaticPredicateBinding |
    schema.FluentPredicate | schema.FluentPredicateBinding | schema.DerivedPredicate | schema.DerivedPredicateBinding |
    schema.StaticFunction | schema.StaticFunctionBinding | schema.FluentFunction | schema.FluentFunctionBinding |
    schema.AuxiliaryFunction | schema.AuxiliaryFunctionBinding | schema.ActionBinding | schema.AxiomBinding |
    schema.Term | schema.StaticAtom | schema.StaticLiteral | schema.StaticGroundAtom |
    schema.StaticGroundLiteral | schema.FluentAtom | schema.FluentLiteral | schema.FluentGroundAtom |
    schema.FluentGroundLiteral | schema.DerivedAtom | schema.DerivedLiteral | schema.DerivedGroundAtom |
    schema.DerivedGroundLiteral | schema.StaticFunctionTerm | schema.StaticGroundFunctionTerm | schema.StaticGroundFunctionTermValue |
    schema.FluentFunctionTerm | schema.FluentGroundFunctionTerm | schema.FluentGroundFunctionTermValue | schema.AuxiliaryFunctionTerm |
    schema.AuxiliaryGroundFunctionTerm | schema.AuxiliaryGroundFunctionTermValue | schema.FluentFDRVariable | schema.FluentFDRFact |
    schema.FunctionExpression | schema.ArithmeticOperator | schema.BooleanOperator | schema.UnaryOperator |
    schema.MultiOperator | schema.BinaryArithmeticOperator | schema.BinaryBooleanOperator | schema.FluentNumericEffectOperator |
    schema.FluentNumericEffect | schema.AuxiliaryNumericEffectOperator | schema.AuxiliaryNumericEffect | schema.GroundFunctionExpression |
    schema.GroundArithmeticOperator | schema.GroundBooleanOperator | schema.GroundUnaryOperator | schema.GroundMultiOperator |
    schema.GroundBinaryArithmeticOperator | schema.GroundBinaryBooleanOperator | schema.FluentGroundNumericEffectOperator | schema.FluentGroundNumericEffect |
    schema.AuxiliaryGroundNumericEffectOperator | schema.AuxiliaryGroundNumericEffect | schema.Action | schema.Axiom |
    schema.ConditionalEffect | schema.ConjunctiveCondition | schema.ConjunctiveEffect | schema.GroundAction |
    schema.GroundAxiom | schema.GroundConditionalEffect | schema.GroundConjunctiveCondition | schema.GroundConjunctiveEffect |
    schema.Metric | schema.Domain | schema.LiftedTask | schema.GroundTask |
    State
)


class Table(TypedDict):
    prefix: str
    rows: list[Row]


class EnumEntry(TypedDict):
    id: int
    name: str
