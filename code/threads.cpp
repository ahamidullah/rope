Job_Queue job_queue;

void
gpu_make_texture_job_callback(void *job_data)
{
	Gpu_Make_Texture_Job *j = (Gpu_Make_Texture_Job *)job_data;

	*(j->output_gpu_texture_handle) = gpu_make_texture(j->gl_tex_unit, j->texture_format, j->pixel_format, j->pixel_width, j->pixel_height, j->pixels);
	*(j->asset_load_status)         = ASSET_LOADED;
}

void
load_asset_callback(void *callback_data)
{
	Load_Asset_Job *j = (Load_Asset_Job *)callback_data;

	switch (j->type) {
	case LOAD_ASE: {
		load_ase(j->ase.path);
	} break;
	case LOAD_TEXTURE: {
		load_texture(j->texture.path, j->texture.base_name);
	} break;
	case LOAD_SPRITE: {
		load_sprite(j->sprite.sprite_path, j->sprite.collider_path, j->sprite.base_name);
	} break;
	}
}

Thread_Job *
get_next_job(Job_Queue *jq)
{
	if (jq->read_head != jq->write_head) {
		u32 original_read_head = jq->read_head;

		if (original_read_head != jq->write_head) {
			u32 next_job_to_do = __sync_val_compare_and_swap(&jq->read_head, original_read_head, (original_read_head + 1) % MAX_JOBS);
			if (next_job_to_do == original_read_head) {
				return &jq->jobs[next_job_to_do];
			}
		}
	}

	return NULL;
}

void
do_all_jobs(Job_Queue *jq)
{
	for (Thread_Job *j = get_next_job(jq); j; j = get_next_job(jq)) {
		j->do_job_callback(j->job_data);
		++jq->num_jobs_completed;
	}
}

void *
job_thread_start(void *job_thread_data)
{
	auto gl_context = platform_make_job_thread_opengl_context();

	Job_Queue *jq = (Job_Queue *)job_thread_data;

	//File_Handle asset_file_handle = platform_open_file(asset_file_path, O_RDONLY);

	while (true) {
		do_all_jobs(jq);
		platform_wait_semaphore(&jq->semaphore);
	}
}

void
add_job(void *data, Do_Job_Callback callback, Job_Queue *jq)
{
	Thread_Job tj = { data, callback };
	jq->jobs[jq->write_head] = tj;
	jq->write_head = (jq->write_head + 1) % MAX_JOBS;

	platform_post_semaphore(&jq->semaphore);
}

#if 0
u32
job_get_queue_index_for_new_job()
{
	static u32 job_queue_index = 0;
	
	auto result = job_queue_index;

	job_queue_index = (job_queue_index + 1) % NUM_JOB_THREADS;

	return result;
}
#endif

void
add_gpu_make_texture_job(u32 gl_tex_unit, s32 texture_format, s32 pixel_format, s32 pixel_width, s32 pixel_height, u8 *pixels, Asset_Load_Status *als, Gpu_Texture_Handle *tid)
{
	Gpu_Make_Texture_Job *j = (Gpu_Make_Texture_Job *)malloc(sizeof(Gpu_Make_Texture_Job));
	j->gl_tex_unit = gl_tex_unit;
	j->texture_format = texture_format;
	j->pixel_format = pixel_format;
	j->pixel_width = pixel_width;
	j->pixel_height = pixel_height;
	j->pixels = pixels;
	j->asset_load_status = als;
	j->output_gpu_texture_handle = tid;

	add_job(j, gpu_make_texture_job_callback, &job_queue);
}

void *
get_job_queue()
{
	return &job_queue;
}

void
add_load_ase_job(const char *path)
{
	Load_Asset_Job *j = (Load_Asset_Job *)malloc(sizeof(Load_Asset_Job));
	j->ase.path = path;
	j->type = LOAD_ASE;

	add_job(j, load_asset_callback, &job_queue);
}

void
add_load_texture_job(const char *texture_directory, const char *base_name)
{
	Load_Asset_Job *j = (Load_Asset_Job *)malloc(sizeof(Load_Asset_Job));
	sprintf(j->texture.path, "%s/%s.png", texture_directory, base_name);
	sprintf(j->texture.base_name, "%s", base_name);
	j->type = LOAD_TEXTURE;

	add_job(j, load_asset_callback, &job_queue);
}

void
add_load_sprite_job(const char *json_directory, const char *base_name)
{
	Load_Asset_Job *j = (Load_Asset_Job *)malloc(sizeof(Load_Asset_Job));
	sprintf(j->sprite.sprite_path, "%s/%s.json", json_directory, base_name);
	sprintf(j->sprite.collider_path, "%s/%s_collider.json", json_directory, base_name);
	sprintf(j->sprite.base_name, "%s", base_name);
	j->type = LOAD_SPRITE;

	add_job(j, load_asset_callback, &job_queue);
}

