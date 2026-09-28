"""Configure the llmcpp documentation site and C++ reference."""

project = "LLMCPP"
extensions = ["myst_parser", "breathe", "sphinxcontrib.mermaid"]
source_suffix = {".md": "markdown"}
root_doc = "index"
exclude_patterns = ["requirements.txt"]
html_theme = "sphinx_rtd_theme"
html_theme_options = {"navigation_depth": 3}
myst_heading_anchors = 3
myst_enable_extensions = ["deflist"]
breathe_default_project = "llmcpp"
breathe_projects = {}
breathe_default_members = ("members", "undoc-members")
