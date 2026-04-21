Contributing
============

We welcome contributions to the Helab Microscope project!

Getting Started
---------------

1. Fork the repository on GitHub
2. Clone your fork:

   .. code-block:: bash

      git clone https://github.com/YOUR_USERNAME/Helab_Microscope.git
      cd Helab_Microscope

3. Create a branch:

   .. code-block:: bash

      git checkout -b feature/my-new-feature

Development Workflow
--------------------

1. **Make changes** following coding standards
2. **Add tests** for new functionality
3. **Run tests**:

   .. code-block:: bash

      ctest --output-on-failure

4. **Commit** with descriptive message:

   .. code-block:: bash

      git commit -m "feat: Add support for new scanner type"

5. **Push** and create pull request

Coding Standards
----------------

C++ Style
~~~~~~~~~

- **C++17** standard
- **clang-format** for formatting
- **Meaningful names**: ``pixelCount`` not ``pc``
- **Small functions**: < 50 lines
- **Small files**: < 800 lines

Naming Conventions
~~~~~~~~~~~~~~~~~~

+-------------------+------------------+
| Type              | Convention       |
+===================+==================+
| Classes           | ``PascalCase``   |
+-------------------+------------------+
| Functions         | ``camelCase``    |
+-------------------+------------------+
| Variables         | ``camelCase``    |
+-------------------+------------------+
| Constants         | ``kPascalCase``  |
+-------------------+------------------+
| Member variables  | ``camelCase_``   |
+-------------------+------------------+

Documentation
~~~~~~~~~~~~~

All public APIs must have Doxygen comments:

.. code-block:: cpp

   /// @brief Start the imaging acquisition.
   /// @return Success or error with message.
   /// @pre Controller must be initialized.
   /// @post State is Imaging.
   Result<void> startImaging();

Error Handling
~~~~~~~~~~~~~~

Use ``Result<T>`` for all operations that can fail:

.. code-block:: cpp

   auto result = doSomething();
   if (!result) {
       return make_error(result.error());
   }

Testing Requirements
--------------------

- **New features**: Must have unit tests
- **Bug fixes**: Must have regression test
- **Coverage**: Maintain 80%+ coverage

Pull Request Guidelines
-----------------------

PR Checklist
~~~~~~~~~~~~

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] New code has tests
- [ ] Documentation updated
- [ ] Commit messages follow convention
- [ ] Branch is up to date with main

Commit Message Format
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   <type>: <description>

   [optional body]

   [optional footer]

Types:

- ``feat``: New feature
- ``fix``: Bug fix
- ``docs``: Documentation
- ``style``: Formatting
- ``refactor``: Code refactoring
- ``test``: Adding tests
- ``chore``: Maintenance

Examples:

.. code-block:: text

   feat: Add resonance scanner support

   fix: Correct galvo waveform generation

   docs: Update installation instructions

Code Review
-----------

All PRs require review before merging.

Review Criteria
~~~~~~~~~~~~~~~

1. **Correctness**: Does it work?
2. **Design**: Is it well-designed?
3. **Performance**: Any performance issues?
4. **Security**: Any security concerns?
5. **Documentation**: Is it documented?

Reporting Issues
----------------

When reporting issues, include:

1. **Description**: What happened?
2. **Steps to reproduce**: How to trigger it
3. **Expected behavior**: What should happen?
4. **Actual behavior**: What actually happened
5. **Environment**: OS, compiler, hardware
6. **Logs**: Relevant log output

Issue Template
~~~~~~~~~~~~~~

.. code-block:: markdown

   ## Description
   [Brief description]

   ## Steps to Reproduce
   1. Step 1
   2. Step 2

   ## Expected Behavior
   [What should happen]

   ## Actual Behavior
   [What actually happened]

   ## Environment
   - OS: Windows 11
   - Compiler: MSVC 2022
   - Micro-Manager: 2.0

   ## Logs
   ```
   [Paste logs here]
   ```

License
-------

By contributing, you agree that your contributions will be licensed under the same license as the project.

Contact
-------

- **Issues**: GitHub Issues
- **Discussions**: GitHub Discussions
- **Email**: [Project email]
