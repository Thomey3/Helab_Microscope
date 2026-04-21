Configuration
=============

Configuration options for the Helab Microscope.

Configuration File
------------------

The microscope can be configured via JSON file:

.. code-block:: json

   {
     "microscope": {
       "name": "Helab Two-Photon",
       "version": "1.0.0"
     },
     "clock": {
       "device": "Dev1",
       "pixel_counter": "ctr0",
       "line_counter": "ctr1",
       "frame_counter": "ctr2",
       "timebase": "80MHz",
       "pixel_count": 512,
       "line_count": 512
     },
     "scanner": {
       "type": "galvo",
       "device": "Dev1",
       "x_channel": "ao0",
       "y_channel": "ao1",
       "scan_range": 5.0,
       "frequency": 1.5,
       "bidirectional": true
     },
     "detector": {
       "type": "qt",
       "device": "Dev2",
       "channels": [0, 1],
       "sample_rate": 1000000,
       "trigger_mode": "external",
       "segment_size": 524288
     },
     "lasers": [
       {
         "name": "920nm",
         "type": "generic",
         "wavelength": 920,
         "max_power": 2000,
         "min_power": 0,
         "enable_channel": "port0/line0",
         "power_channel": "ao2"
       },
       {
         "name": "1064nm",
         "type": "generic",
         "wavelength": 1064,
         "max_power": 5000,
         "min_power": 0,
         "enable_channel": "port0/line1",
         "power_channel": "ao3"
       }
     ],
     "imaging": {
       "default_fps": 1.5,
       "default_width": 512,
       "default_height": 512,
       "default_channels": 1,
       "pixel_size_um": 0.5
     },
     "pipeline": {
       "gpu_enabled": true,
       "buffer_count": 10,
       "stages": ["descan", "intensity_map"]
     }
   }

Loading Configuration
---------------------

.. code-block:: cpp

   #include "app/MicroscopeController.h"
   #include <fstream>
   #include <nlohmann/json.hpp>

   using json = nlohmann::json;

   int main() {
       // Load config file
       std::ifstream file("helab_config.json");
       json config = json::parse(file);

       // Create controller
       auto factory = std::make_shared<HardwareFactory>();
       MicroscopeController controller(factory);

       // Apply configuration
       controller.loadConfiguration(config);

       return 0;
   }

Micro-Manager Properties
------------------------

Properties can be set through Micro-Manager:

Hub Properties
~~~~~~~~~~~~~~

+-------------------+----------+---------+------------------+
| Property          | Type     | Default | Description      |
+===================+==========+=========+==================+
| DeviceName        | String   | Dev1    | NI-DAQ device    |
+-------------------+----------+---------+------------------+
| ScanType          | Enum     | Galvo   | Scan mode        |
+-------------------+----------+---------+------------------+
| ConfigFile        | String   | ""      | Config file path |
+-------------------+----------+---------+------------------+

Camera Properties
~~~~~~~~~~~~~~~~~

+-------------------+----------+---------+------------------+
| Property          | Type     | Default | Description      |
+===================+==========+=========+==================+
| FPS               | Float    | 1.5     | Frame rate (Hz)  |
+-------------------+----------+---------+------------------+
| Width             | Integer  | 512     | Image width      |
+-------------------+----------+---------+------------------+
| Height            | Integer  | 512     | Image height     |
+-------------------+----------+---------+------------------+
| Binning           | Integer  | 1       | Binning factor   |
+-------------------+----------+---------+------------------+
| PixelSizeUm       | Float    | 0.5     | Pixel size (µm)  |
+-------------------+----------+---------+------------------+
| OffsetX           | Integer  | 0       | ROI X offset     |
+-------------------+----------+---------+------------------+
| OffsetY           | Integer  | 0       | ROI Y offset     |
+-------------------+----------+---------+------------------+

Laser Properties
~~~~~~~~~~~~~~~~

+-------------------+----------+---------+------------------+
| Property          | Type     | Default | Description      |
+===================+==========+=========+==================+
| Power             | Float    | 0       | Power (mW)       |
+-------------------+----------+---------+------------------+
| Enable            | Boolean  | Off     | On/Off           |
+-------------------+----------+---------+------------------+
| Wavelength        | Float    | -       | Wavelength (nm)  |
+-------------------+----------+---------+------------------+
| MaxPower          | Float    | -       | Max power (mW)   |
+-------------------+----------+---------+------------------+

Environment Variables
---------------------

.. list-table::
   :header-rows: 1

   * - Variable
     - Description
   * - ``HELAB_CONFIG_PATH``
     - Path to configuration file
   * - ``HELAB_LOG_LEVEL``
     - Log level (debug, info, warn, error)
   * - ``HELAB_LOG_FILE``
     - Log file path
   * - ``CUDA_VISIBLE_DEVICES``
     - GPU device selection

Runtime Configuration
---------------------

Some settings can be changed during operation:

.. code-block:: cpp

   // Change frame rate
   ImagingConfig config = controller->imagingConfig();
   config.fps = 2.0;
   controller->setImagingConfig(config);

   // Change ROI
   ROI roi{100, 100, 256, 256};
   controller->setROI(roi);

   // Change laser power
   controller->setLaserPower("920nm", 150.0);

Persistent Settings
-------------------

Save current settings:

.. code-block:: cpp

   // Save to file
   controller->saveConfiguration("current_config.json");

   // Load saved settings
   controller->loadConfiguration("current_config.json");

Configuration Validation
------------------------

Invalid configurations are rejected:

.. code-block:: cpp

   ImagingConfig config;
   config.fps = -1.0;  // Invalid

   auto result = controller->setImagingConfig(config);
   if (!result) {
       std::cerr << "Invalid config: " << result.error().message << std::endl;
   }

Validation Rules
~~~~~~~~~~~~~~~~

+-------------------+---------------------------+
| Parameter         | Valid Range               |
+===================+===========================+
| FPS               | 0.1 - 30.0                |
+-------------------+---------------------------+
| Width             | 64 - 2048 (multiple of 16)|
+-------------------+---------------------------+
| Height            | 64 - 2048 (multiple of 16)|
+-------------------+---------------------------+
| Binning           | 1, 2, 4                   |
+-------------------+---------------------------+
| Laser Power       | 0 - maxPower              |
+-------------------+---------------------------+

Default Configuration
---------------------

If no configuration file is provided, defaults are used:

.. code-block:: cpp

   // Default configuration
   ImagingConfig defaults;
   defaults.fps = 1.5;
   defaults.width = 512;
   defaults.height = 512;
   defaults.channels = 1;
   defaults.pixelSizeUm = 0.5;
