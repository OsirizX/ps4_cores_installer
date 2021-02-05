WINE =
WINE_PATH_TOOL = winepath

CC = $(WINE) orbis-clang.exe
CXX = $(WINE) orbis-clang++.exe
LD = $(WINE) orbis-clang.exe
OBJCOPY = $(WINE) orbis-objcopy.exe
PUBCMD = $(WINE) orbis-pub-cmd.exe
MAKE_FSELF = python $(SCE_ORBIS_SDK_DIR)/fake_pkg_tools/make_fself.py

OBJDIR = obj
BLDDIR = build
MODDIR = sce_module

TARGET = cores_installer
LIBS = -lkernel_tau_stub_weak -lSceSysmodule_tau_stub_weak -lSceSystemService_stub_weak -lSceSystemService_tau_stub_weak -lSceShellCoreUtil_tau_stub_weak -lScePigletv2VSH_tau_stub_weak -lkernel_util -lSceNet_stub_weak -lSceNetCtl_stub_weak -lSceCommonDialog_stub_weak -lSceMsgDialog_stub_weak -lSceUserService_stub_weak
SDK_MODULES = libc.prx libSceFios2.prx
EXTRA_MODULES =
AUTH_INFO = 000000000000000000000000001C004000FF000000000080000000000000000000000000000000000000008000400040000000000000008000000000000000080040FFFF000000F000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

ASM_SRCS =
C_SRCS = main.c
CXX_SRCS =

COMMON_FLAGS = -Wall
COMMON_FLAGS += -fdiagnostics-color=always
COMMON_FLAGS += -I $(TAUON_SDK_DIR)/include -I $(SCE_ORBIS_SDK_DIR)/target/include -I $(SCE_ORBIS_SDK_DIR)/target/include/common -I $(SCE_ORBIS_SDK_DIR)/target/include/common
COMMON_FLAGS += -DNDEBUG
COMMON_FLAGS += -g

CFLAGS = $(COMMON_FLAGS)
CFLAGS += -std=c11
CFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-unused-label -Werror=implicit-function-declaration
CFLAGS += -fno-strict-aliasing
CFLAGS += -fPIC
CFLAGS += -O3

CXXFLAGS = $(COMMON_FLAGS)
CXXFLAGS += -std=c++11
CXXFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-unused-label -Werror=implicit-function-declaration
CXXFLAGS += -fno-strict-aliasing
CXXFLAGS += -fPIC
CXXFLAGS += -O3
CXXFLAGS += -fexceptions -frtti

ASFLAGS = $(COMMON_FLAGS)

LDFLAGS = -Wl,--strip-unused-data
LDFLAGS += -L $(TAUON_SDK_DIR)/lib -L $(SCE_ORBIS_SDK_DIR)/target/lib -L $(SCE_ORBIS_SDK_DIR)/target/samples/sample_code/common/lib/external

OBJS = $(addprefix $(OBJDIR)/,$(ASM_SRCS:.S=.S.o) $(C_SRCS:.c=.c.o) $(CXX_SRCS:.cpp=.cpp.o))

.PHONY: all clean

all: post-build

pre-build:
	@mkdir -p $(MODDIR) $(OBJDIR) $(BLDDIR)
	@for filename in $(SDK_MODULES); do \
		if [ ! -f "$(MODDIR)/$$filename" ]; then \
			echo Copying $$filename...; \
			cp "`$(WINE_PATH_TOOL) -u \"$(SCE_ORBIS_SDK_DIR)/target/sce_module/$$filename\"`" $(MODDIR)/; \
		fi; \
	done;

post-build: main-build

main-build: pre-build
	@$(MAKE) --no-print-directory pkg

eboot: $(OBJS)
	$(CC) $(LDFLAGS) -o $(BLDDIR)/$(TARGET).elf $^ $(LIBS)

$(OBJDIR)/%.S.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(OBJDIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

sfo:
	$(PUBCMD) sfo_create sce_sys/param.sfx $(BLDDIR)/param.sfo

pkg: sfo eboot
	$(MAKE_FSELF) --auth-info $(AUTH_INFO) $(BLDDIR)/$(TARGET).elf $(BLDDIR)/$(TARGET).self
	$(PUBCMD) img_create $(TARGET).gp4 $(BLDDIR)/$(TARGET).pkg

clean:
	@rm -rf $(OBJDIR) $(BLDDIR)
