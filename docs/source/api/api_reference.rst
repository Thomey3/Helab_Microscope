API Reference
=============

This section contains the complete API reference for all classes and functions.

.. note::

   The API documentation is automatically generated from the source code using
   Doxygen, Breathe, and Exhale.

.. toctree::
   :maxdepth: 3

   api/namespace_helab
   api/class_index
   api/file_index

Namespaces
----------

.. doxygennamespace:: helab
   :members:
   :undoc-members:

Classes by Category
-------------------

Hardware Abstraction Layer
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Class
     - Description
   * - :cpp:class:`IClock`
     - Clock generation interface
   * - :cpp:class:`IScanner`
     - Scanner control interface
   * - :cpp:class:`IDetector`
     - Data acquisition interface
   * - :cpp:class:`ILaser`
     - Laser control interface
   * - :cpp:class:`IAdaptiveOptics`
     - Adaptive optics interface
   * - :cpp:class:`IHardwareFactory`
     - Hardware factory interface

Hardware Implementations
~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Class
     - Description
   * - :cpp:class:`NidaqClock`
     - NI-DAQ clock implementation
   * - :cpp:class:`GalvoScanner`
     - Galvo scanner implementation
   * - :cpp:class:`QtDetector`
     - QT card detector implementation
   * - :cpp:class:`GenericLaser`
     - Generic laser implementation

Application Layer
~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Class
     - Description
   * - :cpp:class:`MicroscopeController`
     - Main controller class
   * - :cpp:class:`ImagePipeline`
     - Image processing pipeline

Micro-Manager Adapter
~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Class
     - Description
   * - :cpp:class:`HelabMicroscopeHub`
     - Hub device
   * - :cpp:class:`HelabCamera`
     - Camera device
   * - :cpp:class:`HelabLaserDevice`
     - Laser device

Configuration Structures
------------------------

.. list-table::
   :header-rows: 1

   * - Structure
     - Description
   * - :cpp:struct:`ClockConfig`
     - Clock configuration
   * - :cpp:struct:`ScannerConfig`
     - Scanner configuration
   * - :cpp:struct:`DetectorConfig`
     - Detector configuration
   * - :cpp:struct:`LaserConfig`
     - Laser configuration
   * - :cpp:struct:`ImagingConfig`
     - Imaging configuration
   * - :cpp:struct:`ROI`
     - Region of interest
   * - :cpp:struct:`ScanPattern`
     - Scan pattern settings

Enumerations
------------

.. list-table::
   :header-rows: 1

   * - Enum
     - Description
   * - :cpp:enum:`MicroscopeState`
     - Controller state
   * - :cpp:enum:`ScanPattern::Type`
     - Scan type (Galvo, Resonance)

Type Aliases
------------

.. list-table::
   :header-rows: 1

   * - Type
     - Description
   * - :cpp:type:`Result<T>`
     - std::expected<T, HardwareError>
   * - :cpp:type:`HardwareError`
     - Error structure with code and message

File Reference
--------------

Header Files
~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - File
     - Description
   * - ``Device/include/hal/IHardware.h``
     - Core interface definitions
   * - ``Device/include/hal/HardwareFactory.h``
     - Hardware factory
   * - ``Device/include/app/MicroscopeController.h``
     - Main controller
   * - ``Device/include/pipeline/ImagePipeline.h``
     - Image processing
   * - ``Device/include/mmadapter/HelabDevices.h``
     - MM adapter classes
