Laser Control
=============

The Helab Microscope supports multiple laser wavelengths.

Available Lasers
----------------

+----------+-------------+-------------+
| Name     | Wavelength  | Max Power   |
+==========+=============+=============+
| 920nm    | 920 nm      | 2000 mW     |
+----------+-------------+-------------+
| 1064nm   | 1064 nm     | 5000 mW     |
+----------+-------------+-------------+
| 808nm    | 808 nm      | 500 mW      |
+----------+-------------+-------------+

Basic Control
-------------

Enable/Disable
~~~~~~~~~~~~~~

.. code-block:: python

   # Enable laser
   mmc.setProperty("HelabLaser_920nm", "Enable", "On")

   # Disable laser
   mmc.setProperty("HelabLaser_920nm", "Enable", "Off")

.. code-block:: cpp

   // Enable laser
   controller->enableLaser("920nm", true);

   // Disable laser
   controller->enableLaser("920nm", false);

Power Control
~~~~~~~~~~~~~

.. code-block:: python

   # Set power to 100 mW
   mmc.setProperty("HelabLaser_920nm", "Power", 100.0)

   # Get current power
   power = mmc.getProperty("HelabLaser_920nm", "Power")

.. code-block:: cpp

   // Set power
   controller->setLaserPower("920nm", 100.0);

   // Get power
   auto result = controller->getLaserPower("920nm");
   if (result) {
       double power = result.value();
   }

Query Status
~~~~~~~~~~~~

.. code-block:: python

   # Check if enabled
   enabled = mmc.getProperty("HelabLaser_920nm", "Enable") == "On"

   # Get wavelength
   wavelength = float(mmc.getProperty("HelabLaser_920nm", "Wavelength"))

.. code-block:: cpp

   // Check if enabled
   bool enabled = controller->isLaserEnabled("920nm");

   // Get wavelength
   auto result = controller->getLaserWavelength("920nm");

Multi-Laser Operation
---------------------

Sequential Imaging
~~~~~~~~~~~~~~~~~~

Use different lasers for different images:

.. code-block:: python

   # Image with 920nm
   mmc.setProperty("HelabLaser_920nm", "Enable", "On")
   mmc.setProperty("HelabLaser_1064nm", "Enable", "Off")
   mmc.snapImage()
   image_920 = mmc.getImage()

   # Image with 1064nm
   mmc.setProperty("HelabLaser_920nm", "Enable", "Off")
   mmc.setProperty("HelabLaser_1064nm", "Enable", "On")
   mmc.snapImage()
   image_1064 = mmc.getImage()

Simultaneous Imaging
~~~~~~~~~~~~~~~~~~~~

Enable multiple lasers at once:

.. code-block:: python

   # Enable both lasers
   mmc.setProperty("HelabLaser_920nm", "Enable", "On")
   mmc.setProperty("HelabLaser_1064nm", "Enable", "On")

   # Set different powers
   mmc.setProperty("HelabLaser_920nm", "Power", 100.0)
   mmc.setProperty("HelabLaser_1064nm", "Power", 50.0)

   # Acquire combined signal
   mmc.snapImage()

Power Ramping
-------------

Gradually increase power:

.. code-block:: python

   import time

   start_power = 0      # mW
   end_power = 200      # mW
   steps = 20
   delay = 0.5          # seconds

   powers = [start_power + i * (end_power - start_power) / steps
             for i in range(steps + 1)]

   for power in powers:
       mmc.setProperty("HelabLaser_920nm", "Power", power)
       time.sleep(delay)

Safety Interlocks
-----------------

.. warning::

   Always follow laser safety protocols!

Hardware Interlocks
~~~~~~~~~~~~~~~~~~~

The system checks:

1. **Enclosure closed**: Laser disabled if open
2. **Emergency stop**: Immediate shutdown
3. **Cooling status**: Disable if overheated
4. **Key switch**: Requires key to enable

Software Limits
~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Power is clamped to valid range
   controller->setLaserPower("920nm", 3000.0);  // Clamped to 2000 mW

   // Check actual power
   auto result = controller->getLaserPower("920nm");
   // result.value() == 2000.0

Warm-up Procedure
-----------------

Proper laser warm-up ensures stable output:

.. code-block:: python

   def warmup_laser(laser_name, warmup_time=300):
       """Warm up laser for stable output.

       Args:
           laser_name: Laser device name
           warmup_time: Warm-up time in seconds (default 5 min)
       """
       import time

       # Enable at low power
       mmc.setProperty(laser_name, "Power", 10.0)
       mmc.setProperty(laser_name, "Enable", "On")

       print(f"Warming up {laser_name} for {warmup_time}s...")
       time.sleep(warmup_time)

       print(f"{laser_name} ready")

   warmup_laser("HelabLaser_920nm")

Shutdown Procedure
------------------

.. code-block:: python

   def shutdown_laser(laser_name):
       """Safely shutdown laser."""
       # Ramp down power
       current_power = float(mmc.getProperty(laser_name, "Power"))
       for power in [current_power * 0.8, current_power * 0.6,
                     current_power * 0.4, current_power * 0.2, 0]:
           mmc.setProperty(laser_name, "Power", power)
           time.sleep(0.5)

       # Disable
       mmc.setProperty(laser_name, "Enable", "Off")

Adding Custom Lasers
--------------------

To add a new laser type:

1. **Implement ILaser interface**:

   .. code-block:: cpp

      class MyCustomLaser : public ILaser {
      public:
          Result<void> initialize() override;
          Result<void> setPower(double power_mW) override;
          Result<void> enable(bool enable) override;
          // ...
      };

2. **Register in factory**:

   .. code-block:: cpp

      // In HardwareFactory.cpp
      if (type == "my_custom") {
          LaserConfig config;
          config.name = "my_custom";
          config.wavelength = 800.0;
          config.maxPower = 1000.0;
          return std::make_unique<MyCustomLaser>(config);
      }

3. **Register in MM adapter**:

   .. code-block:: cpp

      // In Hub.cpp
      RegisterDevice("HelabLaser_800nm", MM::GenericDevice,
          "800nm Laser Control");

Troubleshooting
---------------

Laser Won't Enable
~~~~~~~~~~~~~~~~~~

Check:

1. Interlock status
2. Cooling system
3. Key switch
4. Power supply

Power Not Accurate
~~~~~~~~~~~~~~~~~~

Possible causes:

1. **Not warmed up**: Wait 5+ minutes
2. **Calibration needed**: Recalibrate
3. **Temperature drift**: Check cooling

Communication Error
~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   Error: Laser communication timeout

Solutions:

1. Check cable connections
2. Verify baud rate settings
3. Power cycle the laser
