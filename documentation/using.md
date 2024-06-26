# Using f/manager

## Features

- View and sort the directory of your SD card, and up to 2 connected IEC devices (Commodore-format serial disk drives such as the 1541, 1571, 1581, FNX1591, or FNX1592). 
- Format floppy disks and SD cards
- Create subdirectories on SD cards
- Navigate through subdirectories
- Copy files from one place on a disk, to another place
- Copy files from one device to another
- Rename files
- Delete files
- View a file as text (including word-wrap)
- View a fix as hex
- Launch applications
- Launch files that have known helper applications - images, basic programs, mods, etc.
- View a "directory" of the RAM banks or flash memory banks of your computer
- Load a file from disk into a location in memory of your choosing
- Save a bank (8192 bytes) of memory to a file on disk
- Fill a bank of memory with a byte value you choose
- Find text strings and strings of hex digits in memory
- Set the real-time clock on your F256
- Shortcut to switch to DOS or SuperBASIC at any time

## Starting the f/manager

How you start up the f/manager depends on how you installed it in the first place, and what you are currently doing on the Foenix:
- If you installed it in flash, in the lowest position: turn on or reset your F256. f/manager will automatically start up. 
- If you installed it in flash, but not in the lowest position:
	- If you are in DOS, type "fm" and `<ENTER>`
	- If you are in SuperBASIC, type "/fm" and `<ENTER>`
	- If you are in some other program, say 3 Hail Marys, reset the computer, and hope it comes up in f/manager, DOS, or SuperBASIC. 
- If you did not install it on flash, and have it on disk:
	- If you are in DOS, type "- fm" and `<ENTER>`
	- If you are in SuperBASIC, type "/- fm" and `<ENTER>`
	- If you are in some other program, go outside, run around your house or apartment building 3 times _widdershins_ (very important), go back inside, reset the computer, and hope it comes up in f/manager, DOS, or SuperBASIC. 

## Getting Stuff Done with the f/manager

### Working with Files
- [I want to see what's on my SD Card or floppy disk](#i-want-to-see-whats-on-my-sd-card-or-floppy-disk)
- [I want to copy a file from one place in my SD card to another place](#i-want-to-copy-a-file-from-one-place-in-my-sd-card-to-another-place)
- [I want to copy a file from disk to another disk](#i-want-to-copy-a-file-from-disk-to-another-disk)
- more to come...

### Working with Memory
- coming soon...

### Running other applications
- [insert animated under construction GIF here]

### Doing other Stuff
- [I want to set the system clock](#i-want-to-set-the-system-clock)



#### I want to see what's on my SD Card or floppy disk

On startup, f/manager will scan your system to determine how many disk systems you have attached. Being a Foenix MicroKernel-based program, it can support device 0, the SD Card, as well as Devices 1 and 2, which are reserved for IEC floppy drives if you have any. As long as you have your SD card installed (and it is compatible), the left-hand panel will already be displaying the contents of it. If no SD card was installed, but you have 1 or floppy drives connected, f/manager will show you the contents of the floppies instead. If you have no SD card and no floppies installed, f/manager will instead show you a list of the memory banks in your computer. 

#### I want to copy a file from one place in my SD card to another place

f/manager is a classic two-pane file manager. It will let you copy from either the left pane or the right pane, provided the opposite pane is representing a write-capable device. You can copy from flash memory to disk, for example, but you cannot copy from disk or RAM to flash. Not without reconfiguring the main deflector dish, at least. You can select the same device in both panes. This is allowed and normal, and it is in fact the easiest way to copy a file from one directory to another. Let's take an example where we care copying a file from the SD card's root directory, to a subdirectory called "Games". With the left pane activated (the active pane is the bright green one, the inactive pane is the faded one), hit '0' to show the contents of the SD card's root directory, if it is not already showing. Hit the `<TAB>` key to switch focus to the right-hand pane. Do the same thing here: hit '0' to see the root folder of the SD card. Now use the up and down cursor keys to navigate to the folder you want to copy a file to ("Games" in our example). Enter the folder by hitting, you guessed it: `<ENTER>`. The pane will reload, showing the contents of the directory you selected. Now hit the left cursor key to make the left pane the active pane again. You could also have used `<TAB>` which toggles between left and right. Use the up and down cursor keys to highlight the file you want to copy. When you have it selected, hit 'c' to start the copy operation.  

#### I want to copy a file from disk to another disk

Easy-peasy, lemon-squeezy. Use the '0','1', and/or '2' keys to get the source and destination disks showing in the left and right panes. It doesn't matter which is at left or right. Once you have that set up, make sure the source disk pane is active (bright green). Use `<TAB>` or left/right cursor keys if necessary. Now hit the 'c' button. The Copy menu item shows a series of arrows ('<<<<' or '>>>>') to help you understand which way it will copy when you hit 'c'. That's it. The progress bar will let you know when the copy is complete. File copy is pretty fast on an SD card, but if  you were copy a large file, say "fm.pgZ" from the SD card to a floppy in a 1541 drive, well, that would give you an idea of what computing was like in the 1980s. 


#### I want to set the system clock

Did you notice that yellow clock at the top right of your f/manager screen? If you have never set your clock before, it's probably showing the wrong time. Or maybe a bit of code ran wild and overwrote VICKY registers. Let's get your clock set again. Type 'C'. A dialog will open. Enter the date using the format shown. For example, to set it to April 1, 2024, 6:08 pm, you would enter "24-04-01 18:08". The seconds are always reset to 0. When I was a kid computers couldn't afford seconds. Deal with it. [What's with the weird year>month>day thing, you ask? Here you go.](https://imgs.xkcd.com/comics/iso_8601.png)

[Top](using.md)

[Back](_user_guide.md)