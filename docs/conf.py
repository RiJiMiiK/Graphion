# SPDX-License-Identifier: MIT

project = "Graphion"
author = "Graphion contributors"
copyright = "2026, Graphion contributors"

from pygments.lexers.special import TextLexer
from sphinx.highlighting import lexers

extensions = [
    "myst_parser",
]

source_suffix = {
    ".md": "markdown",
}

root_doc = "index"

exclude_patterns = [
    "_build",
]

html_theme = "furo"
html_title = "Graphion Documentation"
html_static_path = []

myst_enable_extensions = [
    "colon_fence",
    "deflist",
]

lexers["gion"] = TextLexer()
