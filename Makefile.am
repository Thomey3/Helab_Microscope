# Makefile.am for Helab_Microscope device adapter
# Integrates with Micro-Manager autotools build system

# Device adapter library
deviceadapter_LTLIBRARIES = libmmgr_dal_Helab_Microscope.la

# Third-party directory
THIRDPARTY_DIR = $(top_srcdir)/../3rdparty

# Compiler flags
AM_CXXFLAGS = $(MMDEVAPI_CXXFLAGS) $(BOOST_CPPFLAGS)

# Include paths
AM_CPPFLAGS = \
	-I$(srcdir)/Device/include \
	-I$(THIRDPARTY_DIR)/QT12136DC/include \
	-I$(THIRDPARTY_DIR)/NationalInstruments/DAQmx/include \
	-I$(THIRDPARTY_DIR)/Alpao/include \
	$(MMDEVAPI_CPPFLAGS)

# Conditional NI-DAQmx support
if USE_NI_DAQMX
AM_CXXFLAGS += -DUSE_NI_DAQMX
libmmgr_dal_Helab_Microscope_la_LIBADD = -L$(THIRDPARTY_DIR)/NationalInstruments/DAQmx/lib -lNIDAQmx
endif

# Conditional CUDA support
if USE_CUDA
AM_CXXFLAGS += -DUSE_CUDA
AM_CPPFLAGS += -I$(THIRDPARTY_DIR)/CUDA/include
libmmgr_dal_Helab_Microscope_la_LIBADD += -L$(THIRDPARTY_DIR)/CUDA/lib -lcudart
endif

# Source files
libmmgr_dal_Helab_Microscope_la_SOURCES = \
	Hub.cpp \
	Hub.h \
	Common.h \
	Device/source/_920TwophotonCamera.cpp \
	Device/include/_920TwophotonCamera.h \
	Device/source/920CameraPropertyAct.cpp \
	Device/source/CameraThread.cpp \
	Device/source/clock.cpp \
	Device/include/clock.h \
	Device/source/ClockTask.cpp \
	Device/source/galvo.cpp \
	Device/include/galvo.h \
	Device/source/QT12136DC.cpp \
	Device/include/QT12136DC.h \
	Device/source/QT12136DCPropertyAct.cpp \
	Device/source/QT12136DCThread.cpp \
	Device/source/QTSetFunction.cpp \
	Device/include/QTSetFunction.h \
	Device/source/ImageBufferManager.cpp \
	Device/include/ImageBufferManager.h \
	Device/source/DataStream.cpp \
	Device/include/DataStream.h \
	Device/include/CommonAPI.h \
	Device/include/BufferAPI.h \
	Device/include/ImageBufferManagerAPI.h \
	Device/include/QT12136DC_datatype.h \
	Device/include/Platform.h

# NI-DAQmx sources (conditional)
if USE_NI_DAQMX
libmmgr_dal_Helab_Microscope_la_SOURCES += \
	Device/source/NIDAQ.cpp \
	Device/include/NIDAQ.h \
	Device/source/NIAnalogInputPort.cpp \
	Device/source/NIAnalogOutputPort.cpp \
	Device/source/NIDigitalOutputPort.cpp
endif

# CUDA sources (conditional)
if USE_CUDA
libmmgr_dal_Helab_Microscope_la_SOURCES += \
	Device/source/ImgReconstruction.cu \
	Device/include/ImgReconstruction.h \
	Device/include/ImgReconstruction.cuh
endif

# Library versioning
libmmgr_dal_Helab_Microscope_la_LDFLAGS = -version-info 0:0:0

# Install configuration files
deviceadapterconfdir = $(pkgdatadir)/Helab_Microscope/config
dist_deviceadapterconf_DATA = \
	Device/include/config/Interface.json

# Extra files to distribute
EXTRA_DIST = \
	CMakeLists.txt \
	README.md \
	Helab_Microscope.vcxproj \
	Helab_Microscope.vcxproj.filters

# Clean rules
CLEANFILES = *.la *.lo
DISTCLEANFILES = Makefile
