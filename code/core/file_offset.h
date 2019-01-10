#ifndef __FILE_OFFSET_H
#define __FILE_OFFSET_H

// These get shared between cge and the asset packer, and they are platform independent. Thus, they get their own special little file.
typedef uint64_t File_Offset;
File_Offset FILE_OFFSET_ERROR = (File_Offset)-1;

#endif

