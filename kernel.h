#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdbool.h>
#include <stdint.h>

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

// runs a name program (a KUP, in other words)
// pass the KUP name and length
// returns error on error, and never returns on success (because SuperBASIC took over)
void Kernal_RunNamed(char* kup_name, uint8_t name_len);

// calls pexec, passing the path to an app to load, and optionally, the path to a file for that app to load
// returns error on error, and never returns on success (because pexec took over)
bool Kernal_LoadApp(char* the_app_path, char* the_file_path);

// deletes the file at the specified path
// returns false in all error conditions
bool __fastcall__ Kernel_DeleteFile(const char* name);

// deletes the folder at the specified path
// returns false in all error conditions
bool __fastcall__ Kernel_DeleteFolder(const char* name);

// check for any kernel key press. return true if any key was pressed, otherwise false
// NOTE: the key press in question will be lost! only use when you want to check, but not wait for, a user key press
bool Kernal_AnyKeyEvent();


#endif /* KERNEL_H_ */
