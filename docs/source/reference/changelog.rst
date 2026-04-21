Changelog
=========

All notable changes to this project will be documented in this file.

The format is based on `Keep a Changelog <https://keepachangelog.com/en/1.0.0/>`_,
and this project adheres to `Semantic Versioning <https://semver.org/spec/v2.0.0.html>`_.

[1.0.0] - 2024-01-01
--------------------

Added
~~~~~

- Initial release of Helab Microscope device adapter
- Hardware Abstraction Layer with interfaces:

  - IClock for clock generation
  - IScanner for scanner control
  - IDetector for data acquisition
  - ILaser for laser control
  - IAdaptiveOptics for adaptive optics

- Hardware implementations:

  - NidaqClock using NI-DAQmx
  - GalvoScanner for galvanometer control
  - QtDetector for QT12136DC acquisition card
  - GenericLaser for multi-wavelength support

- Application layer:

  - MicroscopeController for business logic coordination
  - ImagePipeline for image processing

- Micro-Manager adapter layer:

  - HelabMicroscopeHub device
  - HelabCamera device
  - HelabLaserDevice for laser control

- Multi-laser support (920nm, 1064nm, extensible)
- GPU-accelerated image reconstruction with CUDA
- Comprehensive unit tests with GoogleTest
- CMake build system with cross-platform support
- Documentation with Sphinx and Doxygen

[0.9.0] - 2023-12-01
--------------------

Added
~~~~~

- Beta release for internal testing
- Basic imaging functionality
- Single laser support
- Initial Micro-Manager integration

[0.1.0] - 2023-06-01
--------------------

Added
~~~~~

- Project initialization
- Basic project structure
- Initial hardware abstraction design

Version History
---------------

.. code-block:: text

   1.0.0   ─── Current stable release
     │
   0.9.0   ─── Beta release
     │
   0.1.0   ─── Initial development

Upcoming Features
-----------------

The following features are planned for future releases:

1.1.0
~~~~~

- Resonance scanner support
- Multi-channel imaging
- Improved error recovery

1.2.0
~~~~~

- Adaptive optics integration
- Real-time image processing
- Extended laser support

2.0.0
~~~~~

- Python API bindings
- Remote control interface
- Plugin architecture
