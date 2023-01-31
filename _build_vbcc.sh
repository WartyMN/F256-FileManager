#!/bin/zsh

# export CC65=/opt/homebrew/Cellar/cc65/2.19/bin
export CC65=/opt/cc65/bin
export PATH=$CC65/bin:$PATH

DEV=~/dev/bbedit-workspace-foenix
PROJECT=$DEV/F256jr-FileManager
CONFIG_DIR=$PROJECT/cc65/include
LOCAL_CC65_DIR=$PROJECT/cc65

#DEBUG_DEFS="-DLOG_LEVEL_1 -DLOG_LEVEL_2 -DLOG_LEVEL_3 -DLOG_LEVEL_4 -DLOG_LEVEL_5"
#DEBUG_DEFS=

#DEBUG_DEF_1="-DLOG_LEVEL_1"
#DEBUG_DEF_2="-DLOG_LEVEL_2"
#DEBUG_DEF_3="-DLOG_LEVEL_3"
#DEBUG_DEF_4="-DLOG_LEVEL_4"
#DEBUG_DEF_5="-DLOG_LEVEL_5"
DEBUG_DEF_1=
DEBUG_DEF_2=
DEBUG_DEF_3=
DEBUG_DEF_4=
DEBUG_DEF_5=

#STACK_CHECK="--check-stack"
STACK_CHECK=

#optimization
#OPTI=-Oirs
OPTI=-Os

BUILD_DIR=$PROJECT/build_cc65
TARGET_DEFS="-D_TRY_TO_WRITE_TO_DISK"
#PLATFORM_DEFS="-D_SIMULATOR_" #do not define simulator if running on real hardware
PLATFORM_DEFS= #do not define simulator if running on real hardware
CC65TGT=none
#CC65LIB=f256jr.lib
CC65LIB=$LOCAL_CC65_DIR/lib/f256.lib
CC65CPU=65C02
EMULATOR=/Users/micahbly/dev/bbedit-workspace-foenix/junior-emulator/emulator/jr256
OVERLAY_CONFIG=$LOCAL_CC65_DIR/config/f256jr.cfg
DATADIR=data

cd $PROJECT

echo "\n**************************\nCC65 compile start...\n**************************\n"
which cc65

rm -r $BUILD_DIR/*.s
rm -r $BUILD_DIR/*.o

# compile
/opt/cc65/bin/cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $STACK_CHECK -T app.c -o $BUILD_DIR/app.s
/opt/cc65/bin/cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $STACK_CHECK -T comm_buffer.c -o $BUILD_DIR/comm_buffer.s
/opt/cc65/bin/cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $STACK_CHECK -T file.c -o $BUILD_DIR/file.s
/opt/cc65/bin/cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $STACK_CHECK -T folder.c -o $BUILD_DIR/folder.s
/opt/cc65/bin/cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $STACK_CHECK -T general.c -o $BUILD_DIR/general.s
/opt/cc65/bin/cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $STACK_CHECK -T list_panel.c -o $BUILD_DIR/list_panel.s
/opt/cc65/bin/cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $STACK_CHECK -T list.c -o $BUILD_DIR/list.s
/opt/cc65/bin/cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $STACK_CHECK -T screen.c -o $BUILD_DIR/screen.s
/opt/cc65/bin/cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $STACK_CHECK -T strings.c -o $BUILD_DIR/strings.s
/opt/cc65/bin/cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $STACK_CHECK -T sys.c -o $BUILD_DIR/sys.s
/opt/cc65/bin/cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $STACK_CHECK -T text.c -o $BUILD_DIR/text.s

# Kernel access
/opt/cc65/bin/cc65 -g --cpu 65C02 -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS -T $PROJECT/cc65/temp/kernel.c -o $BUILD_DIR/kernel.s


echo "\n**************************\nCA65 assemble start...\n**************************\n"

# assemble into object files
cd $BUILD_DIR
/opt/cc65/bin/ca65 -t $CC65TGT app.s
/opt/cc65/bin/ca65 -t $CC65TGT comm_buffer.s
/opt/cc65/bin/ca65 -t $CC65TGT file.s
/opt/cc65/bin/ca65 -t $CC65TGT folder.s
/opt/cc65/bin/ca65 -t $CC65TGT general.s
/opt/cc65/bin/ca65 -t $CC65TGT list_panel.s
/opt/cc65/bin/ca65 -t $CC65TGT list.s
/opt/cc65/bin/ca65 -t $CC65TGT screen.s
/opt/cc65/bin/ca65 -t $CC65TGT strings.s
/opt/cc65/bin/ca65 -t $CC65TGT sys.s
/opt/cc65/bin/ca65 -t $CC65TGT text.s

# Kernel access
/opt/cc65/bin/ca65 -t $CC65TGT kernel.s -o kernel.o

# name 'header'
/opt/cc65/bin/ca65 -t $CC65TGT ../name.s -o name.o
/opt/cc65/bin/ca65 -t $CC65TGT ../memory.asm -o memory.o


echo "\n**************************\nLD65 link start...\n**************************\n"

# link files into an executable
/opt/cc65/bin/ld65 -C $OVERLAY_CONFIG -o fmanager.rom kernel.o name.o app.o comm_buffer.o file.o folder.o general.o list.o list_panel.o memory.o screen.o strings.o sys.o text.o $CC65LIB -m fmanager_$CC65TGT.map -Ln labels.lbl
# $PROJECT/cc65/lib/common.lib

echo "\n**************************\nCC65 tasks complete\n**************************\n"

## test stuff
# cd $PROJECT
# /opt/cc65/bin/cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $STACK_CHECK -T hello.c -o $BUILD_DIR/hello.s
# cd $BUILD_DIR
# /opt/cc65/bin/ca65 -t $CC65TGT hello.s
# #ld65 -C $OVERLAY_CONFIG -o hello.rom $PROJECT/cc65/temp/kernel.o name.o hello.o general.o sys.o text.o memory.o $CC65LIB -m hello_$CC65TGT.map -Ln labels.lbl
# /opt/cc65/bin/ld65 -C $OVERLAY_CONFIG -o hello.rom kernel.o name.o hello.o $CC65LIB -m hello_$CC65TGT.map -Ln labels.lbl

## end test stuff

## ftp it to the linux box
sftp micahbly@10.0.0.122 <<EOF
cd Documents
put fmanager.rom
exit
EOF

## upload it to F256 via fnxmgr
ssh micahbly@10.0.0.122 <<EOF
cd /home/micahbly/Documents/GitHub/C256Mgr
python3 FoenixMgr/fnxmgr.py --binary /home/micahbly/Documents/fmanager.rom --address 2000
EOF

#run executable in VICE
#$EMULATOR $BUILD_DIR/basic.rom@b $BUILD_DIR/hello.rom@3000
# execute from basic: "call 12235" (jumps right to _main)


#$EMULATOR $BUILD_DIR/lichking.rom@300 boot@300

#$EMULATOR $BUILD_DIR/lichking.rom@400 $BUILD_DIR/lichking.rom.1@10000 $BUILD_DIR/lichking.rom.2@12000 $BUILD_DIR/lichking.rom.3@14000 $BUILD_DIR/lichking.rom.4@16000 $BUILD_DIR/lichking.rom.5@18000 $BUILD_DIR/lichking.rom.6@1A0000 $BUILD_DIR/lichking.rom.7@1C000 $BUILD_DIR/lichking.rom.8@1E000 $DATADIR/splashch.bin@20000 $DATADIR/splashco.bin@21000 $DATADIR/inventch.bin@22000 $DATADIR/inventco.bin@23000 $DATADIR/inventch.bin@22000 $DATADIR/inventco.bin@23000 $DATADIR/tinkerch.bin@24000 $DATADIR/tinkerco.bin@25000 $DATADIR/winch.bin@26000 $DATADIR/winco.bin@27000 $DATADIR/losech.bin@28000 $DATADIR/loseco.bin@29000 $DATADIR/helpch.bin@2A000 $DATADIR/helpco.bin@2B000 STRINGDIR/strings.bin@32000 $FONTDIR/lk_font_foenix.bin@34000 dummy.code@48000 boot@400

#/Users/micahbly/dev/bbedit-workspace-foenix/junior-emulator/emulator/jr256 lichking.rom@699 lichking.rom.1@10000 lichking.rom.2@12000 lichking.rom.3@14000 lichking.rom.4@16000 lichking.rom.5@18000 lichking.rom.6@1A000 lichking.rom.7@1C000 lichking.rom.8@1E000 lichking.rom.9@20000 ../data/splashch.bin@24000 ../data/splashco.bin@25000 ../data/inventch.bin@26000 ../data/inventco.bin@27000 ../data/tinkerch.bin@28000 ../data/tinkerco.bin@29000 ../data/winch.bin@2A000 ../data/winco.bin@2B000 ../data/losech.bin@2C000 ../data/loseco.bin@2D000 ../data/helpch.bin@2E000 ../data/helpco.bin@2F000 ../strings/strings.bin@36000 ../font/lk_font_foenix-charset.bin@3A000 dummy.code@48000 boot@0699



# make a 1541 disk image, for copying to real floppy
# $C1541UTIL -format "basic2text,1" d64 $BUILD_DIR/basic2text.d64 -attach $BUILD_DIR/basic2text.d64 \
# -write $PROJECT/$BUILD_DIR/basic2text.rom



#$EMULATOR $BUILD_DIR/lichking.rom@300 $BUILD_DIR/lichking.rom.1@10000 boot@300



#$EMULATOR boot@300 $BUILD_DIR/basic.rom@b $BUILD_DIR/hello.rom@2300

#$EMULATOR $BUILD_DIR/basic.rom@b $BUILD_DIR/hello.rom@2300
# execute from basic: "call 768"

#$EMULATOR $BUILD_DIR/basic.rom@b $BUILD_DIR/hello.rom@1000
# execute from basic: "call 4146" (jumps right to _main)
#$EMULATOR $BUILD_DIR/basic.rom@b $BUILD_DIR/hello.rom@2000
# execute from basic: "call 8242" (jumps right to _main)

echo "\n**************************\nCC65 build script complete\n**************************\n"
