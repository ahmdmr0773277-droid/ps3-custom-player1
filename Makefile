# --- إعدادات البيئة لـ WSL ---
ifeq ($(strip $(PSL1GHT)),)
$(error "Please set PSL1GHT in your environment path or defined variables")
endif

TARGET    := custom_player
TITLE     := PS3 Custom Link Player
APPID     := CNMN00001
CONTENTID := UP0001-CNMN00001_00-CUSTOMPLAYER0000

# --- المكتبات والمسارات ---
LIBS    := -L$(PSL1GHT)/lib -lnet -lsysutil -lio -lrt
INCLUDE := -I$(PSL1GHT)/include
CFLAGS  := -g -O2 -Wall --std=gnu99 $(INCLUDE)

include $(PSL1GHT)/ppu_rules

all: $(TARGET).pkg

clean:
	rm -rf $(TARGET).elf $(TARGET).self $(TARGET).pkg obj/ PARAM.SFO