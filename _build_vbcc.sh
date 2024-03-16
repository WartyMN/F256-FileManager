#!/bin/zsh

export CC65=/opt/homebrew/Cellar/cc65/2.19/bin
export PATH=$CC65/bin:$PATH

DEV=~/dev/bbedit-workspace-foenix
PROJECT=$DEV/F256jr-FileManager
CONFIG_DIR=$PROJECT/config_cc65

# name that will be used in files
VERSION_STRING="1.0b22"

#DEBUG_DEFS="-DLOG_LEVEL_1 -DLOG_LEVEL_2 -DLOG_LEVEL_3 -DLOG_LEVEL_4 -DLOG_LEVEL_5"
#DEBUG_DEFS=

# debug logging levels: 1=error, 2=warn, 3=info, 4=debug general, 5=allocations
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

# whether disk or serial debug will be used, IF debug is actually on
# defining serial debug means serial will be used, not defining it means disk will be used. 
DEBUG_VIA_SERIAL="-DUSE_SERIAL_LOGGING"
#DEBUG_VIA_SERIAL=

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
CC65LIB=f256_lichking_only.lib
CC65CPU=65C02
EMULATOR=/Users/micahbly/dev/bbedit-workspace-foenix/junior-emulator/emulator/jr256
OVERLAY_CONFIG=fmanager_overlay_f256.cfg
DATADIR=data

cd $PROJECT

echo "\n**************************\nCC65 compile start...\n**************************\n"
which cc65

rm -r $BUILD_DIR/*.s
rm -r $BUILD_DIR/*.o

# compile
cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T app.c -o $BUILD_DIR/app.s
cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T comm_buffer.c -o $BUILD_DIR/comm_buffer.s
cc65 -g --cpu $CC65CPU -t $CC65TGT --code-name OVERLAY_2 $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T file.c -o $BUILD_DIR/file.s
cc65 -g --cpu $CC65CPU -t $CC65TGT --code-name OVERLAY_2 $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T folder.c -o $BUILD_DIR/folder.s
cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T general.c -o $BUILD_DIR/general.s
cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T keyboard.c -o $BUILD_DIR/keyboard.s
cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T list_panel.c -o $BUILD_DIR/list_panel.s
cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T list.c -o $BUILD_DIR/list.s
cc65 -g --cpu $CC65CPU -t $CC65TGT --code-name OVERLAY_1 $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T screen.c -o $BUILD_DIR/screen.s
cc65 -g --cpu $CC65CPU -t $CC65TGT --code-name OVERLAY_3 $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T overlay_em.c -o $BUILD_DIR/overlay_em.s
cc65 -g --cpu $CC65CPU -t $CC65TGT --code-name OVERLAY_4 $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T overlay_startup.c -o $BUILD_DIR/overlay_startup.s
cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T sys.c -o $BUILD_DIR/sys.s
cc65 -g --cpu $CC65CPU -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS $DEBUG_DEF_1 $DEBUG_DEF_2 $DEBUG_DEF_3 $DEBUG_DEF_4 $DEBUG_DEF_5 $DEBUG_VIA_SERIAL $STACK_CHECK -T text.c -o $BUILD_DIR/text.s

# Kernel access
cc65 -g --cpu 65C02 -t $CC65TGT $OPTI -I $CONFIG_DIR $TARGET_DEFS $PLATFORM_DEFS -T kernel.c -o $BUILD_DIR/kernel.s


echo "\n**************************\nCA65 assemble start...\n**************************\n"

# assemble into object files
cd $BUILD_DIR
ca65 -t $CC65TGT app.s
ca65 -t $CC65TGT comm_buffer.s
ca65 -t $CC65TGT file.s
ca65 -t $CC65TGT folder.s
ca65 -t $CC65TGT general.s
ca65 -t $CC65TGT keyboard.s
ca65 -t $CC65TGT list_panel.s
ca65 -t $CC65TGT list.s
ca65 -t $CC65TGT overlay_em.s
ca65 -t $CC65TGT overlay_startup.s
ca65 -t $CC65TGT screen.s
ca65 -t $CC65TGT sys.s
ca65 -t $CC65TGT text.s

# Kernel access
ca65 -t $CC65TGT kernel.s -o kernel.o

# name 'header'
#ca65 -t $CC65TGT ../name.s -o name.o
ca65 -t $CC65TGT ../memory.asm -o memory.o


echo "\n**************************\nLD65 link start...\n**************************\n"

# link files into an executable
ld65 -C $CONFIG_DIR/$OVERLAY_CONFIG -o fmanager.rom kernel.o app.o comm_buffer.o file.o folder.o general.o keyboard.o list.o list_panel.o memory.o overlay_em.o overlay_startup.o screen.o sys.o text.o $CC65LIB -m fmanager_$CC65TGT.map -Ln labels.lbl
# $PROJECT/cc65/lib/common.lib

#noTE: 2024-02-12: removed name.o as it was incompatible with the lichking-style memory map I want to use to get more memory

echo "\n**************************\nCC65 tasks complete\n**************************\n"




#copy strings to build folder
#cp fmanager.rom ../release/
#cp fmanager.rom.1 ../release/
#cp fmanager.rom.2 ../release/
#cp fmanager.rom.3 ../release/
#cp fmanager.rom.4 ../release/
cp ../strings/strings.bin .


#build pgZ
# cd ../release
 fname=("fmanager.rom" "fmanager.rom.1" "fmanager.rom.2" "fmanager.rom.3" "fmanager.rom.4" "strings.bin")
 addr=("990700" "000001" "002001" "004001" "006001" "004002")

 for ((i = 1; i <= $#fname; i++)); do
   v1=$(stat -f%z $fname[$i]); v2=$(printf '%04x\n' $v1); v3='00'$v2; v4=$(echo -n $v3 | tac -rs ..); v5=$addr[$i]$v4;v6=$(sed -Ee 's/([A-Za-z0-9]{2})/\\\x\1/g' <<< "$v5"); echo -n $v6 > $fname[$i]'.hdr'
 done

 echo -n 'Z' >> pgZ_start.hdr
 echo -n '\x99\x07\x00\x00\x00\x00' >> pgZ_end.hdr

 cat pgZ_start.hdr fmanager.rom.hdr fmanager.rom fmanager.rom.1.hdr fmanager.rom.1 fmanager.rom.2.hdr fmanager.rom.2 fmanager.rom.3.hdr fmanager.rom.3 fmanager.rom.4.hdr fmanager.rom.4 strings.bin.hdr strings.bin pgZ_end.hdr > fm.pgZ 

 rm *.hdr

# build as firmware/flashware

# cat the header with KUP header and compact pexec with the pgz file
cat fm_firmware_header.bin fm.pgz > fm.bin

# split the fm.bin up  (header + cat program)
cd fm_flash
split -d -b8192 ../fm.bin fm.

# make the last chunk be exactly 8k
truncate -s 8K fm.05

# zip it up
cd ../
zip -vrq fm_"$VERSION_STRING"_flash.zip fm_flash/ -x "*.DS_Store"

# clear temp files
rm fmanager.ro*
rm strings.bin
rm fm.bin

# copy pgz binary to SD Card on F256 via fnxmanager
python3 $FOENIXMGR/FoenixMgr/fnxmgr.py --copy fm.pgZ

# upload bin to flash memory



echo "\n**************************\nCC65 build script complete\n**************************\n"
