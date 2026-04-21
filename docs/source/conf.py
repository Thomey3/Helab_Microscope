# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import sys
from datetime import datetime

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Helab Microscope'
copyright = f'{datetime.now().year}, HeLab'
author = 'HeLab'
release = '1.0.0'
version = '1.0.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.duration',
    'sphinx.ext.doctest',
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx.ext.intersphinx',
    'sphinx.ext.viewcode',
    'sphinx.ext.graphviz',
    'sphinx.ext.todo',
    'sphinx.ext.coverage',
    'breathe',
    'exhale',
    'myst_parser',
    'sphinx_copybutton',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
# Uncomment and add logo files when available:
# html_logo = '_static/logo.png'
# html_favicon = '_static/favicon.ico'

html_theme_options = {
    'canonical_url': 'https://helab-microscope.readthedocs.io/',
    'analytics_id': '',
    'display_version': True,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    'logo_only': False,
    'collapse_navigation': True,
    'sticky_navigation': True,
    'titles_only': False,
    'navigation_depth': 4,
    'includehidden': True,
    'toc_title': 'Table of Contents',
}

html_context = {
    'display_github': True,
    'github_user': 'Thomey3',
    'github_repo': 'Helab_Microscope',
    'github_version': 'main',
    'conf_py_path': '/docs/source/',
}

# -- Breathe configuration ---------------------------------------------------
# https://breathe.readthedocs.io/

breathe_projects = {
    'helab_microscope': '../doxygen/xml'
}
breathe_default_project = 'helab_microscope'
breathe_default_members = ('members', 'undoc-members', 'protected-members', 'private-members')

# -- Exhale configuration ----------------------------------------------------
# https://exhale.readthedocs.io/

exhale_args = {
    'doxygenStripFromPath': '../../Device/include',
    'containmentFolder': './api',
    'rootFileName': 'api_reference.rst',
    'rootFileTitle': 'API Reference',
    'doxygenConfigFile': '../Doxyfile',
    'createTreeView': True,
    'exhaleExecutesDoxygen': True,
    'exhaleUseDoxyfile': True,
    'unabridgedOrphanKinds': {
        'namespace': {'members', 'functions', 'classes', 'structs', 'enums', 'variables', 'defines'},
        'class': {'members', 'functions', 'variables'},
        'struct': {'members', 'functions', 'variables'},
        'enum': {'values'},
    },
    'afterTitleDescription': '.. contents:: Table of Contents\n   :local:\n   :depth: 2\n',
}

# -- Intersphinx configuration -----------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/extensions/intersphinx.html

intersphinx_mapping = {
    'python': ('https://docs.python.org/3/', None),
    'sphinx': ('https://www.sphinx-doc.org/en/master/', None),
}

# -- MyST configuration ------------------------------------------------------
# https://myst-parser.readthedocs.io/

myst_enable_extensions = [
    'amsmath',
    'colon_fence',
    'deflist',
    'dollarmath',
    'fieldlist',
    'html_admonition',
    'html_image',
    'linkify',
    'replacements',
    'smartquotes',
    'strikethrough',
    'substitution',
    'tasklist',
]

myst_heading_anchors = 3

# -- Graphviz configuration --------------------------------------------------

graphviz_output_format = 'svg'

# -- Todo configuration ------------------------------------------------------

todo_include_todos = True

# -- Copybutton configuration ------------------------------------------------

copybutton_prompt_text = r'>>> |\.\.\. |\$ |In \[\d*\]: | {2,5}\.\.\.: | {5,8}: '
copybutton_prompt_is_regexp = True
copybutton_line_continuation_character = '\\'
copybutton_remove_prompts = True
