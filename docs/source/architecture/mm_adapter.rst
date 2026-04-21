Micro-Manager Adapter Layer
===========================

The adapter layer implements Micro-Manager device interfaces.

Adapter Classes
---------------

HelabMicroscopeHub
~~~~~~~~~~~~~~~~~~

The hub device manages all child devices.

.. doxygenclass:: HelabMicroscopeHub
   :members:
   :protected-members:
   :undoc-members:

HelabCamera
~~~~~~~~~~~

The camera device provides imaging interface.

.. doxygenclass:: HelabCamera
   :members:
   :protected-members:
   :undoc-members:

HelabLaserDevice
~~~~~~~~~~~~~~~~

The generic device for laser control.

.. doxygenclass:: HelabLaserDevice
   :members:
   :protected-members:
   :undoc-members:

Device Registration
-------------------

Devices are registered in the module entry point:

.. code-block:: cpp

   MODULE_API void InitializeModuleData() {
       // Register hub
       RegisterDevice("HelabMicroscopeHub", MM::HubDevice,
           "Helab Two-Photon Microscope Hub");

       // Register camera
       RegisterDevice("HelabCamera", MM::CameraDevice,
           "Helab Two-Photon Camera");

       // Register lasers
       RegisterDevice("HelabLaser_920nm", MM::GenericDevice,
           "920nm Laser Control");
       RegisterDevice("HelabLaser_1064nm", MM::GenericDevice,
           "1064nm Laser Control");
   }

Device Creation
---------------

.. code-block:: cpp

   MODULE_API MM::Device* CreateDevice(const char* deviceName) {
       if (deviceName == nullptr) return nullptr;

       if (strcmp(deviceName, "HelabMicroscopeHub") == 0) {
           return new HelabMicroscopeHub;
       }
       if (strcmp(deviceName, "HelabCamera") == 0) {
           return new HelabCamera;
       }
       if (strcmp(deviceName, "HelabLaser_920nm") == 0) {
           return new HelabLaserDevice("920nm");
       }
       if (strcmp(deviceName, "HelabLaser_1064nm") == 0) {
           return new HelabLaserDevice("1064nm");
       }

       return nullptr;  // Unknown device
   }

Properties
----------

Hub Properties
~~~~~~~~~~~~~~

+-------------------+----------+------------------+
| Property          | Type     | Description      |
+===================+==========+==================+
| DeviceName        | String   | NI-DAQ device    |
+-------------------+----------+------------------+
| ScanType          | Enum     | Galvo/Resonance  |
+-------------------+----------+------------------+

Camera Properties
~~~~~~~~~~~~~~~~~

+-------------------+----------+------------------+
| Property          | Type     | Description      |
+===================+==========+==================+
| FPS               | Float    | Frame rate (Hz)  |
+-------------------+----------+------------------+
| Width             | Integer  | Image width      |
+-------------------+----------+------------------+
| Height            | Integer  | Image height     |
+-------------------+----------+------------------+
| Binning           | Integer  | Binning factor   |
+-------------------+----------+------------------+
| Exposure          | Float    | Exposure time    |
+-------------------+----------+------------------+
| Gain              | Float    | PMT gain         |
+-------------------+----------+------------------+
| OffsetX           | Integer  | ROI X offset     |
+-------------------+----------+------------------+
| OffsetY           | Integer  | ROI Y offset     |
+-------------------+----------+------------------+

Laser Properties
~~~~~~~~~~~~~~~~

+-------------------+----------+------------------+
| Property          | Type     | Description      |
+===================+==========+==================+
| Power             | Float    | Power (mW)       |
+-------------------+----------+------------------+
| Enable            | Boolean  | On/Off           |
+-------------------+----------+------------------+
| Wavelength        | Float    | Wavelength (nm)  |
+-------------------+----------+------------------+

Property Callbacks
------------------

Properties use callbacks for read/write operations:

.. code-block:: cpp

   int HelabCamera::OnFps(MM::PropertyBase* pProp, MM::ActionType eAct) {
       if (eAct == MM::BeforeGet) {
           // Read current value
           pProp->Set(controller_->imagingConfig().fps);
       } else if (eAct == MM::AfterSet) {
           // Write new value
           double fps;
           pProp->Get(fps);

           ImagingConfig config = controller_->imagingConfig();
           config.fps = fps;
           auto result = controller_->setImagingConfig(config);

           if (!result) {
               return DEVICE_ERR;
           }
       }
       return DEVICE_OK;
   }

Device Dependencies
-------------------

The camera depends on the hub:

.. code-block:: cpp

   int HelabCamera::Initialize() {
       // Get parent hub
       hub_ = static_cast<HelabMicroscopeHub*>(
           GetParentHub());

       if (!hub_) {
           return DEVICE_ERR;
       }

       // Get controller from hub
       controller_ = hub_->getController();

       return DEVICE_OK;
   }

Sequence Acquisition
--------------------

For continuous imaging:

.. code-block:: cpp

   int HelabCamera::StartSequenceAcquisition(
       long numImages, double interval_ms, bool stopOnOverflow) {

       // Configure for sequence
       sequenceCount_ = numImages;
       sequenceInterval_ = interval_ms;

       // Start continuous imaging
       controller_->setImageCallback([this](const auto& frame, auto ts) {
           this->OnFrameReceived(frame, ts);
       });

       return controller_->startImaging() ? DEVICE_OK : DEVICE_ERR;
   }

   int HelabCamera::StopSequenceAcquisition() {
       return controller_->stopImaging() ? DEVICE_OK : DEVICE_ERR;
   }

Error Mapping
-------------

Map internal errors to MM error codes:

.. code-block:: cpp

   int mapError(const HardwareError& err) {
       switch (err.code) {
           case 0:  return DEVICE_OK;
           case -1: return DEVICE_NOT_CONNECTED;
           case -2: return DEVICE_INVALID_PROPERTY_VALUE;
           case -3: return DEVICE_SERIAL_COMMAND_FAILED;
           case -4: return DEVICE_SERIAL_TIMEOUT;
           default: return DEVICE_ERR;
       }
   }
