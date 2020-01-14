const char *asset_file_path = "../build/assets.ahh";

#define SPRITE_TILE_TAG (0x1)

template <typename T>
struct Asset_Catalog {
	Asset_Catalog(size_t max)
	{
		data          = make_static_array<T>(max, 0);
		names         = make_static_array<char *>(max, 0);
		tags          = make_static_array<u32>(max, 0);
		load_statuses = make_static_array<Asset_Load_Status>(max, 0);
	}

	Static_Array<T>                 data;
	Static_Array<char *>            names;
	Static_Array<u32>               tags;
	Static_Array<Asset_Load_Status> load_statuses;
};

#define MAX_SPRITES  256
#define MAX_TEXTURES 256

Asset_Catalog<Sprite_Asset>  sprite_catalog(MAX_SPRITES);
Asset_Catalog<Texture_Asset> texture_catalog(MAX_TEXTURES);

GLuint gpu_make_texture(u32 gl_tex_unit, s32 texture_format, s32 pixel_format, s32 pixel_width, s32 pixel_height, u8 *pixels);

void add_load_sprite_job(const char *, const char *);
void add_load_texture_job(const char *, const char *);
void add_load_ase_job(const char *);

template <typename T>
T *
get_asset(Asset_Catalog<T> *c, const char *name)
{
	for (u32 i = 0; i < c->names.size; ++i) {
		if (strcmp(name, c->names[i]) == 0) {
			if (c->load_statuses[i] == ASSET_UNLOADED) {
				log_print(MINOR_ERROR_LOG, "Tried to retreive asset that was not loaded yet: %s.", name);
				return NULL;
			}

			return &c->data[i];
		}
	}

	log_print(MINOR_ERROR_LOG, "Tried to retreive asset that does not exist: %s.", name);

	return NULL;
}

Sprite_Instance
make_sprite_instance(const char *name, bool animated = true)
{
	Sprite_Instance si;

	si.sprite_asset_id = 0;

	assert(strlen(name) < SPRITE_INSTANCE_NAME_BUFFER_LENGTH);
	strcpy(si.name, name);

	if (!animated) {
		si.current_frame = 0;
	}

	return si;
}

Sprite_Asset *
get_sprite(const char *name)
{
	return get_asset(&sprite_catalog, name);
}

Texture_Asset *
get_texture(const char *name)
{
	return get_asset(&texture_catalog, name);
	//return (Loaded_Texture *)get_asset(id, TEXTURE_ASSET_TYPE);
}

template <typename T>
void
add_asset(Asset_Catalog<T> *c, const T &a, u32 tags, const char *name)
{
	static_array_add(&c->data, a);
	char *name_copy = (char *)emalloc(strlen(name) + 1);
	strcpy(name_copy, name);
	static_array_add(&c->names, name_copy);
	static_array_add(&c->tags, tags);
	static_array_add(&c->load_statuses, ASSET_LOADED);
}

void
add_sprite(const Sprite_Asset &a, u32 tags, const char *name)
{
	add_asset(&sprite_catalog, a, tags, name);
}

void
add_texture(const Texture_Asset &a, u32 tags, const char *name)
{
	add_asset(&texture_catalog, a, tags, name);
}

struct Parse_Stream {
	char * source               = NULL;
	char * current_token        = NULL;
	size_t current_token_length = 0;
	s8     previous_found_delim = ' ';
};

bool
get_token(Parse_Stream *ps)
{
	if (ps->source == NULL) {
		return false;
	}

	if (ps->current_token != NULL) {
		*(ps->source) = ps->previous_found_delim;
		ps->source += 1;
	}

	while (*(ps->source) == ' ' && *(ps->source) != '\0') {
		ps->source += 1;
	}

	char *token_start = ps->source;

	if (*(ps->source) == '\0') {
		ps->source = NULL;
		return false;
	}

	ps->current_token = ps->source;

	while (*(ps->source) != ' ' && *(ps->source) != '\0') {
		ps->source += 1;
	}

	char *token_end = ps->source;

	ps->current_token_length = token_end - token_start;

	if (*(ps->source) == '\0') {
		ps->source = NULL;
	} else {
		ps->previous_found_delim = *(ps->source);
		*(ps->source) = '\0';
	}


	return true;
}

void
load_sprite(const char *sprite_path, const char *collider_path, const char *file_base_name)
{
	char *sprite_file   = read_entire_file(sprite_path);
	char *collider_file = read_entire_file(collider_path);

	char sprite_name[256];
	
	s32 frame_x = 0, frame_y = 0, frame_w = 0, frame_h = 0;
	s32 frame_offset_in_cel_x = 0, frame_offset_in_cel_y = 0;
	s32 cel_w = 0, cel_h = 0;
	s32 duration = 0;

	s32 texture_pixel_width = 0, texture_pixel_height = 0;

	Parse_Stream size_ps;
	size_ps.source = sprite_file;


	while (get_token(&size_ps)) {
		if (strcmp(size_ps.current_token, "\"size\":") == 0) {
			get_token(&size_ps);
			get_token(&size_ps);
			get_token(&size_ps);
			texture_pixel_width = strtol(size_ps.current_token, NULL, 10);
			get_token(&size_ps);
			get_token(&size_ps);
			texture_pixel_height = strtol(size_ps.current_token, NULL, 10);
		}
	}

	Parse_Stream ps;
	ps.source = sprite_file;

	Parse_Stream collider_ps;
	collider_ps.source = collider_file;

	u32 tags = 0;

	if (strcmp(file_base_name, "tiles") == 0) {
		tags |= SPRITE_TILE_TAG;
	}

	s32 collider_frame_number = 0;

	Array<Sprite_Frame> frames = make_array<Sprite_Frame>(32, 0);

	//for (char *word = strtok(file, " "); word; word = strtok(NULL, " ")) {
	while (get_token(&ps)) {
		if (strcmp(ps.current_token, "\"frame\":") == 0) {
			while (get_token(&ps) && strcmp(ps.current_token, "},\n")) {
				if (strcmp(ps.current_token, "\"x\":") == 0) {
					get_token(&ps);
					frame_x = strtol(ps.current_token, NULL, 10);
				}
				if (strcmp(ps.current_token, "\"y\":") == 0) {
					get_token(&ps);
					frame_y = strtol(ps.current_token, NULL, 10);
				}
				if (strcmp(ps.current_token, "\"w\":") == 0) {
					get_token(&ps);
					frame_w = strtol(ps.current_token, NULL, 10);
				}
				if (strcmp(ps.current_token, "\"h\":") == 0) {
					get_token(&ps);
					frame_h = strtol(ps.current_token, NULL, 10);
				}
			}
		} else if (strcmp(ps.current_token, "\"spriteSourceSize\":") == 0) {
			get_token(&ps);
			get_token(&ps);
			get_token(&ps);
			frame_offset_in_cel_x = strtol(ps.current_token, NULL, 10);
			get_token(&ps);
			get_token(&ps);
			frame_offset_in_cel_y = strtol(ps.current_token, NULL, 10);
		} else if (strcmp(ps.current_token, "\"sourceSize\":") == 0) {
			get_token(&ps);
			get_token(&ps);
			get_token(&ps);
			cel_w = strtol(ps.current_token, NULL, 10);
			get_token(&ps);
			get_token(&ps);
			cel_h = strtol(ps.current_token, NULL, 10);
		} else if (strcmp(ps.current_token, "\"duration\":") == 0) {
			get_token(&ps);
			duration = strtol(ps.current_token, NULL, 10);
			array_add(&frames, { { (f32)frame_x / (f32)texture_pixel_width,
					       (f32)frame_y / (f32)texture_pixel_height,
					       (f32)frame_w / (f32)texture_pixel_width,
					       (f32)frame_h / (f32)texture_pixel_height },
					     { (f32)frame_offset_in_cel_x * meters_per_pixel,
					       ((f32)cel_h - (frame_offset_in_cel_y + frame_h)) * meters_per_pixel },
					     (f32)frame_w * meters_per_pixel,
					     (f32)frame_h * meters_per_pixel,
					     duration });
		} else if (strcmp(ps.current_token, "\"frameTags\":") == 0) {
			while ((get_token(&ps)) && strcmp(ps.current_token, "]\n")) {
				if (strcmp(ps.current_token, "\"name\":") == 0) {
					get_token(&ps);

					size_t base_name_length = sprintf(sprite_name, "%s", file_base_name);

					// @HACK: Strip out the leading " and the trailing ", from animation names.
					size_t anim_name_length = strlen(&ps.current_token[1]);
					snprintf(sprite_name + base_name_length, anim_name_length, "_%s", &ps.current_token[1]);

					s32 from_frame = 0, to_frame = 0;
					get_token(&ps);
					get_token(&ps);
					from_frame = strtol(ps.current_token, NULL, 10);
					get_token(&ps);
					get_token(&ps);
					to_frame = strtol(ps.current_token, NULL, 10);

					u32 num_frames = to_frame - from_frame + 1;

					Sprite_Asset ls;
					ls.frames              = make_array<Sprite_Frame>(num_frames, 0);

					ls.texture_name = (char *)emalloc(base_name_length + 1);
					strcpy(ls.texture_name, file_base_name);

					for (s32 i = from_frame; i <= to_frame; ++i) {
						array_add(&ls.frames, frames[i]);
					}

					ls.collider = { 0.0f, 0.0f, 0.0f, 0.0f };

					s32 collider_untrimmed_frame_w = 0, collider_untrimmed_frame_h = 0, collider_scissor_x = 0, collider_scissor_y = 0, collider_scissor_w = 0, collider_scissor_h = 0;
					bool found_a_collider = false;

					while (get_token(&collider_ps) && (collider_frame_number <= to_frame)) {
						if (collider_ps.current_token[0] == '"' && (strcmp(&collider_ps.current_token[1], file_base_name) == 0)) {
							get_token(&collider_ps);
							if (strcmp(collider_ps.current_token, "(collider)")) {
								continue;
							}

							get_token(&collider_ps);
							collider_frame_number = strtol(collider_ps.current_token, NULL, 10);
						} else if (strcmp(collider_ps.current_token, "\"spriteSourceSize\":") == 0) {
							s32 old_x = collider_scissor_x, old_y = collider_scissor_y, old_w = collider_scissor_w, old_h = collider_scissor_h;

							get_token(&collider_ps);
							get_token(&collider_ps);
							get_token(&collider_ps);
							collider_scissor_x = strtol(collider_ps.current_token, NULL, 10);
							get_token(&collider_ps);
							get_token(&collider_ps);
							collider_scissor_y = strtol(collider_ps.current_token, NULL, 10);
							get_token(&collider_ps);
							get_token(&collider_ps);
							collider_scissor_w = strtol(collider_ps.current_token, NULL, 10);
							get_token(&collider_ps);
							get_token(&collider_ps);
							collider_scissor_h = strtol(collider_ps.current_token, NULL, 10);

							if (found_a_collider) {
								if (old_x != collider_scissor_x || old_y != collider_scissor_y || old_w != collider_scissor_w || old_h != collider_scissor_h ) {
									_abort("Detected a collider change in the middle of a sprite. We don't support this yet...");
								}
							}
						} else if (strcmp(collider_ps.current_token, "\"sourceSize\":") == 0) {
							get_token(&collider_ps);
							get_token(&collider_ps);
							get_token(&collider_ps);
							collider_untrimmed_frame_w = strtol(collider_ps.current_token, NULL, 10);
							get_token(&collider_ps);
							get_token(&collider_ps);
							collider_untrimmed_frame_h = strtol(collider_ps.current_token, NULL, 10);

							ls.collider = { collider_scissor_x * meters_per_pixel,
							                (collider_untrimmed_frame_h - (collider_scissor_y + collider_scissor_h)) * meters_per_pixel,
							                collider_scissor_w * meters_per_pixel,
							                collider_scissor_h * meters_per_pixel };
						}
					}

					add_sprite(ls, tags, sprite_name);
				}
			}
		}
	}
}

void
load_texture(const char *path, const char *base_name)
{
	int texture_width, texture_height, texture_channels;

	stbi_uc* pixels = stbi_load(path, &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
	if (!pixels) {
		_abort("Failed to load image %s.\n", path);
	}

	u32 num_texture_bytes = texture_width * texture_height * texture_channels;

	Texture_Asset t;
	t.gpu_handle = gpu_make_texture(GL_TEXTURE0, GL_RGBA, GL_RGBA, texture_width, texture_height, pixels);

	add_texture(t, 0, base_name);
}

void
load_ase(const char *asset_path)
{
	const char *json_directory    = "../build/sprite_data";
	const char *texture_directory = "../build/textures";

	static const s32 buffer_size = 256;

	char base_name[buffer_size];

	const char *base_name_start = strrchr(asset_path, '/') + 1;
	const char *base_name_end   = strrchr(base_name_start, '.');

	if (!base_name_end) {
		_abort("Could not find the '.' in asset file %s.\n", asset_path);
	}

	s32 base_name_length = base_name_end - base_name_start;

	if (base_name_length > buffer_size - 1) {
		_abort("base_name_length of size %d is too big for buffer of size %d.", base_name_length, buffer_size);
	}

	strncpy(base_name, base_name_start, base_name_length);

	base_name[base_name_length] = '\0';

	printf("Loading ase file %s\n", base_name);

	char buffer[buffer_size];

	char *aseprite_gen_command = buffer;

	sprintf(aseprite_gen_command, "aseprite -b --data %s/%s.json --sheet %s/%s.png --trim --list-tags --ignore-empty %s", json_directory, base_name, texture_directory, base_name, asset_path);
	system(aseprite_gen_command);

	sprintf(aseprite_gen_command, "aseprite -b --data %s/%s_collider.json --trim --ignore-empty --layer collider %s", json_directory, base_name, asset_path);
	system(aseprite_gen_command);

	//add_load_texture_job(texture_directory, base_name);
	//add_load_sprite_job(json_directory, base_name);
	char b1[256], b2[256], b3[256];
	sprintf(b1, "%s/%s.png", texture_directory, base_name);
	load_texture(b1, base_name);
	sprintf(b1, "%s/%s.json", json_directory, base_name);
	sprintf(b2, "%s/%s_collider.json", json_directory, base_name);
	load_sprite(b1, b2, base_name);
}

void
load_asset_file(const char *path)
{
	char extension[4];
	extension[0] = path[strlen(path) - 3];
	extension[1] = path[strlen(path) - 2];
	extension[2] = path[strlen(path) - 1];
	extension[3] = '\0';

	if (strcmp(extension, "ase") == 0) {
		load_ase(path);
	} else {
		_abort("Unknown asset file type %s from file %s.", extension, path);
	}
}

Thread_Job *get_next_job(Job_Queue *jq);
void do_all_jobs(Job_Queue *jq);

void
init_assets()
{
	load_asset_file("../data/sprites/player.ase");
	load_asset_file("../data/sprites/tiles.ase");

	/*
	u32 num_asset_loads_queued = 4;

	extern Job_Queue job_queue;
	do_all_jobs(&job_queue);

	while (job_queue.num_jobs_completed < num_asset_loads_queued) {
	}
	*/
}

