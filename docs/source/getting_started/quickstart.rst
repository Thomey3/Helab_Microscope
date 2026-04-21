Quick Start
===========

This guide will help you get your first image from the Helab Microscope.

Step 1: Launch Micro-Manager
-----------------------------

1. Open Micro-Manager Studio
2. Go to **Tools → Hardware Configuration Wizard**
3. Create a new configuration or add to existing

Step 2: Add the Device
-----------------------

1. In the Hardware Configuration Wizard, click **Add**
2. Find **HelabMicroscopeHub** in the device list
3. Select it and click **Add**

The hub will automatically detect and add:

- **HelabCamera** - The two-photon camera device
- **HelabLaser_920nm** - 920nm laser control
- **HelabLaser_1064nm** - 1064nm laser control

Step 3: Configure Hardware
---------------------------

Set the device properties in the Device Property Browser:

.. list-table::
   :header-rows: 1

   * - Property
     - Value
     - Description
   * - DeviceName
     - Dev1
     - NI-DAQ device name
   * - X-Channel
     - ao0
     - X galvo output channel
   * - Y-Channel
     - ao1
     - Y galvo output channel
   * - CounterChannel
     - ctr0
     - Pixel clock counter

Step 4: Acquire Your First Image
---------------------------------

Using the GUI
~~~~~~~~~~~~~

1. Click **Snap** to acquire a single image
2. Use **Live** for continuous preview
3. Adjust exposure and laser power as needed

Using the API (Python)
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   import pymmcore

   # Initialize
   mmc = pymmcore.CMMCore()
   mmc.loadDevice("Hub", "Helab_Microscope", "HelabMicroscopeHub")
   mmc.initializeAllDevices()

   # Configure imaging
   mmc.setProperty("HelabCamera", "FPS", 1.5)
   mmc.setProperty("HelabCamera", "Width", 512)
   mmc.setProperty("HelabCamera", "Height", 512)

   # Enable laser
   mmc.setProperty("HelabLaser_920nm", "Power", 100.0)  # 100 mW
   mmc.setProperty("HelabLaser_920nm", "Enable", "On")

   # Acquire
   mmc.snapImage()
   image = mmc.getImage()

   print(f"Image size: {mmc.getImageWidth()} x {mmc.getImageHeight()}")

Using the API (C++)
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "app/MicroscopeController.h"
   #include "hal/HardwareFactory.h"

   using namespace helab;

   int main() {
       // Create factory and controller
       auto factory = std::make_shared<HardwareFactory>();
       MicroscopeController controller(factory);

       // Initialize
       auto result = controller.initialize();
       if (!result) {
           std::cerr << "Error: " << result.error().message << std::endl;
           return 1;
       }

       // Configure imaging
       ImagingConfig config;
       config.fps = 1.5;
       config.width = 512;
       config.height = 512;
       controller.setImagingConfig(config);

       // Add and enable laser
       controller.addLaserByType("920nm", "main");
       controller.setLaserPower("main", 100.0);  // 100 mW
       controller.enableLaser("main", true);

       // Acquire single frame
       std::vector<uint8_t> frame;
       controller.acquireSingleFrame(frame);

       std::cout << "Acquired frame: " << frame.size() << " bytes" << std::endl;

       return 0;
   }

Step 5: Save Your Configuration
--------------------------------

1. In the Hardware Configuration Wizard, click **Save**
2. Choose a location for the .cfg file
3. Load this configuration automatically on startup

Next Steps
----------

- :doc:`/user_guide/imaging` - Learn about imaging modes
- :doc:`/user_guide/laser_control` - Control multiple lasers
- :doc:`/architecture/overview` - Understand the architecture
- :doc:`/api/api_reference` - Full API reference
