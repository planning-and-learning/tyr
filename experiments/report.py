from downward.reports.absolute import AbsoluteReport


class BaseReport(AbsoluteReport):
    INFO_ATTRIBUTES = ["time_limit", "wall_time_limit", "memory_limit"]
    ERROR_ATTRIBUTES = [
        "domain",
        "problem",
        "algorithm",
        "unexplained_errors",
        "error",
        "node",
    ]
