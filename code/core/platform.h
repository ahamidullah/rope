#ifndef __PLATFORM_H__
#define __PLATFORM_H__

// @TODO: Make platform_read a macro which takes a type and count. It shall read sizeof(type)*count bytes.

void platform_update_mouse_pos(Mouse *);
unsigned platform_keysym_to_scancode(Key_Symbol);
Program_State platform_handle_events(Input *);
void platform_swap_buffers();

void platform_debug_print(size_t, const char *);

File_Handle platform_open_file(const char *path, const char *modes);
bool platform_close_file(File_Handle);
File_Offset platform_seek_file(File_Handle, File_Offset, File_Seek_Relative);
File_Offset platform_read_file(File_Handle, size_t, void *);
bool platform_write_file(File_Handle, size_t, const void *);
File_Offset platform_size_file(File_Handle);
long platform_key_symbol_to_unicode(Key_Symbol keysym);

char *platform_get_memory(size_t);
void platform_free_memory(void *, size_t);
size_t platform_get_page_size();

Time_Spec platform_get_time();
long platform_time_diff(Time_Spec, Time_Spec, unsigned);

void platform_exit(int);

#endif
