Installation
============

Prerequisites
-------------

Operating System
~~~~~~~~~~~~~~~~

+----------+------------------+
| Platform | Support Status   |
+==========+==================+
| Windows  | ✅ Full support  |
+----------+------------------+
| Linux    | ✅ Supported     |
+----------+------------------+
| macOS    | ⚠️ Partial       |
+----------+------------------+

Required Software
~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Software
     - Version
     - Purpose
   * - C++ Compiler
     - C++17 or later
     - Build the adapter
   * - CMake
     - 3.16+
     - Build system
   * - Micro-Manager
     - 2.0+
     - Host application
   * - NI-DAQmx
     - 22.0+
     - DAQ card driver

Optional Dependencies
~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1

   * - Software
     - Version
     - Purpose
   * - CUDA Toolkit
     - 12.x
     - GPU image reconstruction
   * - GoogleTest
     - 1.14+
     - Unit testing
   * - Doxygen
     - 1.9+
     - Documentation generation

Driver Installation
-------------------

NI-DAQmx (Required)
~~~~~~~~~~~~~~~~~~~

Windows
^^^^^^^

1. Download from `NI Website <https://www.ni.com/en-us/support/downloads/drivers/download.ni-daqmx.html>`_
2. Run the installer
3. Reboot after installation

Linux
^^^^^

.. code-block:: bash

   # Download NI-DAQmx for Linux from NI website
   sudo dpkg -i ni-daqmx_*.deb
   sudo apt-get install -f

CUDA Toolkit (Optional)
~~~~~~~~~~~~~~~~~~~~~~~~

Windows
^^^^^^^

1. Download from `NVIDIA Website <https://developer.nvidia.com/cuda-downloads>`_
2. Run the installer with default options
3. Verify installation:

   .. code-block:: powershell

      nvcc --version

Linux
^^^^^

.. code-block:: bash

   # Ubuntu/Debian
   wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb
   sudo dpkg -i cuda-keyring_1.1-1_all.deb
   sudo apt update
   sudo apt install cuda-12-3

   # Verify
   nvcc --version

Building from Source
---------------------

Windows (Visual Studio)
~~~~~~~~~~~~~~~~~~~~~~~~

1. Open ``x64 Native Tools Command Prompt for VS 2022``

2. Clone and build:

   .. code-block:: powershell

      git clone https://github.com/Thomey3/Helab_Microscope.git
      cd Helab_Microscope
      mkdir build && cd build
      cmake .. -A x64 -DBUILD_TESTS=ON
      cmake --build . --config Release

3. Install to Micro-Manager:

   .. code-block:: powershell

      copy Release\mmgr_dal_Helab_Microscope.dll "C:\Program Files\Micro-Manager-2.0\"

Windows (CMake + MinGW)
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   git clone https://github.com/Thomey3/Helab_Microscope.git
   cd Helab_Microscope
   mkdir build && cd build
   cmake .. -G "MinGW Makefiles" -DBUILD_TESTS=ON
   cmake --build . --config Release

Linux
~~~~~

.. code-block:: bash

   git clone https://github.com/Thomey3/Helab_Microscope.git
   cd Helab_Microscope
   mkdir build && cd build
   cmake .. -DUSE_NI_DAQMX=ON -DUSE_CUDA=ON -DBUILD_TESTS=ON
   make -j$(nproc)

   # Install
   sudo make install

   # Or copy to Micro-Manager
   cp libmmgr_dal_Helab_Microscope.so /usr/local/lib/micro-manager/

Build Options
-------------

.. list-table::
   :header-rows: 1

   * - Option
     - Default
     - Description
   * - ``USE_NI_DAQMX``
     - ON
     - Enable NI-DAQmx support
   * - ``USE_CUDA``
     - ON
     - Enable CUDA image reconstruction
   * - ``BUILD_SHARED_LIBS``
     - ON
     - Build as shared library
   * - ``BUILD_TESTS``
     - ON
     - Build unit tests

Example with custom options:

.. code-block:: bash

   cmake .. -DUSE_CUDA=OFF -DBUILD_TESTS=OFF

Running Tests
-------------

.. code-block:: bash

   cd build
   ctest --output-on-failure

   # Or run directly
   ./Helab_Microscope_Tests

Troubleshooting
---------------

NI-DAQmx Not Found
~~~~~~~~~~~~~~~~~~

**Error**: ``Could not find NIDAQmx.h``

**Solution**: Install NI-DAQmx driver or specify path:

.. code-block:: bash

   cmake .. -DNIDAQMX_INCLUDE_DIR=/path/to/daqmx/include

CUDA Not Found
~~~~~~~~~~~~~~

**Error**: ``Could not find cuda_runtime.h``

**Solution**: Install CUDA Toolkit or disable:

.. code-block:: bash

   cmake .. -DUSE_CUDA=OFF

MMDevice Headers Not Found
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Error**: ``MMDevice.h not found``

**Solution**: Specify Micro-Manager source path:

.. code-block:: bash

   cmake .. -DMMDEVICE_INCLUDE_DIR=/path/to/mmCoreAndDevices/MMDevice

Permission Denied (Linux)
~~~~~~~~~~~~~~~~~~~~~~~~~

**Error**: ``Permission denied when copying library``

**Solution**: Use sudo or install to user directory:

.. code-block:: bash

   mkdir -p ~/.local/lib/micro-manager
   cp libmmgr_dal_Helab_Microscope.so ~/.local/lib/micro-manager/
