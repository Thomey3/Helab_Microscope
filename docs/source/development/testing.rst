Testing
=======

The Helab Microscope has comprehensive unit tests using GoogleTest.

Test Structure
--------------

.. code-block:: text

   Device/tests/
   ├── mocks/
   │   └── MockHardware.h        # Mock implementations
   └── app/
       └── MicroscopeControllerTest.cpp  # Controller tests

Running Tests
-------------

Build with Tests
~~~~~~~~~~~~~~~~

.. code-block:: bash

   cmake .. -DBUILD_TESTS=ON
   make -j$(nproc)

Run All Tests
~~~~~~~~~~~~~

.. code-block:: bash

   # Using CTest
   ctest --output-on-failure

   # Run directly
   ./Helab_Microscope_Tests

Run Specific Tests
~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Run specific test suite
   ./Helab_Microscope_Tests --gtest_filter=MicroscopeControllerTest.*

   # Run single test
   ./Helab_Microscope_Tests --gtest_filter=MicroscopeControllerTest.Initialize*

Test Output
~~~~~~~~~~~

.. code-block:: text

   [==========] Running 10 tests from 1 test suite.
   [----------] Global test environment set-up.
   [----------] 10 tests from MicroscopeControllerTest
   [ RUN      ] MicroscopeControllerTest.Initialize_WhenNotInitialized_InitializesAllHardware
   [       OK ] MicroscopeControllerTest.Initialize_WhenNotInitialized_InitializesAllHardware (1 ms)
   [ RUN      ] MicroscopeControllerTest.StartImaging_WhenNotInitialized_ReturnsError
   [       OK ] MicroscopeControllerTest.StartImaging_WhenNotInitialized_ReturnsError (0 ms)
   ...
   [----------] 10 tests from MicroscopeControllerTest (5 ms total)
   [==========] 10 tests from 1 test suite ran. (5 ms total)
   [  PASSED  ] 10 tests.

Test Coverage
-------------

Generate Coverage Report
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Build with coverage flags
   cmake .. -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage"
   make -j$(nproc)

   # Run tests
   ctest --output-on-failure

   # Generate report
   lcov --capture --directory . --output-file coverage.info
   genhtml coverage.info --output-directory coverage_html

   # View report
   open coverage_html/index.html

Coverage Requirements
~~~~~~~~~~~~~~~~~~~~~

- **Minimum coverage**: 80%
- **Critical paths**: 100% (initialization, imaging, laser control)

Test Categories
---------------

Initialization Tests
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   TEST_F(MicroscopeControllerTest, Initialize_WhenNotInitialized_InitializesAllHardware) {
       // Arrange
       EXPECT_CALL(*mockFactory, createClock("nidaq")).Times(1);
       EXPECT_CALL(*mockFactory, createScanner("galvo")).Times(1);
       EXPECT_CALL(*mockFactory, createDetector("qt")).Times(1);

       MicroscopeController controller(mockFactory);

       // Act
       auto result = controller.initialize();

       // Assert
       EXPECT_TRUE(result);
       EXPECT_TRUE(controller.isInitialized());
   }

Imaging Tests
~~~~~~~~~~~~~

.. code-block:: cpp

   TEST_F(MicroscopeControllerTest, StartImaging_WhenNotInitialized_ReturnsError) {
       MicroscopeController controller(mockFactory);

       auto result = controller.startImaging();

       EXPECT_FALSE(result);
       EXPECT_EQ(result.error().code, static_cast<int>(HardwareErrorCode::NotInitialized));
   }

Laser Tests
~~~~~~~~~~~

.. code-block:: cpp

   TEST_F(MicroscopeControllerTest, SetLaserPower_WhenLaserExists_SetsPower) {
       MicroscopeController controller(mockFactory);
       controller.initialize();

       auto mockLaser = std::make_unique<NiceMock<MockLaser>>();
       EXPECT_CALL(*mockLaser, setPower(100.0))
           .WillOnce(Return(success()));

       controller.addLaser(std::move(mockLaser), "920nm");

       auto result = controller.setLaserPower("920nm", 100.0);

       EXPECT_TRUE(result);
   }

Mock Objects
------------

MockHardware.h
~~~~~~~~~~~~~~

.. code-block:: cpp

   class MockClock : public IClock {
   public:
       MOCK_METHOD(Result<void>, configure, (const ClockConfig&), (override));
       MOCK_METHOD(Result<void>, start, (), (override));
       MOCK_METHOD(Result<void>, stop, (), (override));
       MOCK_METHOD(Result<void>, reset, (), (override));
       MOCK_METHOD(bool, isRunning, (), (const, override));
   };

   class MockScanner : public IScanner { /* ... */ };
   class MockDetector : public IDetector { /* ... */ };
   class MockLaser : public ILaser { /* ... */ };

Using Mocks
~~~~~~~~~~~

.. code-block:: cpp

   // Create mock
   auto mockClock = std::make_unique<NiceMock<MockClock>>();

   // Set expectations
   EXPECT_CALL(*mockClock, start())
       .WillOnce(Return(success()));

   // Use mock
   auto result = mockClock->start();

Sanitizers
----------

Run tests with sanitizers for memory safety:

Address Sanitizer
~~~~~~~~~~~~~~~~~

.. code-block:: bash

   cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address -g"
   make -j$(nproc)
   ctest --output-on-failure

Undefined Behavior Sanitizer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=undefined -g"
   make -j$(nproc)
   ctest --output-on-failure

Thread Sanitizer
~~~~~~~~~~~~~~~~

.. code-block:: bash

   cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
   make -j$(nproc)
   ctest --output-on-failure

Integration Tests
-----------------

Hardware-in-the-Loop
~~~~~~~~~~~~~~~~~~~~

For testing with real hardware:

.. code-block:: cpp

   #ifdef HARDWARE_TEST
   TEST(HardwareIntegration, AcquireImage_ReturnsValidData) {
       auto factory = std::make_shared<HardwareFactory>();
       MicroscopeController controller(factory);

       controller.initialize();
       controller.startImaging();

       std::vector<uint8_t> frame;
       auto result = controller.acquireSingleFrame(frame);

       EXPECT_TRUE(result);
       EXPECT_GT(frame.size(), 0u);
   }
   #endif

Continuous Integration
----------------------

GitHub Actions
~~~~~~~~~~~~~~

.. code-block:: yaml

   name: Tests

   on: [push, pull_request]

   jobs:
     test:
       runs-on: ubuntu-latest
       steps:
         - uses: actions/checkout@v4
         - run: mkdir build && cd build
         - run: cmake .. -DBUILD_TESTS=ON
         - run: make -j$(nproc)
         - run: ctest --output-on-failure

Test Best Practices
-------------------

1. **AAA Pattern**: Arrange, Act, Assert
2. **One assertion per test** (when practical)
3. **Descriptive names**: Test_WhenCondition_ExpectedBehavior
4. **Independent tests**: No shared state
5. **Fast execution**: Mock external dependencies

Adding New Tests
----------------

1. Create test file in ``Device/tests/``
2. Include GoogleTest and mocks
3. Write test following AAA pattern
4. Add to CMakeLists.txt:

   .. code-block:: cmake

      list(APPEND TEST_SOURCES
          Device/tests/my_new_test.cpp
      )
