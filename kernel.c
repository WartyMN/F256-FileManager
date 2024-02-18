// This file implements read(2) and write(2) along with a minimal console
// driver for reads from stdin and writes to stdout -- enough to enable
// cc65's stdio functions. It really should be written in assembler for
// speed (mostly for scrolling), but this will at least give folks a start.

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#include "api.h"
#include "app.h"	// need for FILE_MAX_PATHNAME_SIZE
#include "dirent.h"  // Users are expected to "-I ." to get the local copy.
#include "general.h" // need for strnlen
#include "f256.h" // need for F1 key values

#define VECTOR(member) (size_t) (&((struct call*) 0xff00)->member)
#define EVENT(member)  (size_t) (&((struct events*) 0)->member)
#define CALL(fn) (unsigned char) ( \
                   asm("jsr %w", VECTOR(fn)), \
                   asm("stz %v", error), \
                   asm("ror %v", error), \
                   __A__)


#pragma bss-name (push, "KERNEL_ARGS")
struct call_args args; // in gadget's version of f256 lib, this is allocated and initialized with &args in crt0. 
#pragma bss-name (pop)

#pragma bss-name (push, "ZEROPAGE")
struct event_t event; // in gadget's version of f256 lib, this is allocated and initialized with &event in crt0. 
char error;
#pragma bss-name (pop)


#define MAX_DRIVES 8

// Just hard-coded for now.
#define MAX_ROW 60
#define MAX_COL 80

static char row = 0;
static char col = 0;
static char *line = (char*) 0xc000;

 
void
kernel_init(void)
{
    args.events.event = &event;
}

static void
cls()
{
    int i;
    char *vram = (char*)0xc000;
    
    asm("lda #$02");
    asm("sta $01");  
    
    for (i = 0; i < 80*60; i++) {
        *vram++ = 32;
    }
    
    row = col = 0;
    line = (char*)0xc000;
    
    asm("stz $1"); asm("lda #9"); asm("sta $d010");
    (__A__ = row, asm("sta $d016"), asm("stz $d017"));
    (__A__ = col, asm("sta $d014"), asm("stz $d015"));
    asm("lda #'_'"); asm("sta $d012");
    asm("stz $d011");
}

void
scroll()
{
    int i;
    char *vram = (char*)0xc000;
    
    asm("lda #$02");
    asm("sta $01");  
    
    for (i = 0; i < 80*59; i++) {
        vram[i] = vram[i+80];
    }
    vram += i;
    for (i = 0; i < 80; i++) {
        *vram++ = 32;
    }
}

static void 
out(char c)
{
    switch (c) {
    case 12: 
        cls();
        break;
    default:
        asm("lda #2");
        asm("sta $01");    
        line[col] = c;
        col++;
        if (col != MAX_COL) {
            break;
        }
    case 10:
    case 13:
        col = 0;
        row++;
        if (row == MAX_ROW) {
            scroll();
            row--;
            break;
        }
        line += 80;
        break;
    }
    
    asm("stz $01");
    (__A__ = row, asm("sta $d016"));
    (__A__ = col, asm("sta $d014"));
}  
    
char
GETIN()
{
    while (1) {
        
        CALL(NextEvent);
        if (error) {
            asm("jsr %w", VECTOR(Yield));
            continue;
        }
        
        if (event.type != EVENT(key.PRESSED)) {
            continue;
        }
        
        if (event.key.flags) {
        	// if a function key, return raw code.
        	if (event.key.raw >= CH_F1 && event.key.raw <= CH_F8)
        	{
        		return event.key.raw;
        	}
            continue;  // Meta key.
        }
        
        return event.key.ascii;
    }
}

static const char *
path_without_drive(const char *path, char *drive)
{
    *drive = 0;
    
    if (strlen(path) < 2) {
        return path;
    }
    
    if (path[1] != ':') {
        return path;
    }
    
    if ((*path >= '0') && (*path <= '7')) {
        *drive = *path - '0';
    }
        
    return (path + 2);
}

int
open(const char *fname, int mode, ...)
{
    int ret = 0;
    char drive;
    
    fname = path_without_drive(fname, &drive);
    
    args.common.buf = (uint8_t*) fname;
    args.common.buflen = strlen(fname);
    args.file.open.drive = drive;
    if (mode == 1) {
        mode = 0;
    } else {
        mode = 1;
    }
    args.file.open.mode = mode;
    ret = CALL(File.Open);
    if (error) {
        return -1;
    }
    
    for(;;) {
        event.type = 0;
        asm("jsr %w", VECTOR(NextEvent));
        switch (event.type) {
        case EVENT(file.OPENED):
            return ret;
        case EVENT(file.NOT_FOUND):
        case EVENT(file.ERROR):
            return -1;
        default:
        	continue;
        }
    }
}

static int 
Kernel_Read(int fd, void *buf, uint16_t nbytes)
{
    
    if (fd == 0) {
        // stdin
        *(char*)buf = GETIN();
        return 1;
    }
    
    if (nbytes > 255) {
        nbytes = 255;
    }
    
    args.file.read.stream = fd;
    args.file.read.buflen = nbytes;
    CALL(File.Read);
    if (error) {
        return -1;
    }

    for(;;) {
        event.type = 0;
        asm("jsr %w", VECTOR(NextEvent));
        switch (event.type) {
        case EVENT(file.DATA):
            args.common.buf = buf;
            args.common.buflen = event.file.data.delivered;
            asm("jsr %w", VECTOR(ReadData));
            if (!event.file.data.delivered) {
                return 256;
            }
            return event.file.data.delivered;
        case EVENT(file.EOFx):
            return 0;
        case EVENT(file.ERROR):
            return -1;
        default: 
        	continue;
        }
    }
}

int 
read(int fd, void *buf, uint16_t nbytes)
{
    char *data = buf;
    int  gathered = 0;
    
    // fread should be doing this, but it isn't, so we're doing it.
    while (gathered < nbytes) {
        int returned = Kernel_Read(fd, data + gathered, nbytes - gathered);
        if (returned <= 0) {
            break;
        }
        gathered += returned;
    }
    
    return gathered;
}

static int
kernel_write(uint8_t fd, void *buf, uint8_t nbytes)
{
    args.file.read.stream = fd;
    args.common.buf = buf;
    args.common.buflen = nbytes;
    CALL(File.Write);
    if (error) {
        return -1;
    }

    for(;;) {
        event.type = 0;
        asm("jsr %w", VECTOR(NextEvent));
        if (event.type == EVENT(file.WROTE)) {
            return event.file.data.delivered;
        }
        if (event.type == EVENT(file.ERROR)) {
            return -1;
        }
    }
}

int 
write(int fd, const void *buf, uint16_t nbytes)
{
    uint8_t  *data = buf;
    int      total = 0;
    
    uint8_t  writing;
    int      written;
    
    if (fd == 1) {
        int i;
        char *text = (char*) buf;
        for (i = 0; i < nbytes; i++) {
            out(text[i]);
        }
        return i;
    }
    
    while (nbytes) {
        
        if (nbytes > 254) {
            writing = 254;
        } else {
            writing = nbytes;
        }
        
        written = kernel_write(fd, data+total, writing);
        if (written <= 0) {
            return -1;
        }
        
        total += written;
        nbytes -= written;
    }
        
    return total;
}


int
close(int fd)
{
    args.file.close.stream = fd;
    asm("jsr %w", VECTOR(File.Close));
    for(;;) {
        event.type = 0;
        asm("jsr %w", VECTOR(NextEvent));
        switch (event.type) {
        case EVENT(file.CLOSED):
                return 0;
        case EVENT(file.ERROR):
                return -1;
        default: continue;
        }
    }
    
    return 0;
}


   
////////////////////////////////////////
// dirent

static char dir_stream[MAX_DRIVES];

DIR* __fastcall__ 
Kernel_OpenDir(const char* name)
{
    char drive, stream;

// out(name[0]);
// out(name[1]);
// out(name[2]);
    
    name = path_without_drive(name, &drive);
//out(48+drive);
// out(48+(uint8_t)strlen(name));
   
    if (dir_stream[drive]) {
//out(64);
        return NULL;  // Only one at a time.
    }
    
    args.directory.open.drive = drive;
    args.common.buf = name;
    args.common.buflen = strlen(name);
//out(48+(uint8_t)args.common.buflen);
    stream = CALL(Directory.Open);
    if (error) {
//out(66); // B
        return NULL;
    }
//out(67); // C
    
    for(;;) {
        event.type = 0;
        asm("jsr %w", VECTOR(NextEvent));
        if (event.type == EVENT(directory.OPENED)) {
//out(68); // D
            break;
        }
        if (event.type == EVENT(directory.ERROR)) {
//out(69); // E
            return NULL;
        }
    }
    
    dir_stream[drive] = stream;
//out(70); // F
    return (DIR*) &dir_stream[drive];
}

struct dirent* __fastcall__ 
Kernel_ReadDir(DIR* dir)
{
    static struct dirent dirent;
    
    if (!dir) {
        return NULL;
    }
    
    args.directory.read.stream = *(char*)dir;
    CALL(Directory.Read);
    if (error) {
        return NULL;
    }
    
    for(;;) {
        
        unsigned len;
        
        event.type = 0;
        asm("jsr %w", VECTOR(NextEvent));
        
        switch (event.type) {
        
        case EVENT(directory.VOLUME):
            
            dirent.d_blocks = 0;
            dirent.d_type = 2;
            break;
            
        case EVENT(directory.FILE): 
            
            args.common.buf = &dirent.d_blocks;
            args.common.buflen = sizeof(dirent.d_blocks);
            CALL(ReadExt);
            dirent.d_type = (dirent.d_blocks == 0);
            break;
                
        case EVENT(directory.FREE):
            // dirent doesn't care about these types of records.
            args.directory.read.stream = *(char*)dir;
            CALL(Directory.Read);
            if (!error) {
                continue;
            }
            // Fall through.
        
        case EVENT(directory.EOFx):
        case EVENT(directory.ERROR):
            return NULL;
            
        default: continue;
        }
        
        // Copy the name.
        len = event.directory.file.len;
        if (len >= sizeof(dirent.d_name)) {
            len = sizeof(dirent.d_name) - 1;
        }
            
        if (len > 0) {
            args.common.buf = &dirent.d_name;
            args.common.buflen = len;
            CALL(ReadData);
        }
        dirent.d_name[len] = '\0';
                
        return &dirent;
    }
}
    
    
int __fastcall__ 
Kernel_CloseDir (DIR* dir)
{
    if (!dir) {
        return -1;
    }
    
    for(;;) {
        if (*(char*)dir) {
            args.directory.close.stream = *(char*)dir;
            CALL(Directory.Close);
            if (!error) {
                *(char*)dir = 0;
            }
        }
        event.type = 0;
        asm("jsr %w", VECTOR(NextEvent));
        if (event.type == EVENT(directory.CLOSED)) {
            *(char*)dir = 0;
            return 0;
        }
    }
}


// deletes the file at the specified path
// returns false in all error conditions
bool __fastcall__ Kernel_DeleteFile(const char* name)
{
    char drive, stream;
    
    name = path_without_drive(name, &drive);
    args.file.delete.drive = drive;
    args.common.buf = name;
    args.common.buflen = strlen(name);
    stream = CALL(File.Delete);
    if (error) {
        return false;
    }
    
    for(;;) {
        event.type = 0;
        asm("jsr %w", VECTOR(NextEvent));
        if (event.type == EVENT(file.DELETED)) {
            break;
        }
        if (event.type == EVENT(file.ERROR)) {
            return false;
        }
    }
    
    return true;
}

// deletes the folder at the specified path
// returns false in all error conditions
bool __fastcall__ Kernel_DeleteFolder(const char* name)
{
    char drive, stream;
    
    name = path_without_drive(name, &drive);
    args.file.delete.drive = drive;
    args.common.buf = name;
    args.common.buflen = strlen(name);
    stream = CALL(Directory.RmDir);
    if (error) {
        return false;
    }
    
    for(;;) {
        event.type = 0;
        asm("jsr %w", VECTOR(NextEvent));
        if (event.type == EVENT(directory.DELETED)) {
            break;
        }
        if (event.type == EVENT(directory.ERROR)) {
            return false;
        }
    }
    
    return true;
}


int __fastcall__ 
rename(const char* name, const char *to)
{
    char drive, stream, dest;
    
    name = path_without_drive(name, &drive);
    to = path_without_drive(to, &dest);
    if (dest != drive) {    
        // rename across drives is not supported.
        return -1;
    }
    
    args.file.delete.drive = drive;
    args.common.buf = name;
    args.common.buflen = strlen(name);
    args.common.ext = to;
    args.common.extlen = strlen(to);
    stream = CALL(File.Rename);
    if (error) {
        return -1;
    }
    
    for(;;) {
        event.type = 0;
        asm("jsr %w", VECTOR(NextEvent));
        if (event.type == EVENT(file.RENAMED)) {
            break;
        }
        if (event.type == EVENT(file.ERROR)) {
            return -1;
        }
    }
    
    return 0;
}


// wrapper to mkfs
//   pass the name you want for the formatted disk/SD card in name, and the drive number (0-2) in the drive param.
//   do NOT prepend the path onto name. 
// return negative number on any error
int __fastcall__
mkfs(const char* name, const char drive)
{
	char stream;
	
	args.file.delete.drive = drive;
    args.common.buf = name;
    args.common.buflen = strlen(name);
    stream = CALL(FileSystem.MkFS);
    if (error) {
        return -2;
    }
    
    for(;;) {
        event.type = 0;
        asm("jsr %w", VECTOR(NextEvent));
        if (event.type == EVENT(fs.CREATED)) {
            break;
        }
        if (event.type == EVENT(fs.ERROR)) {
            return -3;
        }
    }
    
    return 0;	
}


// perform a MkDir on the specified device, with the specified path
// returns false on any error
bool Kernal_MkDir(char* the_path, uint8_t drive_num)
{
    char stream;
    
    the_path += 2;	// get past 0:, 1:, 2:, etc. 

    args.directory.mkdir.drive = drive_num;
    //args.directory.mkdir.path = the_path;
    //args.directory.mkdir.path_len = General_Strnlen(the_path, FILE_MAX_PATHNAME_SIZE) + 1;
    args.common.buf = the_path;
    args.common.buflen = General_Strnlen(the_path, FILE_MAX_PATHNAME_SIZE) + 1;
    //args.directory.mkdir.cookie = 126; // NOT HANDLING THIS CURRENTLY. FUTURE: PROVIDE A COOKIE AND USE IT TO TRACK COMPLETION?

    stream = CALL(Directory.MkDir);
    
    if (error)
    {
        return false;
    }
    
    for(;;) {
        event.type = 0;
        asm("jsr %w", VECTOR(NextEvent));
        if (event.type == EVENT(directory.CREATED)) {
            break;
        }
        if (event.type == EVENT(file.ERROR)) {
            return false;
        }
    }
    
    return true;
}

// Directory.MkDir
// 
// Creates a sub-directory.
// 
// Input
// 
// kernel.args.directory.mkdir.drive contains the device id (0 = SD, 1 = IEC #8, 2 = IEC #9).
// kernel.args.directory.mkdir.path points to a buffer containing the path.
// kernel.args.directory.mkdir.path_len contains the length of the path above. May be zero for the root directory.
// kernel.args.directory.mkdir.cookie contains a user-supplied cookie for matching the completed event.
// Output
// 
// Carry cleared on success.
// Carry set on error (device not found, kernel out of event or stream objects).
// Events
// 
// On successful completion, the kernel will queue an event.directory.CREATED event.
// On error, the kernel will queue an event.directory.ERROR event.
// In either case, event.directory.cookie will contain the above cookie.


// // NOTE: this code below MIGHT be working, but SuperBASIC doesn't seem to be argument aware as of 2024/02/17
// //   also, not sure if name of superbasic is not "basic". it probably is.
// // calls SuperBASIC and tells it to run the specified basic file
// // returns error on error, and never returns on success (because SuperBASIC took over)
// bool Kernal_RunBASIC(char* the_path)
// {
//     char		stream;
//     uint8_t		path_len;
// 	
// 	// kernel.args.buf needs to have name of named app to run, which in this case is '-' (pexec's real name)
// 	// we also need to prep a different buffer with a series of pointers (2), one of which points to a string for '-', one for 'SuperBASIC', and one for the basic program SuperBASIC should load
// 	// per dwsjason, these should be located at $200, with the pointers starting at $280. 
// 	//  'arg0- to pexec should be "-", then arg1 should be the name of the pgz you want to run, includign the whole path. 
// 	args.common.buf = (char*)0x0200; //"-";
// 	args.common.buflen = 2;
// 
// 	General_Strlcpy((char*)0x0202, "SuperBASIC", 11);
// 	
//     // as of 2024-02-15, pexec doesn't support device nums, it always loads from 0:
//     the_path += 2;	// get past 0:, 1:, 2:, etc.     
// 	path_len = General_Strnlen(the_path, FILE_MAX_PATHNAME_SIZE)+1;
// 
// 	General_Strlcpy((char*)0x020d, the_path, path_len);
// 	
// 	args.common.ext = (char*)0x0280;
// 	args.common.extlen = 8;
// 	
// 	*(char*)0x0284 = 0x0d;
// 	*(char*)0x0285 = 0x02;
// 	*(char*)0x0286 = 0x00;
// 	
// 	stream = CALL(RunNamed);
//     
//     if (error) 
//     {
//         return false;
//     }
//     
//     return true; // just so cc65 is happy; but will not be hit in event of success as pexec will already be running.
// }


// calls Pexec and tells it to run the specified path. 
// returns error on error, and never returns on success (because pexec took over)
bool Kernal_RunExe(char* the_path)
{
    char		stream;
    uint8_t		path_len;
	
	// kernel.args.buf needs to have name of named app to run, which in this case is '-' (pexec's real name)
	// we also need to prep a different buffer with a series of pointers (2), one of which points to a string for '-', one for '- filetorun.pgz'
	// per dwsjason, these should be located at $200, with the pointers starting at $280. 
	//  'arg0- to pexec should be "-", then arg1 should be the name of the pgz you want to run, includign the whole path. 
	args.common.buf = (char*)0x0200; //"-";
	args.common.buflen = 2;
	
    // as of 2024-02-15, pexec doesn't support device nums, it always loads from 0:
    the_path += 2;	// get past 0:, 1:, 2:, etc.     
	path_len = General_Strnlen(the_path, FILE_MAX_PATHNAME_SIZE)+1;

	General_Strlcpy((char*)0x0202, the_path, path_len);
	
	args.common.ext = (char*)0x0280;
	args.common.extlen = 4;
	
	stream = CALL(RunNamed);
    
    if (error) 
    {
        return false;
    }
    
    return true; // just so cc65 is happy; but will not be hit in event of success as pexec will already be running.
}

// Input
// • kernel.args.buf points to a buffer containing the name of the program to run. 
// • kernel.args.buflen contains the length of the name.
// Output
// • On success, the call doesn’t return.
// • Carry set on error (a program with the provided name was not found).
// Notes
// • The name match is case-insensitive.


//https://github.com/FoenixRetro/Documentation/blob/main/f256/programming-developing.md

// Parameter Passing
// 
// Although not part of the kernel specification, a standardized method of passing commandline arguments to programs exists.
// 
// Both DOS and SuperBASIC are able to pass arguments to the program to run, and pexec is also able to pass any further arguments after the filename on to the program. As an example, /- program.pgz hello in SuperBASIC would start pexec with the parameters -, program.pgz, and hello. pexec would then load program.pgz, and start it with the parameters program.pgz and hello.
// 
// Arguments are passed in the ext and extlen kernel arguments. This approach is suitable for passing arguments through the RunNamed and RunBlock kernel functions, and is also used by pexec when starting a PGX or PGZ program.
// 
// ext will contain an array of pointers, one for each argument given on the commandline. The first pointer is the program name itself. The list is terminated with a null pointer. extlen contains the length in bytes of the array, less the null pointer. For instance, if two parameters are passed, extlen will be 4.
// 
// pexec reserves $200-$2FF for parameters - programs distributed in the PGX and PGZ formats should therefore load themselves no lower than $0300, if they want to access commandline parameters. If they do not use the commandline parameters, they may load themselves as low as $0200.