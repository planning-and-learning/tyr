# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later
"""Sphinx configuration for tyr's documentation.

Build it with `build/docs-venv/bin/sphinx-build -W -b html docs build/docs/html`; the virtual environment comes from
docs/requirements.txt.
"""

project = "tyr"
author = "Dominik Drexler"
project_copyright = "2026, Dominik Drexler"

extensions = ["sphinx.ext.githubpages"]

html_theme = "furo"
html_title = "tyr"
html_theme_options = {
    "source_repository": "https://github.com/planning-and-learning/tyr/",
    "source_branch": "main",
    "source_directory": "docs/",
}
