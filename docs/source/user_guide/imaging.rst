Imaging Guide
=============

This guide covers the imaging capabilities of the Helab Microscope.

Imaging Modes
-------------

Single Snap
~~~~~~~~~~~

Acquire a single image:

.. code-block:: python

   # Python (pycro-manager)
   mmc.snapImage()
   image = mmc.getImage()

.. code-block:: cpp

   // C++
   std::vector<uint8_t> frame;
   controller->acquireSingleFrame(frame);

Live Mode
~~~~~~~~~

Continuous imaging:

.. code-block:: python

   # Start live mode
   mmc.startContinuousSequenceAcquisition(0)  # 0 = maximum speed

   # Get images as they arrive
   while True:
       if mmc.getRemainingImageCount() > 0:
           image = mmc.popNextImage()
           # Process image

   # Stop
   mmc.stopSequenceAcquisition()

.. code-block:: cpp

   // Set callback
   controller->setImageCallback([](const auto& frame, auto ts) {
       // Process frame
   });

   // Start
   controller->startImaging();

   // ... running ...

   // Stop
   controller->stopImaging();

Sequence Acquisition
~~~~~~~~~~~~~~~~~~~~

Acquire a fixed number of images:

.. code-block:: python

   num_images = 100
   interval_ms = 100  # 100ms between frames

   mmc.startSequenceAcquisition(num_images, interval_ms, False)

   # Wait for completion
   while mmc.isSequenceRunning():
       time.sleep(0.1)

   # Retrieve all images
   images = []
   while mmc.getRemainingImageCount() > 0:
       images.append(mmc.popNextImage())

Imaging Parameters
------------------

Frame Rate
~~~~~~~~~~

Set the imaging frame rate:

.. code-block:: python

   # Set to 1.5 Hz (max for galvo)
   mmc.setProperty("HelabCamera", "FPS", 1.5)

.. note::

   Maximum frame rate depends on scan type:
   - Galvo: 1.5 Hz
   - Resonance: 30+ Hz (future)

Image Size
~~~~~~~~~~

Set image dimensions:

.. code-block:: python

   # 512 x 512 pixels
   mmc.setProperty("HelabCamera", "Width", 512)
   mmc.setProperty("HelabCamera", "Height", 512)

   # Or 1024 x 1024
   mmc.setProperty("HelabCamera", "Width", 1024)
   mmc.setProperty("HelabCamera", "Height", 1024)

Supported sizes: 256, 512, 1024

Pixel Dwell Time
~~~~~~~~~~~~~~~~

The time spent on each pixel:

.. code-block:: text

   dwell_time = 1 / (fps * width * height)

   Example: 1.5 Hz, 512x512
   dwell_time = 1 / (1.5 * 512 * 512) = 2.54 µs

Binning
~~~~~~~

Enable pixel binning for higher sensitivity:

.. code-block:: python

   mmc.setProperty("HelabCamera", "Binning", 2)  # 2x2 binning

Binning reduces resolution but increases signal:

+--------+-------------+----------------+
| Binning| Resolution  | Signal Gain    |
+========+=============+================+
| 1      | Full        | 1x             |
+--------+-------------+----------------+
| 2      | Half        | 4x             |
+--------+-------------+----------------+
| 4      | Quarter     | 16x            |
+--------+-------------+----------------+

Region of Interest (ROI)
------------------------

Set a sub-region for faster acquisition:

.. code-block:: python

   # Set ROI
   mmc.setROI(100, 100, 256, 256)  # x, y, width, height

   # Get current ROI
   x, y, w, h = mmc.getROI()

   # Clear ROI (full frame)
   mmc.clearROI()

.. code-block:: cpp

   ROI roi;
   roi.x = 100;
   roi.y = 100;
   roi.width = 256;
   roi.height = 256;
   controller->setROI(roi);

Multi-Channel Imaging
---------------------

For multiple detection channels:

.. code-block:: python

   # Enable channels
   mmc.setProperty("HelabCamera", "Channel-0", "On")   # Green
   mmc.setProperty("HelabCamera", "Channel-1", "On")   # Red

   # Acquire multi-channel
   mmc.snapImage()
   green = mmc.getImage()  # Channel 0
   red = mmc.getImage(1)   # Channel 1

Z-Stack Acquisition
-------------------

Acquire images at multiple Z positions:

.. code-block:: python

   import numpy as np

   z_start = 0      # µm
   z_end = 100      # µm
   z_step = 5       # µm
   z_positions = np.arange(z_start, z_end, z_step)

   images = []
   for z in z_positions:
       mmc.setPosition(z)
       mmc.waitForDevice("ZStage")
       mmc.snapImage()
       images.append((z, mmc.getImage()))

Time-Lapse Acquisition
----------------------

Acquire images over time:

.. code-block:: python

   import time

   duration_s = 60      # Total duration
   interval_s = 5       # Interval between frames
   num_frames = duration_s // interval_s

   images = []
   for i in range(num_frames):
       start_time = time.time()

       mmc.snapImage()
       images.append(mmc.getImage())

       # Wait for next interval
       elapsed = time.time() - start_time
       if elapsed < interval_s:
           time.sleep(interval_s - elapsed)

Performance Optimization
-----------------------

GPU Acceleration
~~~~~~~~~~~~~~~~

Enable CUDA for faster processing:

.. code-block:: cpp

   pipeline->enableGpuAcceleration(true);

Buffer Size
~~~~~~~~~~~

Adjust buffer count for smooth acquisition:

.. code-block:: cpp

   // More buffers = smoother but more memory
   BufferPool pool(20, bufferSize);  // 20 buffers

Scan Pattern
~~~~~~~~~~~~

Choose optimal scan pattern:

.. code-block:: cpp

   // Bidirectional scanning (faster)
   ScanPattern pattern;
   pattern.bidirectional = true;
   scanner->setScanPattern(pattern);

Troubleshooting
---------------

Low Frame Rate
~~~~~~~~~~~~~~

Possible causes:

1. **Large image size**: Reduce width/height
2. **No GPU**: Enable CUDA
3. **Slow disk**: Use RAM disk for saving
4. **USB bandwidth**: Use PCIe devices

Image Artifacts
~~~~~~~~~~~~~~~

+-------------------+------------------------+
| Artifact          | Possible Cause         |
+===================+========================+
| Horizontal lines  | Sync issue with scanner|
+-------------------+------------------------+
| Noise             | Low PMT gain, no avg   |
+-------------------+------------------------+
| Distortion        | Incorrect descan       |
+-------------------+------------------------+
| Missing pixels    | Buffer overflow        |
+-------------------+------------------------+
