TOOLCHAIN		?=	../palmdev_V3/buildtools/toolchain/bin/
SDK				?=	../palmdev_V3/buildtools/palm-os-sdk-master/sdk-5r3/include
PILRC			=	../palmdev_V3/buildtools/pilrc3_3_unofficial/bin/pilrc
CC				=	$(TOOLCHAIN)/m68k-none-elf-gcc
LD				=	$(TOOLCHAIN)/m68k-none-elf-gcc
OBJCOPY			=	$(TOOLCHAIN)/m68k-none-elf-objcopy
COMMON			=	-Wno-multichar -funsafe-math-optimizations -Os -m68000 -mno-align-int -mpcrel -fpic -fshort-enums -mshort
WARN			=	-Wsign-compare -Wextra -Wall -Werror -Wno-unused-parameter -Wno-old-style-declaration -Wno-unused-function -Wno-unused-variable -Wno-error=cpp -Wno-error=switch -Wno-implicit-fallthrough -Wno-shift-count-overflow -Wno-maybe-uninitialized
LKR				=	linker.lkr
CCFLAGS			=	$(LTO) $(WARN) $(COMMON) -I. -ffunction-sections -fdata-sections
LDFLAGS			=	$(LTO) $(WARN) $(COMMON) -Wl,--gc-sections -Wl,-T $(LKR)
RSC				=	src/
SRCS_CLI		=   src/devices/system.c src/uxn.c src/uxncli.c 
RCP_CLI			=	src/uxncli.rcp
OBJS_CLI		=	$(patsubst %.S,%.o,$(patsubst %.c,%.o,$(SRCS_CLI)))
TARGET_CLI		=	uxnemu
SRCS_V			=   src/devices/system.c src/devices/screen.c src/uxn.c src/uxnemu.c
RCP_V			=	src/uxnemu.rcp
OBJS_V			=	$(patsubst %.S,%.o,$(patsubst %.c,%.o,$(SRCS_V)))
TARGET_V		=	uxnemu
CREATOR			=	UxnV
TYPE			=	appl

#add PalmOS SDK
INCS			+=	-isystem$(SDK)
INCS			+=	-isystem$(SDK)/Core
INCS			+=	-isystem$(SDK)/Core/Hardware
INCS			+=	-isystem$(SDK)/Core/System
INCS			+=	-isystem$(SDK)/Core/UI
INCS			+=	-isystem$(SDK)/Dynamic
INCS			+=	-isystem$(SDK)/Libraries
INCS			+=	-isystem$(SDK)/Libraries/PalmOSGlue



$(TARGET_CLI).prc: code0001.bin
	$(PILRC) -ro -o $(TARGET_CLI).prc -creator $(CREATOR) -type $(TYPE) -name $(TARGET_CLI) -I $(RSC) $(RCP_CLI) && rm code0001.bin

$(TARGET_V).prc: code0002.bin
	$(PILRC) -ro -o $(TARGET_V).prc -creator $(CREATOR) -type $(TYPE) -name $(TARGET_V) -I $(RSC) $(RCP_V) && rm code0002.bin

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@ -j.vec -j.text -j.rodata

code0001.elf: $(OBJS_CLI)
	$(LD) -o $@ $(LDFLAGS) $^

code0002.elf: $(OBJS_V)
	$(LD) -o $@ $(LDFLAGS) $^

%.o : %.c Makefile
	$(CC) $(CCFLAGS)  $(INCS) -c $< -o $@

clean:
	rm -rf $(OBJS_CLI) $(OBJ_V) $(NAME).elf
 
.PHONY: clean
