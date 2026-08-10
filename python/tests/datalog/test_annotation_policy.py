import pytest

from pytyr import datalog


@pytest.mark.parametrize("module", [datalog.ground, datalog.lifted])
def test_annotation_policy_api_is_unified(module) -> None:
    for name in (
        "NoAnnotationPolicy",
        "SumMinCostAnnotationPolicy",
        "MaxMinCostAnnotationPolicy",
        "MaxMinCostAnnotationWithAchieversPolicy",
    ):
        assert getattr(module, name)() is not None

    for name in (
        "NoOrAnnotationPolicy",
        "NoAndAnnotationPolicy",
        "OrAnnotationPolicy",
        "SumAndAnnotationPolicy",
        "MaxAndAnnotationPolicy",
        "MaxAchieverAndAnnotationPolicy",
    ):
        assert not hasattr(module, name)

    for workspace in (module.UnannotatedProgramWorkspace, module.SumProgramWorkspace, module.MaxProgramWorkspace):
        assert hasattr(workspace, "get_annotation_policy")
        assert hasattr(workspace, "get_annotations")
        assert hasattr(workspace, "get_numeric_annotations")
