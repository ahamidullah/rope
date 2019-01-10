#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <vector>
#include <string>
#include <functional>
#include <ctype.h>
#include <fstream>
#include <sstream>
#include <map>
#include <dirent.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "../core/file_offset.h"
#include "asset_packer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "image.cpp"

Asset_Id current_asset_id = 0;

FILE *log_file = NULL;

#define log_print(fmt, ...) log_print_actual(__FILE__, __LINE__, __func__, fmt, ## __VA_ARGS__)

void
log_print_actual(const char *file, int line, const char *func, const char *fmt, ...)
{
	if (log_file == NULL) {
		log_file = fopen("../../build/asset_packer.log", "w");
		assert(log_file);
	}

	va_list args;
	va_start(args, fmt);
	fprintf(log_file, "%s,%d in %s(): ", file, line, func);
	vfprintf(log_file, fmt, args);
	va_end(args);
}

// @TODO: Add shaders to the .ahh file.

struct Dir_Read {
	DIR *dir = NULL;
	struct dirent *ent = NULL;
};

std::string
get_base_name(std::string s)
{
	return s.substr(s.find_last_of('/')+1, s.find_last_of('.') - s.find_last_of('/') - 1);
}

std::string
to_upper(std::string s)
{
	for (auto &c : s)
		c = std::toupper(c);
	return s;
}

struct dirent *
get_each_file_in_dir(Dir_Read *dr, std::string &path)
{
	if (!dr->dir) { // First read.
		dr->dir = opendir(path.c_str());
		if (!dr->dir) {
			printf("**** Failed to open animation directory %s - %s\n", path.c_str(), strerror(errno));
			return NULL;
		}
	}
	if (dr->ent)
		path.resize(path.size() - strlen(dr->ent->d_name) - 1);
	while ((dr->ent = readdir(dr->dir))) {
		if (!strcmp(dr->ent->d_name, ".") || !strcmp(dr->ent->d_name, ".."))
			continue;
		path += (std::string("/") + std::string(dr->ent->d_name));
		return dr->ent;
	}
	return NULL;
}

Asset_Id
get_texture_id(std::map<std::string, Asset_Id> tex_map, std::string tex_path)
{
	auto itr = tex_map.find(tex_path);
	if (itr == tex_map.end()) {
		printf("\t\tAttempted to get non-existant texture %s.\n", tex_path.c_str());
		return ASSET_DOES_NOT_EXIST;
	}	
	printf("\t\tGot texture %s.\n", tex_path.c_str());
	return itr->second;
}

std::vector<std::string> asset_names;
std::vector<File_Offset> asset_offsets;
std::vector<Asset_Tags>  asset_tags;
std::vector<Asset_Type>  asset_types;

Asset_Type_Info asset_type_info[NUM_ASSET_TYPES] = {};

//std::map<std::string, Rectangle_U32> collider_map;
std::map<std::string, Asset_Id> texture_map;
std::vector<Asset_Id> dependencies;

FILE *asset_file;

std::stringstream
read_entire_file(std::string path)
{
	std::ifstream t(path);
	std::stringstream buffer;
	buffer << t.rdbuf();
	return buffer;
}

Asset_Type_Info
do_pack(std::string directory_relative_path, std::string base_directory_base_name, Asset_Type *at)
{
	struct dirent *ent;
	Dir_Read dir;

	Asset_Type_Info tinfo = {};
	tinfo.first_id = (Asset_Id)current_asset_id;

	std::string directory_base_name = get_base_name(directory_relative_path);
	const char *directory_absolute_path_c_string = realpath(directory_relative_path.c_str(), NULL);
	if (directory_absolute_path_c_string == NULL) {
		printf("Could not get absolute path of file %s -- %s.\n", directory_relative_path.c_str(), strerror(errno));
		exit(1);
	}

	std::string directory_absolute_path = std::string(directory_absolute_path_c_string);

	auto add_asset = [](std::string id, Asset_Type type, Asset_Id dependency, Asset_Tags tags, const void *header, size_t header_length, const void *data, size_t data_length, uint32_t *type_info_count) {
		File_Offset fo = ftell(asset_file);

		asset_names.push_back(id);
		asset_offsets.push_back(fo);
		asset_tags.push_back(tags);
		asset_types.push_back(type);

		dependencies.push_back(dependency);

		fwrite(header, header_length, 1, asset_file);
		fwrite(data, data_length, 1, asset_file);

		++current_asset_id;

		*type_info_count = *type_info_count + 1;
	};

	while ((ent = get_each_file_in_dir(&dir, directory_relative_path))) {
		if (ent->d_type == DT_DIR) {
			log_print("Recursing on subdirectory %s.\n", directory_relative_path.c_str());
			auto sub = do_pack(directory_relative_path, base_directory_base_name, at);
			tinfo.count += sub.count;
			continue;
		}
		if (ent->d_type != DT_REG) {
			log_print("Skipping file %s in directory %s, not a regular file.\n", ent->d_name, base_directory_base_name);
			continue;
		}

		std::string file_name = std::string(ent->d_name);
		std::string file_base_name = get_base_name(file_name);
		std::string file_absolute_path = directory_absolute_path + std::string("/") + file_name;
		std::string file_extension = file_name.substr(file_name.find_last_of('.') + 1, file_name.length() - file_name.find_last_of('.') - 1);

		log_print("Trying to load file %s.\n", file_absolute_path.c_str());

		File_Offset asset_start_offset = ftell(asset_file);

		Asset_Tags tags = 0;

		if (base_directory_base_name == "sprite") {
			if (file_extension != "json") {
				log_print("Skipping file %s: not a json file.\n", file_absolute_path.c_str());
				continue;
			}

			if (file_base_name == "tiles") {
				printf("Tagging sprite from file %s as tile.\n", file_name.c_str());
				tags |= SPRITE_TILE_ASSET_TAG;
			}

			std::string collider_json_file_path = directory_absolute_path + std::string("/") + file_base_name + "_collider.json";
			bool sprite_has_collider = false;
			std::stringstream collider_json_file_string;
			//Asset_File_Collider collider = ASSET_FILE_COLLIDER_DOES_NOT_EXIST;

			if (access(collider_json_file_path.c_str(), F_OK) != -1) {
				sprite_has_collider = true;

				collider_json_file_string = read_entire_file(collider_json_file_path);
			}

			*at = SPRITE_ASSET_TYPE;

			std::vector<Asset_File_Sprite_Frame> frames;
			std::string texture_path;
			std::stringstream json_file_string = read_entire_file(file_absolute_path);
			std::string word;
			File_Offset texture_offset;
			int32_t x = 0, y = 0, w = 0, h = 0, duration = 0, untrimmed_frame_w = 0, untrimmed_frame_h = 0, scissor_x = 0, scissor_y = 0, scissor_w = 0, scissor_h = 0;
			uint32_t texture_pixel_width = 0, texture_pixel_height = 0;

			bool found_texture_path = false, found_size = false;
			while (json_file_string >> word) {
				if (word == "\"image\":") {
					found_texture_path = true;

					json_file_string >> texture_path;
					texture_path = texture_path.substr(word.find_first_of('"') + 1, texture_path.find_last_of('"') - 1);
				}
				if (word == "\"size\":") {
					found_size = true;

					json_file_string >> word;
					json_file_string >> word;
					json_file_string >> texture_pixel_width;
					json_file_string >> word;
					json_file_string >> word;
					json_file_string >> texture_pixel_height;
				}
			}

			if (!found_texture_path || !found_size) {
				printf("Could not find the meta data for animation file %s.\n", file_absolute_path.c_str());
				exit(1);
			}

			if (texture_map.find(texture_path) == texture_map.end()) {
				printf("************\n*  ABORT   *\n************\nCould not find texture %s referenced in json file %s.\n", texture_path.c_str(), file_name.c_str());
				exit(1);
			}

			json_file_string.clear();
			json_file_string.seekg(0, json_file_string.beg);
			
			std::string asset_id_string;
			
			uint32_t collider_frame_number = 0;

			while (json_file_string >> word) {
				if (word == "\"tiles") {
					json_file_string >> word;
					std::string tile_name = word.substr(word.find_first_of('(') + 1, word.find_last_of(')') - 1);
					asset_id_string = "TILE_" + to_upper(tile_name) + std::string("_SPRITE");
				}
				if (word == "\"frame\":") {
					while (json_file_string >> word && word != "},") {
						if (word == "\"x\":") {
							json_file_string >> x;
						}
						if (word == "\"y\":") {
							json_file_string >> y;
						}
						if (word == "\"w\":") {
							json_file_string >> w;
						}
						if (word == "\"h\":") {
							json_file_string >> h;
						}
					}
				} else if (word == "\"spriteSourceSize\":") {
					json_file_string >> word;
					json_file_string >> word;
					json_file_string >> scissor_x;
					json_file_string >> word;
					json_file_string >> word;
					json_file_string >> scissor_y;
					json_file_string >> word;
					json_file_string >> word;
					json_file_string >> scissor_w;
					json_file_string >> word;
					json_file_string >> word;
					json_file_string >> scissor_h;
				} else if (word == "\"sourceSize\":") {
					json_file_string >> word;
					json_file_string >> word;
					json_file_string >> untrimmed_frame_w;
					json_file_string >> word;
					json_file_string >> word;
					json_file_string >> untrimmed_frame_h;
				} else if (word == "\"duration\":") {
					json_file_string >> duration;
					//frames.push_back({ x, y, w, h, scissor_x - (int32_t)collider_map[file_base_name].x, (untrimmed_frame_h - (scissor_y + scissor_h)) - (int32_t)collider_map[file_base_name].y, duration });
					frames.push_back({ x, y, w, h, scissor_x, untrimmed_frame_h - (scissor_y + scissor_h), duration });
					#if 0
					printf("%u %d %d\n", collider_map[file_base_name].x, frames.back().trim_x_offset, frames.back().trim_y_offset);

					if (file_base_name == "tiles") {
						Sprite_Header sh = { 1, texture_pixel_width, texture_pixel_height };

						Asset_Id dependency = texture_map[texture_path];

						add_asset(asset_id_string, *at, dependency, tags, &sh, sizeof(sh), &frames.back(), 1 * sizeof(Asset_File_Sprite_Frame), &tinfo.count);

						continue;
					}
					#endif
				} else if (word == "\"frameTags\":") {
					while (json_file_string >> word && word != "],") {
						if (word == "\"name\":") {
							json_file_string >> word;
							std::string animation_name = word.substr(word.find_first_of('"') + 1, word.find_last_of('"') - 1);
							int32_t from_frame = 0, to_frame = 0;
							json_file_string >> word;
							json_file_string >> from_frame;
							json_file_string >> word;
							json_file_string >> word;
							json_file_string >> to_frame;

							asset_id_string = to_upper(file_base_name) + std::string("_") + to_upper(animation_name) + std::string("_SPRITE");
							printf("Loaded sprite %s [%d, %d] in file %s.\n", asset_id_string.c_str(), from_frame, to_frame, file_absolute_path.c_str());

							uint32_t num_frames = to_frame - from_frame + 1;

							Asset_Id dependency = texture_map[texture_path];

							Asset_File_Collider collider = NO_ASSET_FILE_COLLIDER;

							if (sprite_has_collider) {
								uint32_t collider_untrimmed_frame_w = 0, collider_untrimmed_frame_h = 0, collider_scissor_x = 0, collider_scissor_y = 0, collider_scissor_w = 0, collider_scissor_h = 0;

								while ((collider_json_file_string >> word) && (collider_frame_number <= to_frame)) {
									if (word == (std::string("\"") + file_base_name)) {
										collider_json_file_string >> word;
										if (word != std::string("(collider)")) {
											continue;
										}

										collider_json_file_string >> collider_frame_number;
									} else if (word == "\"spriteSourceSize\":") {
										int32_t old_x = collider_scissor_x, old_y = collider_scissor_y, old_w = collider_scissor_w, old_h = collider_scissor_h;

										collider_json_file_string >> word;
										collider_json_file_string >> word;
										collider_json_file_string >> collider_scissor_x;
										collider_json_file_string >> word;
										collider_json_file_string >> word;
										collider_json_file_string >> collider_scissor_y;
										collider_json_file_string >> word;
										collider_json_file_string >> word;
										collider_json_file_string >> collider_scissor_w;
										collider_json_file_string >> word;
										collider_json_file_string >> word;
										collider_json_file_string >> collider_scissor_h;

										if (collider != NO_ASSET_FILE_COLLIDER) {
											if (old_x != collider_scissor_x || old_y != collider_scissor_y || old_w != collider_scissor_w || old_h != collider_scissor_h ) {
												printf("*************** ERROR: Detected a collider change in the middle of a sprite. We don't support this yet...\n*\n*\n");
												exit(1);
											}
										}
									} else if (word == "\"sourceSize\":") {
										collider_json_file_string >> word;
										collider_json_file_string >> word;
										collider_json_file_string >> collider_untrimmed_frame_w;
										collider_json_file_string >> word;
										collider_json_file_string >> word;
										collider_json_file_string >> collider_untrimmed_frame_h;

										collider = { collider_scissor_x, collider_untrimmed_frame_h - (collider_scissor_y + collider_scissor_h), collider_scissor_w, collider_scissor_h };
									}
								}
							}

							printf("Adding collider: %u %u %u %u.\n", collider.x, collider.y, collider.w, collider.h);
							Sprite_Header sh = { num_frames, collider, texture_pixel_width, texture_pixel_height };

							add_asset(asset_id_string, *at, dependency, tags, &sh, sizeof(sh), &frames[from_frame], num_frames * sizeof(Asset_File_Sprite_Frame), &tinfo.count);
						}
					}
				}
			}

			if (!json_file_string.eof()) {
				printf("**** Failed to parse json file %s -- %s.\n", file_absolute_path.c_str(), strerror(errno));
				exit(1);
			}
		} else if (base_directory_base_name == "texture") {
			if (file_extension != "png") {
				log_print("Skipping file %s: not a png file.\n", file_absolute_path.c_str());
				continue;
			}

			*at = TEXTURE_ASSET_TYPE;

			texture_map[file_absolute_path] = current_asset_id;

			Asset_Id dependency = ASSET_DOES_NOT_EXIST;

			File_Offset texture_offset = ftell(asset_file);
			int texture_width, texture_height, texture_channels;
			std::string texture_path = file_absolute_path;

			stbi_uc* pixels = stbi_load(texture_path.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
			if (!pixels) {
				printf("**** Failed to load image %s.\n", texture_path.c_str());
				exit(1);
			}

			// @TODO: Should be sprite/animation header or something.
			Texture_Header tex_header;
			tex_header.bytes_per_pixel = 4;
			tex_header.pixel_width     = texture_width;
			tex_header.pixel_height    = texture_height;

			printf("Loaded texture %s, width: %d, height: %d\n", texture_path.c_str(), texture_width, texture_height);

			std::string asset_id_string = to_upper(file_base_name).c_str() + std::string("_TEXTURE");

			fseek(asset_file, texture_offset, SEEK_SET);
			add_asset(asset_id_string, *at, dependency, tags, &tex_header, sizeof(tex_header), pixels, texture_width * texture_height * 4, &tinfo.count);
		} else {
			log_print("Skipping unrecognized asset directory %s.\n", base_directory_base_name.c_str());
		}
	}

	printf("\n");

	tinfo.one_past_last_id = (Asset_Id)current_asset_id;
	return tinfo;
}

void
pack_asset_directory(std::string directory_relative_path)
{
	Asset_Type at = NUM_ASSET_TYPES;

	std::string directory_base_name = get_base_name(directory_relative_path);
	auto new_ati = do_pack(directory_relative_path, directory_base_name, &at);

	assert(at != NUM_ASSET_TYPES);

	asset_type_info[at] = new_ati;
}

const char *
load_graphics_directory(std::string path, File_Offset asset_start_offset)
{
	int texture_width, texture_height, texture_channels;

	if (path.substr(path.find_last_of('.') + 1) != "gif" && path.substr(path.find_last_of('.') + 1) != "png" && path.substr(path.find_last_of('.') + 1) != "bmp")
		return NULL;

	std::string json_file_path = path.substr(0, path.find_last_of('.') + 1) + "json";
	if (access(json_file_path.c_str(), F_OK) != -1) {
		//FILE *json_file_ptr = fopen(json_file_path.c_str(), "r");
		//std::string search;
		//search.resize(100);
		//fread(search.c_str(), 100, 1, json_file_ptr);
		//search.find("\"frame\":");
	}

	stbi_uc* pixels = stbi_load(path.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
	if (!pixels) {
		printf("****** Failed to load file %s.\n", path.c_str());
		return NULL;
	}

	// @TODO: Should be sprite/animation header or something.
	Texture_Header tex_header;
	tex_header.bytes_per_pixel = 4;
	tex_header.pixel_width = texture_width;
	tex_header.pixel_height = texture_height;
	//tex_header.collider = NO_ASSET_COLLIDER;

	// @TODO: Could potentially make the asset file smaller / faster to load if we stored these in a
	// seperate table.
	//if (collider_map.find(get_base_name(path)) != collider_map.end())
		//tex_header.collider = collider_map[get_base_name(path)];

	fwrite(&tex_header, sizeof(tex_header), 1, asset_file);
	fwrite(pixels, texture_width * texture_height * 4, 1, asset_file);

	return "SPRITE";
}

// @TODO: Pack in the shaders, too.
// @TODO: This won't work for large file (>2GB). ftell might not work properly with file larger than 2GB because it returns a signed long as the offset.
// Might have to use some OS specific offset query function.
int
main(int, char **)
{
	Asset_File_Footer aff;

	//gen_header_file = fopen("../assets.h", "w+");
	//if (!gen_header_file) {
		//printf("**** Failed to open gen_header_file - %s\n", strerror(errno));
		//return 1;
	//}
	//fprintf(gen_header_file, "#ifndef __ASSETS_H__\n");
	//fprintf(gen_header_file, "#define __ASSETS_H__\n\n");
	//fprintf(gen_header_file, "#include \"asset_packer/asset_packer.h\"\n\n");

	asset_file = fopen("../../build/assets.ahh", "wb");
	if (!asset_file) {
		printf("**** Failed to open asset file - %s\n", strerror(errno));
		return 1;
	}

	//fprintf(gen_header_file, "enum Asset_Id : Asset_Id_Type {\n");

	//File_Offset textures_start = asset_offsets.size();
	struct dirent *ent;
	Dir_Read dir;
	std::string path = "../../data/colliders";
	while ((ent = get_each_file_in_dir(&dir, path))) {
		if (ent->d_type != DT_REG) {
			printf("Skipping file %s, not a regular file.\n", ent->d_name);
			continue;
		}

		int texture_width, texture_height, texture_channels;
		stbi_uc* pixels = stbi_load(path.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
		if (!pixels)
			return 0;

		Asset_File_Collider collider;

		uint32_t col_l = UINT32_MAX, col_r = UINT32_MAX, col_t = UINT32_MAX, col_b = UINT32_MAX;
		for (int i = 0; i < texture_height; ++i) {
			uint32_t row_l = UINT32_MAX, row_r = UINT32_MAX;
			for (int j = 0; j < texture_width; ++j) {
				unsigned char r = *pixels++;
				unsigned char g = *pixels++;
				unsigned char b = *pixels++;
				unsigned char a = *pixels++;

				if (!(r == 0 && g == 0 && b == 0)) {
					if (row_l == UINT32_MAX) {
						row_l = j;
						row_r = j;
					} else if (row_r != UINT32_MAX && row_r != j - 1) {
						printf("Discontinous collider in file %s.\n", path.c_str());
						return 1;
					} else {
						++row_r;
					}
				}
			}
			if (col_l == UINT32_MAX && row_l != UINT32_MAX) {
				col_l = row_l;
				col_r = row_r;
				col_t = i;
			}
			if (col_t != UINT32_MAX && col_b == UINT32_MAX) {
				if ((row_l == UINT32_MAX && row_r == UINT32_MAX) || i == texture_height - 1) {
					col_b = i;
				} else if (row_l != col_l || row_r != col_r) {
					printf("Non-rectangular collider in file %s.\n", path.c_str());
					return 1;
				}
			}
		}

		if (col_l == UINT32_MAX && col_r == UINT32_MAX && col_t == UINT32_MAX && col_b == UINT32_MAX)
			collider = {0,0,0,0};
		else
			collider = {col_l, texture_height - (col_b + 1), col_r - col_l + 1, col_b - col_t + 1};
		printf("Loaded collider %u %u %u %u %u\n", col_l, col_r, col_t, col_b, texture_height - (col_b + 1));
		//tex_header.bytes_per_row = tex->pitch;
		//fwrite(&tex_header, sizeof(tex_header), 1, asset_file);
		//fwrite(pixels, texture_width * texture_height * 4, 1, asset_file);
	}

	printf("\n");

	pack_asset_directory("../../data/texture");

	pack_asset_directory("../../data/sprite");

	//fprintf(gen_header_file, "\tNUM_ASSETS\n");
	//fprintf(gen_header_file, "};\n\n");

	aff.num_assets = current_asset_id;

	aff.asset_offsets_start = ftell(asset_file);
	for (auto ai : asset_offsets) {
		//printf("Writing offset: %lu\n", ai.offset);
		fwrite(&ai, sizeof(ai), 1, asset_file);
	}

	aff.asset_names_start = ftell(asset_file);
	for (uint32_t i = 0; i < asset_names.size(); ++i) {
		uint32_t name_length = asset_names[i].length();
		fwrite(&name_length, sizeof(name_length), 1, asset_file);
		fwrite(&i, sizeof(i), 1, asset_file);
		// @TODO: Padding?
		fwrite(asset_names[i].c_str(), 1, asset_names[i].length(), asset_file);
	}

	aff.asset_tags_start = ftell(asset_file);
	for (auto at : asset_tags) {
		fwrite(&at, sizeof(at), 1, asset_file);
	}

	aff.asset_types_start = ftell(asset_file);
	for (auto at : asset_types) {
		fwrite(&at, sizeof(at), 1, asset_file);
	}

#if 0
	aff.asset_type_info_start = ftell(asset_file);
	for (auto ati : asset_type_info) {
		printf("ati.count %u\n", ati.count);
		fwrite(&ati, sizeof(ati), 1, asset_file);
		//fwrite(&ati.asset_type, sizeof(ati.asset_type), 1, asset_file);
		//fwrite(&ati.first_id, sizeof(ati.first_id), 1, asset_file);
		//fwrite(&ati.one_past_last_id, sizeof(ati.one_past_last_id), 1, asset_file);
	}
#endif

	aff.asset_dependencies_start = ftell(asset_file);
	for (auto st : dependencies) {
		printf("DEP: %u\n", st);
		fwrite(&st, sizeof(st), 1, asset_file);
	}

	fwrite(&aff, sizeof(aff), 1, asset_file);
}

