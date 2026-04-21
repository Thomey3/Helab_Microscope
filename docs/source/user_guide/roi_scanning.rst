ROI and Scanning
================

This guide covers region of interest (ROI) and scanning patterns.

Region of Interest (ROI)
------------------------

Setting ROI
~~~~~~~~~~~

.. code-block:: python

   # Set ROI: x, y, width, height
   mmc.setROI(100, 100, 256, 256)

.. code-block:: cpp

   ROI roi;
   roi.x = 100;
   roi.y = 100;
   roi.width = 256;
   roi.height = 256;
   controller->setROI(roi);

Getting ROI
~~~~~~~~~~~

.. code-block:: python

   x, y, w, h = mmc.getROI()
   print(f"ROI: ({x}, {y}) {w}x{h}")

Clearing ROI
~~~~~~~~~~~~

.. code-block:: python

   # Return to full frame
   mmc.clearROI()

ROI Benefits
~~~~~~~~~~~~

+-------------------+----------------------------------------+
| Benefit           | Description                            |
+===================+========================================+
| Faster imaging    | Fewer pixels = higher frame rate       |
+-------------------+----------------------------------------+
| Less photodamage  | Smaller illuminated area               |
+-------------------+----------------------------------------+
| Memory efficient  | Smaller images                         |
+-------------------+----------------------------------------+

ROI Limitations
~~~~~~~~~~~~~~~

- Width and height must be multiples of 16
- Minimum size: 64 x 64 pixels
- Maximum size: Full sensor size

Scan Patterns
-------------

Galvo Scanning
~~~~~~~~~~~~~~

Standard galvanometer scanning:

.. code-block:: cpp

   // Set galvo scanning
   controller->setScanType(ScanPattern::Type::Galvo);

   // Configure galvo parameters
   ScanPattern pattern;
   pattern.type = ScanPattern::Type::Galvo;
   pattern.bidirectional = true;  // Scan both directions
   pattern.fillFraction = 0.8;    // 80% active scan
   pattern.galvoAmplitude = 4.0;  // ±4V

   scanner->setScanPattern(pattern);

Galvo Characteristics
^^^^^^^^^^^^^^^^^^^^^

+-------------------+------------------+
| Parameter         | Value            |
+===================+==================+
| Max Frame Rate    | 1.5 Hz           |
+-------------------+------------------+
| Scan Range        | ±5 V             |
+-------------------+------------------+
| Linearity         | High             |
+-------------------+------------------+

Resonance Scanning (Future)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

High-speed resonance scanning:

.. code-block:: cpp

   // Set resonance scanning
   controller->setScanType(ScanPattern::Type::Resonance);

   // Configure resonance parameters
   ScanPattern pattern;
   pattern.type = ScanPattern::Type::Resonance;
   pattern.resonanceFrequency = 8000;  // 8 kHz
   pattern.bidirectional = true;

   scanner->setScanPattern(pattern);

.. note::

   Resonance scanning is planned for future release.

Resonance Characteristics
^^^^^^^^^^^^^^^^^^^^^^^^^

+-------------------+------------------+
| Parameter         | Value            |
+===================+==================+
| Max Frame Rate    | 30+ Hz           |
+-------------------+------------------+
| Frequency         | 8 kHz typical    |
+-------------------+------------------+
| Linearity         | Sinusoidal       |
+-------------------+------------------+

Bidirectional Scanning
~~~~~~~~~~~~~~~~~~~~~~

Scan in both directions for higher speed:

.. code-block:: text

   Unidirectional:
   → → → → → → → → → →
                     ← ← ← ← ← ← ← ← ← ← (flyback, no data)

   Bidirectional:
   → → → → → → → → → →
   ← ← ← ← ← ← ← ← ← ← (data collected)

Fill Fraction
~~~~~~~~~~~~~

The portion of scan used for imaging:

.. code-block:: text

   Scan line: |----====------|
             |    |    |     |
             |    |    |     └─ End
             |    |    └─ Flyback (20%)
             |    └─ Active scan (80%)
             └─ Start

   fillFraction = 0.8

Higher fill fraction = more pixels per line, but less time for flyback.

Scan Rotation
-------------

Rotate the scan orientation:

.. code-block:: cpp

   // Rotate 45 degrees
   scanner->setRotation(45.0);

   // Common rotations
   scanner->setRotation(0);    // Standard
   scanner->setRotation(90);   // Rotated
   scanner->setRotation(45);   // Diagonal

Zoom Control
------------

Adjust the scan amplitude for zoom:

.. code-block:: cpp

   // Zoom level
   double zoom = 2.0;  // 2x zoom

   // Reduce scan amplitude
   double baseAmplitude = 4.0;
   scanner->setAmplitude(baseAmplitude / zoom);

+------+----------------+------------------+
| Zoom | Scan Amplitude | Field of View    |
+======+================+==================+
| 1x   | 4.0 V          | Full             |
+------+----------------+------------------+
| 2x   | 2.0 V          | Half             |
+------+----------------+------------------+
| 4x   | 1.0 V          | Quarter          |
+------+----------------+------------------+

Point Scanning
--------------

Scan a single point:

.. code-block:: cpp

   // Move to specific position
   scanner->setPosition(0.5, 0.5);  // Center

   // Or with absolute coordinates
   scanner->setPosition(100.0, 100.0);  // x=100, y=100

Useful for:

- Spot bleaching
- Point stimulation
- Calibration

Raster vs. Random Access
------------------------

Raster Scan
~~~~~~~~~~~

Standard line-by-line scanning:

.. code-block:: text

   Line 0: → → → → → → → →
   Line 1: → → → → → → → →
   Line 2: → → → → → → → →
   ...

Random Access Scan
~~~~~~~~~~~~~~~~~~

Scan specific points in any order:

.. code-block:: cpp

   // Define points of interest
   std::vector<std::pair<double, double>> points = {
       {0.0, 0.0},   // Point 1
       {0.5, 0.5},   // Point 2
       {1.0, 1.0},   // Point 3
   };

   // Scan each point
   for (const auto& [x, y] : points) {
       scanner->setPosition(x, y);
       // Acquire data
   }

Scan Calibration
----------------

Calibrate scan coordinates:

.. code-block:: cpp

   // 1. Acquire calibration grid
   // 2. Measure known distances
   // 3. Calculate scale factors

   double pixelSizeUm = 0.5;  // µm per pixel
   controller->setPixelSize(pixelSizeUm);

Troubleshooting
---------------

Distorted Images
~~~~~~~~~~~~~~~~

Causes:

1. **Incorrect fill fraction**: Adjust for your scanner
2. **Timing mismatch**: Check clock configuration
3. **Nonlinear scanner**: Use calibration

Low Frame Rate
~~~~~~~~~~~~~~

Solutions:

1. **Reduce image size**: Smaller ROI
2. **Bidirectional scanning**: Enable if not already
3. **Increase fill fraction**: More active scan time

Scan Jitter
~~~~~~~~~~~

Causes:

1. **Clock instability**: Check NI-DAQ
2. **Mechanical resonance**: Avoid resonant frequencies
3. **Electrical noise**: Shield cables
