Application Layer
=================

The Application Layer contains the business logic for microscope control.

MicroscopeController
--------------------

The central coordinator for all microscope operations.

.. doxygenclass:: helab::MicroscopeController
   :members:
   :protected-members:
   :undoc-members:

Configuration Structures
------------------------

MicroscopeConfig
~~~~~~~~~~~~~~~~

.. doxygenstruct:: helab::MicroscopeConfig
   :members:

ImagingConfig
~~~~~~~~~~~~~

.. doxygenstruct:: helab::ImagingConfig
   :members:

ROI (Region of Interest)
~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: helab::ROI
   :members:

MicroscopeState
~~~~~~~~~~~~~~~

.. doxygenenum:: helab::MicroscopeState

Usage Examples
--------------

Basic Initialization
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "app/MicroscopeController.h"
   #include "hal/HardwareFactory.h"

   using namespace helab;

   int main() {
       // Create factory
       auto factory = std::make_shared<HardwareFactory>();

       // Create controller
       MicroscopeController controller(factory);

       // Initialize hardware
       auto result = controller.initialize();
       if (!result) {
           std::cerr << "Failed to initialize: " << result.error().message << std::endl;
           return 1;
       }

       std::cout << "Microscope initialized successfully" << std::endl;
       return 0;
   }

Imaging Configuration
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Configure imaging parameters
   ImagingConfig config;
   config.fps = 1.5;           // 1.5 frames per second
   config.width = 512;         // 512 pixels width
   config.height = 512;        // 512 pixels height
   config.channels = 1;        // Single channel
   config.pixelSizeUm = 0.5;   // 0.5 µm per pixel

   auto result = controller.setImagingConfig(config);
   if (!result) {
       std::cerr << "Invalid configuration: " << result.error().message << std::endl;
   }

Laser Control
~~~~~~~~~~~~~

.. code-block:: cpp

   // Add laser by type
   auto result = controller.addLaserByType("920nm", "main");
   if (!result) {
       std::cerr << "Failed to add laser: " << result.error().message << std::endl;
       return 1;
   }

   // Set power to 100 mW
   result = controller.setLaserPower("main", 100.0);
   if (!result) {
       std::cerr << "Failed to set power: " << result.error().message << std::endl;
   }

   // Enable laser
   result = controller.enableLaser("main", true);
   if (!result) {
       std::cerr << "Failed to enable laser: " << result.error().message << std::endl;
   }

ROI Setting
~~~~~~~~~~~

.. code-block:: cpp

   // Set region of interest
   ROI roi;
   roi.x = 100;
   roi.y = 100;
   roi.width = 256;
   roi.height = 256;

   auto result = controller.setROI(roi);
   if (!result) {
       std::cerr << "Invalid ROI: " << result.error().message << std::endl;
   }

Scan Type Selection
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Switch to galvo scanning
   auto result = controller.setScanType(ScanPattern::Type::Galvo);

   // Future: Switch to resonance scanning
   // result = controller.setScanType(ScanPattern::Type::Resonance);

Image Acquisition
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Single frame acquisition
   std::vector<uint8_t> frame;
   auto result = controller.acquireSingleFrame(frame);
   if (result) {
       std::cout << "Acquired " << frame.size() << " bytes" << std::endl;
   }

   // Continuous acquisition with callback
   controller.setImageCallback([](const std::vector<uint8_t>& frame, uint64_t timestamp) {
       std::cout << "Frame at " << timestamp << ": " << frame.size() << " bytes" << std::endl;
   });

   controller.startImaging();
   // ... acquisition running ...
   controller.stopImaging();

State Management
----------------

State Transitions
~~~~~~~~~~~~~~~~~

.. code-block:: text

   Uninitialized ──initialize()──→ Ready
        ↑                              │
        │                              ├──startImaging()──→ Imaging
        │                              │                         │
        └──shutdown()──────────────────┴──stopImaging()──────────┘

Querying State
~~~~~~~~~~~~~~

.. code-block:: cpp

   MicroscopeState state = controller.state();

   switch (state) {
       case MicroscopeState::Uninitialized:
           std::cout << "Not initialized" << std::endl;
           break;
       case MicroscopeState::Ready:
           std::cout << "Ready for imaging" << std::endl;
           break;
       case MicroscopeState::Imaging:
           std::cout << "Currently imaging" << std::endl;
           break;
       case MicroscopeState::Error:
           std::cout << "Error state" << std::endl;
           break;
   }

Error Recovery
--------------

When errors occur, the controller attempts to recover:

.. code-block:: cpp

   auto result = controller.startImaging();
   if (!result) {
       // Check if we can recover
       if (controller.state() == MicroscopeState::Error) {
           // Try to reset
           controller.shutdown();
           controller.initialize();
       }
   }

Dependency Injection
--------------------

The controller accepts any factory implementing :cpp:class:`IHardwareFactory`:

.. code-block:: cpp

   // Production factory
   auto prodFactory = std::make_shared<HardwareFactory>();

   // Test factory with mocks
   auto testFactory = std::make_shared<MockHardwareFactory>();

   // Same controller works with both
   MicroscopeController prodController(prodFactory);
   MicroscopeController testController(testFactory);
