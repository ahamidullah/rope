#ifndef __ASSET_PACKER_H__
#define __ASSET_PACKER_H__

#include <stdint.h>

// @TODO: Add values for where asset ids for each type begin and end so that functions can check whether passed in ids are valid.
// @TODO: Generate functions that translate from id to id string name at runtime.
// @TODO: Change the colliders to s32.

typedef uint32_t Asset_Id;
typedef uint32_t Asset_Tags;

Asset_Tags SPRITE_TILE_ASSET_TAG = 0x1;

struct Asset_File_Collider {
	uint32_t x, y, w, h;
	const Asset_File_Collider &operator=(Asset_File_Collider);
};

inline bool
operator==(Asset_File_Collider a, Asset_File_Collider b)
{
	return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

inline bool
operator!=(Asset_File_Collider a, Asset_File_Collider b)
{
	return !(a==b);
}

inline const Asset_File_Collider & Asset_File_Collider::
operator=(Asset_File_Collider a)
{
	this->x = a.x;
	this->y = a.y;
	this->w = a.w;
	this->h = a.h;
	return (*this);
}

#define NO_ASSET_FILE_COLLIDER ((Asset_File_Collider){ UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX })

struct Texture_Header {
	uint32_t pixel_width;
	uint32_t pixel_height;
	uint32_t bytes_per_pixel;
	//Rectangle_U32 collider;
};

struct Sprite_Header {
	uint32_t num_frames;
	Asset_File_Collider collider;
	uint32_t texture_pixel_width;
	uint32_t texture_pixel_height;
};

struct Asset_File_Sprite_Frame {
	int32_t x;
	int32_t y;
	int32_t w;
	int32_t h;
	int32_t trim_x_offset;
	int32_t trim_y_offset;
	int32_t duration;
};

#define ASSET_DOES_NOT_EXIST (Asset_Id)-1
#define NO_ASSET_COLLIDER (Rectangle_U32){UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX}

enum Asset_Type {
	SPRITE_ASSET_TYPE,
	TEXTURE_ASSET_TYPE,
	SOUND_ASSET_TYPE,
	FONT_ASSET_TYPE,

	NUM_ASSET_TYPES
};

struct Asset_Type_Info {
	uint32_t   count            = 0;
	Asset_Id   first_id         = (Asset_Id)0;
	Asset_Id   one_past_last_id = (Asset_Id)0;
};

struct Asset_File_Footer {
	uint32_t    num_assets;

	File_Offset asset_names_start;
	File_Offset asset_types_start;
	File_Offset asset_offsets_start;
	File_Offset asset_tags_start;
	File_Offset asset_dependencies_start;
};

#endif

