#ifndef KERNEL_H_
#define KERNEL_H_


// wrapper to mkfs
//   pass the name you want for the formatted disk/SD card in name, and the drive number (0-2) in the drive param.
//   do NOT prepend the path onto name. 
// return negative number on any error
int __fastcall__ mkfs(const char* name, const char drive);

char GETIN(void);
void kernel_init(void);


#endif /* KERNEL_H_ */
