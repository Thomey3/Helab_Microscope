Hardware Setup
==============

This guide covers the physical hardware setup and connections.

Hardware Components
-------------------

.. figure:: _static/hardware_connections.svg
   :alt: Hardware Connections
   :width: 100%

   Hardware connection diagram for the two-photon microscope.

Connection Overview
-------------------

+-------------------+-------------------+-------------------+
| Component         | Connection        | Port/Channel      |
+===================+===================+===================+
| NI-DAQ PCIe-6363  | PCIe slot         | Dev1              |
+-------------------+-------------------+-------------------+
| QT12136DC         | PCIe slot         | Dev2              |
+-------------------+-------------------+-------------------+
| X-Galvo           | NI-DAQ AO         | Dev1/ao0          |
+-------------------+-------------------+-------------------+
| Y-Galvo           | NI-DAQ AO         | Dev1/ao1          |
+-------------------+-------------------+-------------------+
| PMT Signal        | QT Card AI        | Dev2/ai0          |
+-------------------+-------------------+-------------------+
| Pixel Clock       | NI-DAQ Counter    | Dev1/ctr0         |
+-------------------+-------------------+-------------------+
| Line Clock        | NI-DAQ Counter    | Dev1/ctr1         |
+-------------------+-------------------+-------------------+

NI-DAQ Connections
------------------

Analog Outputs (Galvo Control)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------+-------------+------------------+
| NI-DAQ Terminal | Signal      | Galvo Terminal   |
+=================+=============+==================+
| Dev1/ao0        | X waveform  | X-Galvo Input    |
+-----------------+-------------+------------------+
| Dev1/ao1        | Y waveform  | Y-Galvo Input    |
+-----------------+-------------+------------------+

.. note::

   Galvo input range is ±5V. The scanner generates sawtooth/triangle waveforms
   within this range.

Counter Outputs (Clock Generation)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------+----------------+--------------------+
| NI-DAQ Terminal | Signal         | Destination        |
+=================+================+==================:+
| Dev1/ctr0       | Pixel clock    | QT Card trigger    |
+-----------------+----------------+--------------------+
| Dev1/ctr1       | Line clock     | Frame sync         |
+-----------------+----------------+--------------------+
| Dev1/PFI0       | Frame trigger  | External trigger   |
+-----------------+----------------+--------------------+

Digital I/O (Laser Control)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------+----------------+------------------+
| NI-DAQ Terminal | Signal         | Laser Terminal   |
+=================+================+==================+
| Dev1/port0/line0| 920nm Enable   | Laser 1 Enable   |
+-----------------+----------------+------------------+
| Dev1/port0/line1| 1064nm Enable  | Laser 2 Enable   |
+-----------------+----------------+------------------+

QT Card Connections
-------------------

Analog Inputs (PMT Signals)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-----------------+-------------+------------------+
| QT Terminal     | Signal      | PMT Output       |
+=================+=============+==================+
| Dev2/ai0        | Channel 0   | PMT 1 (Green)    |
+-----------------+-------------+------------------+
| Dev2/ai1        | Channel 1   | PMT 2 (Red)      |
+-----------------+-------------+------------------+
| Dev2/ai2        | Channel 2   | PMT 3 (Blue)     |
+-----------------+-------------+------------------+
| Dev2/ai3        | Channel 3   | PMT 4 (Backup)   |
+-----------------+-------------+------------------+

Trigger Input
~~~~~~~~~~~~~

+-----------------+------------------+
| QT Terminal     | Signal           |
+=================+==================+
| Dev2/PFI0       | Pixel clock in   |
+-----------------+------------------+

Laser Connections
-----------------

920nm Ti:Sapphire Laser
~~~~~~~~~~~~~~~~~~~~~~~

+------------------+------------------+
| Connection       | Specification    |
+==================+==================+
| Power Control    | 0-5V analog      |
+------------------+------------------+
| Enable Signal    | TTL (5V = ON)    |
+------------------+------------------+
| Max Power        | 2000 mW          |
+------------------+------------------+

1064nm Nd:YAG Laser
~~~~~~~~~~~~~~~~~~~

+------------------+------------------+
| Connection       | Specification    |
+==================+==================+
| Power Control    | 0-5V analog      |
+------------------+------------------+
| Enable Signal    | TTL (5V = ON)    |
+------------------+------------------+
| Max Power        | 5000 mW          |
+------------------+------------------+

Configuration File
------------------

Create a hardware configuration file ``helab_config.json``:

.. code-block:: json

   {
     "clock": {
       "device": "Dev1",
       "pixel_counter": "ctr0",
       "line_counter": "ctr1",
       "timebase": "80MHz",
       "pixel_count": 512,
       "line_count": 512
     },
     "scanner": {
       "device": "Dev1",
       "x_channel": "ao0",
       "y_channel": "ao1",
       "scan_range": 5.0,
       "frequency": 1.5
     },
     "detector": {
       "device": "Dev2",
       "channels": [0, 1],
       "sample_rate": 1000000,
       "trigger_mode": "external"
     },
     "lasers": [
       {
         "name": "920nm",
         "wavelength": 920,
         "max_power": 2000,
         "enable_channel": "port0/line0",
         "power_channel": "ao2"
       },
       {
         "name": "1064nm",
         "wavelength": 1064,
         "max_power": 5000,
         "enable_channel": "port0/line1",
         "power_channel": "ao3"
       }
     ]
   }

Safety Considerations
---------------------

.. warning::

   - **Never look directly at laser beams** - Use appropriate eye protection
   - **Ensure proper interlocks** - Laser should shut off when enclosure opens
   - **Check power levels** - Start with minimum power and increase gradually
   - **Monitor temperatures** - Ensure cooling systems are operational

Laser Safety Checklist
~~~~~~~~~~~~~~~~~~~~~~

Before operating the microscope:

.. code-block:: text

   [ ] Eye protection available and worn
   [ ] Laser interlocks functional
   [ ] Beam path enclosed
   [ ] Warning signs posted
   [ ] Emergency stop accessible
   [ ] Cooling systems running
   [ ] Fire extinguisher nearby

Power-On Sequence
-----------------

1. Turn on cooling systems (wait 5 minutes)
2. Enable interlocks
3. Power on NI-DAQ card
4. Power on QT card
5. Power on PMTs (with low voltage first)
6. Enable lasers (at minimum power)
7. Start acquisition software

Power-Off Sequence
------------------

1. Stop acquisition
2. Disable lasers
3. Turn off PMTs
4. Power off QT card
5. Power off NI-DAQ card
6. Disable interlocks
7. Turn off cooling systems
