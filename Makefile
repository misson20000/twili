OBJECTS := twili.o ITwiliService.o IPipe.o USBBridge.o process_creation.o MonitoredProcess.o ELFCrashReport.o twili.squashfs.o IHBABIShim.o util.o msgpack11/msgpack11.o Process.o
LAUNCHER_OBJECTS := twili_launcher.o twili_launcher.squashfs.o process_creation.o util.o
HBABI_SHIM_OBJECTS := hbabi_shim.o

TWILI_CXX_FLAGS := -Werror-return-type -Og

BUILD_PFS0 := build_pfs0

all: build/twili_launcher.nsp build/twili.nro build/twili.nso build/twili.kip

build/twili_launcher.nsp: build/twili_launcher_exefs/main build/twili_launcher_exefs/main.npdm
	mkdir -p $(@D)
	$(BUILD_PFS0) build/twili_launcher_exefs/ $@

build/twili_launcher_exefs/main: build/twili_launcher.nso
	mkdir -p $(@D)
	cp $< $@

build/twili_launcher_exefs/main.npdm: main.npdm
	mkdir -p $(@D)
	cp $< $@

clean:
	rm -rf build

# include libtransistor rules
ifndef LIBTRANSISTOR_HOME
    $(error LIBTRANSISTOR_HOME must be set)
endif
include $(LIBTRANSISTOR_HOME)/libtransistor.mk

build/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CC_FLAGS) -c -o $@ $<

build/%.o: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(TWILI_CXX_FLAGS) -c -o $@ $<

build/%.squashfs.o: build/%.squashfs
	mkdir -p $(@D)
	$(LD) -s -r -b binary -m aarch64elf -T $(LIBTRANSISTOR_HOME)/fs.T -o $@ $<

build/twili.squashfs: build/hbabi_shim.nro
	mkdir -p $(@D)
	mksquashfs $^ $@ -comp xz -nopad -noappend

build/twili_launcher.squashfs: build/twili.nro
	mkdir -p $(@D)
	mksquashfs $^ $@ -comp xz -nopad -noappend

build/twili.kip: build/twili.nso.so twili_kip.json
	elf2kip build/twili.nso.so twili_kip.json build/twili.kip

build/twili.nro.so: $(addprefix build/,$(OBJECTS)) $(LIBTRANSITOR_NRO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(addprefix build/,$(OBJECTS)) $(LIBTRANSISTOR_NRO_LDFLAGS)

build/twili.nso.so: $(addprefix build/,$(OBJECTS)) $(LIBTRANSITOR_NSO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(addprefix build/,$(OBJECTS)) $(LIBTRANSISTOR_NSO_LDFLAGS)

build/twili_launcher.nso.so: $(addprefix build/,$(LAUNCHER_OBJECTS)) $(LIBTRANSITOR_NSO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(addprefix build/,$(LAUNCHER_OBJECTS)) $(LIBTRANSISTOR_NSO_LDFLAGS)

build/hbabi_shim.nro.so: $(addprefix build/,$(HBABI_SHIM_OBJECTS)) $(LIBTRANSITOR_NRO_LIB) $(LIBTRANSISTOR_COMMON_LIBS)
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -o $@ $(addprefix build/,$(HBABI_SHIM_OBJECTS)) $(LIBTRANSISTOR_NRO_LDFLAGS)
