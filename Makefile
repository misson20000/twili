TWILI_OBJECTS := twili.o service/ITwiliService.o service/IPipe.o bridge/usb/USBBridge.o bridge/Object.o bridge/ResponseOpener.o bridge/ResponseWriter.o process/MonitoredProcess.o ELFCrashReport.o twili.squashfs.o service/IHBABIShim.o msgpack11/msgpack11.o process/Process.o bridge/interfaces/ITwibDeviceInterface.o bridge/interfaces/ITwibPipeReader.o TwibPipe.o bridge/interfaces/ITwibPipeWriter.o bridge/interfaces/ITwibDebugger.o ipcbind/pm/IShellService.o ipcbind/ldr/IDebugMonitorInterface.o bridge/usb/RequestReader.o bridge/usb/ResponseState.o bridge/tcp/TCPBridge.o bridge/tcp/Connection.o bridge/tcp/ResponseState.o ipcbind/nifm/IGeneralService.o ipcbind/nifm/IRequest.o Socket.o MutexShim.o service/IAppletShim.o service/IAppletShimControlImpl.o service/IAppletShimHostImpl.o AppletTracker.o process/AppletProcess.o process/ManagedProcess.o process/UnmonitoredProcess.o process_creation.o service/IAppletController.o service/fs/IFileSystem.o service/fs/IFile.o process/fs/ProcessFileSystem.o process/fs/VectorFile.o process/fs/ActualFile.o bridge/interfaces/ITwibProcessMonitor.o process/ProcessMonitor.o process/fs/TransmutationFile.o process/fs/NSOTransmutationFile.o process/fs/NRONSOTransmutationFile.o
TWILI_RESOURCES := $(addprefix build/,hbabi_shim.nro applet_host.nso twili_applet_shim/applet_host.npdm applet_control.nso twili_applet_shim/applet_control.npdm)
COMMON_OBJECTS := Buffer.o util.o

APPLET_HOST_OBJECTS := applet_host.o applet_common.o
APPLET_CONTROL_OBJECTS := applet_control.o applet_common.o
HBABI_SHIM_OBJECTS := hbabi_shim.o

BUILD_PFS0 := build_pfs0

ATMOSPHERE_TWILI_TITLE_ID := 0100000000006480
ATMOSPHERE_TWILI_TITLE_DIR := build/atmosphere/titles/$(ATMOSPHERE_TWILI_TITLE_ID)
ATMOSPHERE_TWILI_TARGETS := $(addprefix $(ATMOSPHERE_TWILI_TITLE_DIR)/,exefs/main exefs/main.npdm exefs/rtld.stub boot2.flag)

all: build/twili.nro build/twili.nso $(ATMOSPHERE_TWILI_TARGETS)

$(ATMOSPHERE_TWILI_TITLE_DIR)/exefs/main: build/twili.nso
	mkdir -p $(@D)
	cp $< $@

$(ATMOSPHERE_TWILI_TITLE_DIR)/exefs/main.npdm: twili/twili.json
	mkdir -p $(@D)
	npdmtool $< $@

build/%.npdm: %.json
	mkdir -p $(@D)
	npdmtool $< $@

%/exefs/rtld.stub:
	mkdir -p $(@D)
	touch $@

%/boot2.flag:
	mkdir -p $(@D)
	touch $@

clean:
	rm -rf build

# include libtransistor rules
ifndef LIBTRANSISTOR_HOME
    $(error LIBTRANSISTOR_HOME must be set)
endif
include $(LIBTRANSISTOR_HOME)/libtransistor.mk

CXX_FLAGS += -Werror-return-type -Og -Itwili_common -Icommon

build/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CC_FLAGS) -c -o $@ $<

build/%.o: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(TWILI_CXX_FLAGS) -c -o $@ $<

build/%.squashfs.o: build/%.squashfs
	mkdir -p $(@D)
	$(LD) -s -r -b binary -m aarch64elf -T $(LIBTRANSISTOR_HOME)/fs.T -o $@ $<

build/twili/twili.squashfs: $(TWILI_RESOURCES)
	mkdir -p $(@D)
	mksquashfs $^ $@ -comp xz -nopad -noappend

# main twili service
TWILI_DEPS = $(addprefix build/twili/,$(TWILI_OBJECTS)) $(addprefix build/common/,$(COMMON_OBJECTS))
build/twili.nro.so: $(TWILI_DEPS) $(LIBTRANSITOR_NRO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(TWILI_DEPS) $(LIBTRANSISTOR_NRO_LDFLAGS)

build/twili.nso.so: $(TWILI_DEPS) $(LIBTRANSITOR_NSO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(TWILI_DEPS) $(LIBTRANSISTOR_NSO_LDFLAGS)

# applet host
APPLET_HOST_DEPS = $(addprefix build/twili_applet_shim/,$(APPLET_HOST_OBJECTS))
build/applet_host.nro.so: $(APPLET_HOST_DEPS) $(LIBTRANSITOR_NRO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(APPLET_HOST_DEPS) $(LIBTRANSISTOR_NRO_LDFLAGS)

build/applet_host.nso.so: $(APPLET_HOST_DEPS) $(LIBTRANSITOR_NSO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(APPLET_HOST_DEPS) $(LIBTRANSISTOR_NSO_LDFLAGS)

# applet control
APPLET_CONTROL_DEPS = $(addprefix build/twili_applet_shim/,$(APPLET_CONTROL_OBJECTS))
build/applet_control.nro.so: $(APPLET_CONTROL_DEPS) $(LIBTRANSITOR_NRO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(APPLET_CONTROL_DEPS) $(LIBTRANSISTOR_NRO_LDFLAGS)

build/applet_control.nso.so: $(APPLET_CONTROL_DEPS) $(LIBTRANSITOR_NSO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(APPLET_CONTROL_DEPS) $(LIBTRANSISTOR_NSO_LDFLAGS)

# HBABI shim
HBABI_SHIM_DEPS = $(addprefix build/hbabi_shim/,$(HBABI_SHIM_OBJECTS))
build/hbabi_shim.nro.so: $(HBABI_SHIM_DEPS) $(LIBTRANSITOR_NRO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(HBABI_SHIM_DEPS) $(LIBTRANSISTOR_NRO_LDFLAGS)
