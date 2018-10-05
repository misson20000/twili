TWILI_OBJECTS := twili.o service/ITwiliService.o service/IPipe.o bridge/usb/USBBridge.o bridge/Object.o bridge/ResponseOpener.o bridge/ResponseWriter.o process/MonitoredProcess.o ELFCrashReport.o twili.squashfs.o service/IHBABIShim.o msgpack11/msgpack11.o process/Process.o bridge/interfaces/ITwibDeviceInterface.o bridge/interfaces/ITwibPipeReader.o TwibPipe.o bridge/interfaces/ITwibPipeWriter.o bridge/interfaces/ITwibDebugger.o ipcbind/pm/IShellService.o ipcbind/ldr/IDebugMonitorInterface.o bridge/usb/RequestReader.o bridge/usb/ResponseState.o bridge/tcp/TCPBridge.o bridge/tcp/Connection.o bridge/tcp/ResponseState.o ipcbind/nifm/IGeneralService.o ipcbind/nifm/IRequest.o Socket.o MutexShim.o service/IAppletShim.o service/IAppletShimControlImpl.o service/IAppletShimHostImpl.o AppletTracker.o process/AppletProcess.o process/ManagedProcess.o process/UnmonitoredProcess.o
TWILI_COMMON_OBJECTS := process_creation.o util.o
COMMON_OBJECTS := Buffer.o

TWILI_LAUNCHER_OBJECTS := twili_launcher.o twili_launcher.squashfs.o
TWILI_APPLET_SHIM_OBJECTS := twili_applet_shim.o
HBABI_SHIM_OBJECTS := hbabi_shim.o

BUILD_PFS0 := build_pfs0

ATMOSPHERE_TWILI_TITLE_ID := 0100000000006480
ATMOSPHERE_TWILI_TITLE_DIR := build/atmosphere/titles/$(ATMOSPHERE_TWILI_TITLE_ID)
ATMOSPHERE_TWILI_TARGETS := $(addprefix $(ATMOSPHERE_TWILI_TITLE_DIR)/,exefs/main exefs/main.npdm exefs/rtld.stub boot2.flag)

ATMOSPHERE_TWILI_APPLET_SHIM_TITLE_ID := 010000000000100d
ATMOSPHERE_TWILI_APPLET_SHIM_TITLE_DIR := build/atmosphere/titles/$(ATMOSPHERE_TWILI_APPLET_SHIM_TITLE_ID)
ATMOSPHERE_TWILI_APPLET_SHIM_TARGETS := $(addprefix $(ATMOSPHERE_TWILI_APPLET_SHIM_TITLE_DIR)/,exefs/main exefs/main.npdm exefs/rtld.stub)

all: build/twili_launcher.nsp build/twili.nro build/twili.nso build/twili.kip build/twili_launcher.kip $(ATMOSPHERE_TWILI_TARGETS) $(ATMOSPHERE_TWILI_APPLET_SHIM_TARGETS)

build/twili_launcher.nsp: build/twili_launcher_exefs/main build/twili_launcher_exefs/main.npdm
	mkdir -p $(@D)
	$(BUILD_PFS0) build/twili_launcher_exefs/ $@

build/twili_launcher_exefs/main: build/twili_launcher.nso
	mkdir -p $(@D)
	cp $< $@

build/twili_launcher_exefs/main.npdm: twili_launcher/twili_launcher.json
	mkdir -p $(@D)
	npdmtool $< $@

$(ATMOSPHERE_TWILI_TITLE_DIR)/exefs/main: build/twili.nso
	mkdir -p $(@D)
	cp $< $@

$(ATMOSPHERE_TWILI_TITLE_DIR)/exefs/main.npdm: twili/twili.json
	mkdir -p $(@D)
	npdmtool $< $@

$(ATMOSPHERE_TWILI_APPLET_SHIM_TITLE_DIR)/exefs/main: build/twili_applet_shim.nso
	mkdir -p $(@D)
	cp $< $@

$(ATMOSPHERE_TWILI_APPLET_SHIM_TITLE_DIR)/exefs/main.npdm: twili_applet_shim/twili_applet_shim.json
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

CXX_FLAGS += -Werror-return-type -Og -I$(realpath twili_common)

build/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CC_FLAGS) -c -o $@ $<

build/%.o: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(TWILI_CXX_FLAGS) -c -o $@ $<

build/%.squashfs.o: build/%.squashfs
	mkdir -p $(@D)
	$(LD) -s -r -b binary -m aarch64elf -T $(LIBTRANSISTOR_HOME)/fs.T -o $@ $<

build/twili/twili.squashfs: build/hbabi_shim.nro
	mkdir -p $(@D)
	mksquashfs $^ $@ -comp xz -nopad -noappend

build/twili_launcher/twili_launcher.squashfs: build/twili.nro
	mkdir -p $(@D)
	mksquashfs $^ $@ -comp xz -nopad -noappend

build/twili.kip: build/twili.nso.so twili/twili_kip.json
	elf2kip build/twili.nso.so twili/twili_kip.json build/twili.kip

build/twili_launcher.kip: build/twili_launcher.nso.so twili_launcher/twili_launcher_kip.json
	elf2kip build/twili_launcher.nso.so twili_launcher/twili_launcher_kip.json build/twili_launcher.kip

# main twili service
TWILI_DEPS = $(addprefix build/twili/,$(TWILI_OBJECTS)) $(addprefix build/twili_common/,$(TWILI_COMMON_OBJECTS)) $(addprefix build/common/,$(COMMON_OBJECTS))
build/twili.nro.so: $(TWILI_DEPS) $(LIBTRANSITOR_NRO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(TWILI_DEPS) $(LIBTRANSISTOR_NRO_LDFLAGS)

build/twili.nso.so: $(TWILI_DEPS) $(LIBTRANSITOR_NSO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(TWILI_DEPS) $(LIBTRANSISTOR_NSO_LDFLAGS)

# twili launcher
TWILI_LAUNCHER_DEPS = $(addprefix build/twili_launcher/,$(TWILI_LAUNCHER_OBJECTS)) $(addprefix build/twili_common/,$(TWILI_COMMON_OBJECTS))
build/twili_launcher.nso.so: $(TWILI_LAUNCHER_DEPS) $(LIBTRANSITOR_NSO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(TWILI_LAUNCHER_DEPS) $(LIBTRANSISTOR_NSO_LDFLAGS)

# twili applet shim
TWILI_APPLET_SHIM_DEPS = $(addprefix build/twili_applet_shim/,$(TWILI_APPLET_SHIM_OBJECTS))
build/twili_applet_shim.nro.so: $(TWILI_APPLET_SHIM_DEPS) $(LIBTRANSITOR_NRO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(TWILI_APPLET_SHIM_DEPS) $(LIBTRANSISTOR_NRO_LDFLAGS)

build/twili_applet_shim.nso.so: $(TWILI_APPLET_SHIM_DEPS) $(LIBTRANSITOR_NSO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(TWILI_APPLET_SHIM_DEPS) $(LIBTRANSISTOR_NSO_LDFLAGS)

# HBABI shim
HBABI_SHIM_DEPS = $(addprefix build/hbabi_shim/,$(HBABI_SHIM_OBJECTS))
build/hbabi_shim.nro.so: $(HBABI_SHIM_DEPS) $(LIBTRANSITOR_NRO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(HBABI_SHIM_DEPS) $(LIBTRANSISTOR_NRO_LDFLAGS)
