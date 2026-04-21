Building from Source
====================

This guide covers building the Helab Microscope from source.

Prerequisites
-------------

Compiler Requirements
~~~~~~~~~~~~~~~~~~~~~

+-----------------+------------------+
| Platform        | Compiler         |
+=================+==================+
| Windows         | MSVC 2022+       |
+-----------------+------------------+
| Linux           | GCC 12+ / Clang 15+ |
+-----------------+------------------+

C++ Standard: **C++17** or later

Required Tools
~~~~~~~~~~~~~~

- **CMake** 3.16 or later
- **Git** for cloning
- **Make** or **Ninja** (Linux)

Windows Build
-------------

Visual Studio (Recommended)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. **Open Developer Command Prompt**:

   .. code-block:: powershell

      # From Start Menu, open:
      # "x64 Native Tools Command Prompt for VS 2022"

2. **Clone and configure**:

   .. code-block:: powershell

      git clone https://github.com/Thomey3/Helab_Microscope.git
      cd Helab_Microscope
      mkdir build && cd build
      cmake .. -A x64

3. **Build**:

   .. code-block:: powershell

      cmake --build . --config Release

4. **Install**:

   .. code-block:: powershell

      cmake --install . --prefix "C:\Program Files\Micro-Manager-2.0"

Visual Studio IDE
~~~~~~~~~~~~~~~~~

1. Open ``Helab_Microscope.sln`` in Visual Studio
2. Select ``x64`` platform
3. Select ``Release`` configuration
4. Build → Build Solution

CMake GUI
~~~~~~~~~

1. Open CMake GUI
2. Set source directory to repository root
3. Set build directory to ``build``
4. Click **Configure**
5. Select ``Visual Studio 17 2022`` generator
6. Click **Generate**
7. Click **Open Project**

Linux Build
-----------

Standard Build
~~~~~~~~~~~~~~

.. code-block:: bash

   git clone https://github.com/Thomey3/Helab_Microscope.git
   cd Helab_Microscope
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)

With Options
~~~~~~~~~~~~

.. code-block:: bash

   cmake .. \
       -DCMAKE_BUILD_TYPE=Release \
       -DUSE_NI_DAQMX=ON \
       -DUSE_CUDA=ON \
       -DBUILD_TESTS=ON \
       -DCMAKE_INSTALL_PREFIX=/usr/local

   make -j$(nproc)
   sudo make install

Ninja Build (Faster)
~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
   ninja

Build Options
-------------

+------------------------+----------+----------------------------------------+
| Option                 | Default  | Description                            |
+========================+==========+========================================+
| ``USE_NI_DAQMX``       | ON       | Enable NI-DAQmx support                |
+------------------------+----------+----------------------------------------+
| ``USE_CUDA``           | ON       | Enable CUDA image reconstruction       |
+------------------------+----------+----------------------------------------+
| ``BUILD_SHARED_LIBS``  | ON       | Build shared library                   |
+------------------------+----------+----------------------------------------+
| ``BUILD_TESTS``        | ON       | Build unit tests                       |
+------------------------+----------+----------------------------------------+
| ``CMAKE_BUILD_TYPE``   | Release  | Build type (Debug/Release)             |
+------------------------+----------+----------------------------------------+

Debug Build
~~~~~~~~~~~

.. code-block:: bash

   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make -j$(nproc)

Release Build
~~~~~~~~~~~~~

.. code-block:: bash

   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)

Dependency Paths
----------------

If dependencies are in non-standard locations:

NI-DAQmx
~~~~~~~~

.. code-block:: bash

   cmake .. \
       -DNIDAQMX_INCLUDE_DIR=/custom/path/include \
       -DNIDAQMX_LIBRARY_DIR=/custom/path/lib

CUDA
~~~~

.. code-block:: bash

   cmake .. \
       -DCUDA_INCLUDE_DIR=/custom/cuda/include \
       -DCUDA_LIB_DIR=/custom/cuda/lib

MMDevice Headers
~~~~~~~~~~~~~~~~

.. code-block:: bash

   cmake .. \
       -DMMDEVICE_INCLUDE_DIR=/path/to/MMDevice

Output Files
------------

Windows
~~~~~~~

.. code-block:: text

   build/Release/
   ├── mmgr_dal_Helab_Microscope.dll  # Device adapter
   ├── mmgr_dal_Helab_Microscope.lib  # Import library
   └── mmgr_dal_Helab_Microscope.pdb  # Debug symbols

Linux
~~~~~

.. code-block:: text

   build/
   ├── libmmgr_dal_Helab_Microscope.so  # Device adapter
   └── libmmgr_dal_Helab_Microscope.a   # Static library (optional)

Installation
------------

Windows
~~~~~~~

.. code-block:: powershell

   # Copy to Micro-Manager
   copy build\Release\mmgr_dal_Helab_Microscope.dll `
        "C:\Program Files\Micro-Manager-2.0\"

   # Or use CMake install
   cmake --install build --prefix "C:\Program Files\Micro-Manager-2.0"

Linux
~~~~~

.. code-block:: bash

   # Install system-wide
   sudo cmake --install build

   # Or copy to Micro-Manager
   cp build/libmmgr_dal_Helab_Microscope.so \
      /usr/local/lib/micro-manager/

Troubleshooting
---------------

CMake Not Found
~~~~~~~~~~~~~~~

.. code-block:: bash

   # Ubuntu/Debian
   sudo apt install cmake

   # macOS
   brew install cmake

Compiler Not Found
~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Ubuntu/Debian
   sudo apt install build-essential g++-12

   # Set compiler
   cmake .. -DCMAKE_CXX_COMPILER=g++-12

NI-DAQmx Not Found
~~~~~~~~~~~~~~~~~~

.. code-block:: text

   CMake Error: Could not find NIDAQmx.h

Solutions:

1. Install NI-DAQmx driver
2. Or specify path:

   .. code-block:: bash

      cmake .. -DNIDAQMX_INCLUDE_DIR=/path/to/include

CUDA Not Found
~~~~~~~~~~~~~~

.. code-block:: text

   CMake Warning: CUDA not found

Solutions:

1. Install CUDA Toolkit
2. Or disable CUDA:

   .. code-block:: bash

      cmake .. -DUSE_CUDA=OFF

Link Errors
~~~~~~~~~~~

.. code-block:: text

   undefined reference to `DAQmxCreateTask`

Solutions:

1. Check NI-DAQmx library path
2. Verify library architecture (x64 vs x86)
3. Check library is linked:

   .. code-block:: bash

      ldd libmmgr_dal_Helab_Microscope.so

Clean Build
~~~~~~~~~~~

.. code-block:: bash

   # Remove build directory
   rm -rf build
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
