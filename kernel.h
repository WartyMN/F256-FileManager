#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdbool.h>

// wrapper to mkfs
//   pass the name you want for the formatted disk/SD card in name, and the drive number (0-2) in the drive param.
//   do NOT prepend the path onto name. 
// return negative number on any error
int __fastcall__ mkfs(const char* name, const char drive);

char GETIN(void);
void kernel_init(void);

// perform a MkDir on the specified device, with the specified path
// returns false on any error
bool Kernal_MkDir(char* the_folder_path, uint8_t drive_num);

// calls Pexec and tells it to run the specified path. 
// returns error on error, and never returns on success (because pexec took over)
bool Kernal_RunExe(char* the_path);

// // calls SuperBASIC and tells it to run the specified basic file
// // returns error on error, and never returns on success (because SuperBASIC took over)
// bool Kernal_RunBASIC(char* the_path);

// calls modojr and tells it to load the specified .mod file
// returns error on error, and never returns on success (because pexec took over)
bool Kernal_RunMod(char* the_path);

// deletes the file at the specified path
// returns false in all error conditions
bool __fastcall__ Kernel_DeleteFile(const char* name);

// deletes the folder at the specified path
// returns false in all error conditions
bool __fastcall__ Kernel_DeleteFolder(const char* name);

#endif /* KERNEL_H_ */
