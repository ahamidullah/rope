void *
emalloc(size_t size)
{
	void *result = malloc(size);
	if (!result) {
		_abort("Failed to allocated memory.");
	}

	return result;
}

String
read_entire_file(const char *path, Memory_Arena *ma)
{
	File_Handle fh = platform_open_file(path, O_RDONLY); // @TODO: Platform generic flags.
	if (fh == FILE_HANDLE_ERROR)
		return STRING_ERROR;
	File_Offset len = platform_seek_file(fh, 0, FILE_SEEK_END);
	if (len == FILE_OFFSET_ERROR)
		return STRING_ERROR;
	platform_seek_file(fh, 0, FILE_SEEK_START);
	char *buf = mem_alloc_array(char, (len+1), ma);
	// Read may return less bytes than requested, so we have to loop.
	File_Offset tot_read = 0, cur_read = 0;
	char *pos = buf;
	do {
		cur_read = platform_read_file(fh, len - tot_read, pos);
		tot_read += cur_read;
		pos += cur_read;
	} while (tot_read < len && cur_read != 0);
	if (tot_read != len)
		return STRING_ERROR;
	buf[len] = '\0';
	platform_close_file(fh);
	return String{buf, len};
}

char *
read_entire_file(const char *path)
{
	FILE *fh = fopen(path, "r");
	if (!fh) {
		_abort("Failed to open file %s -- %s.", path, strerror(errno));
	}

	File_Offset file_size = platform_get_file_size(path);
	
	char *buffer = (char *)emalloc(file_size + 1);
	
	size_t num_read_bytes = fread(buffer, file_size + 1, 1, fh);
	if (!feof(fh)) {
		_abort("Failed to read complete file %s (expected %lu bytes, got %lu) -- %s.", path, file_size, num_read_bytes, strerror(errno));
	}

	return buffer;
}

void
debug_print(const char *fmt, va_list args)
{
	char buf[4096];
	size_t nbytes_writ = vsprintf(buf, fmt, args);
	platform_debug_print(nbytes_writ, buf);
}

void
debug_print(const char *fmt, ...)
{
	char buf[4096];
	va_list arg_list;
	va_start(arg_list, fmt);
	size_t nbytes_writ = vsprintf(buf, fmt, arg_list);
	platform_debug_print(nbytes_writ, buf);
	va_end(arg_list);
}

// @TODO: Handle log types.
void
log_print_actual(Log_Type lt, const char *file, int line, const char *func, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	if (lt != STANDARD_LOG) {
		debug_print("Error: ");
	}
	debug_print("%s,%d in %s: ", file, line, func);
	debug_print(fmt, args);
	debug_print("\n");
	va_end(args);
}

void
_abort_actual(const char *file, int line, const char *func, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	debug_print("!!!!!!!!!!!!!!!!!!!!!!!!!!PROGRAM ABORT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n*\n*\n*\n");
	debug_print("Error: %s,%d in %s: ", file, line, func);
	debug_print(fmt, args);
	debug_print("\n*\n*\n*\n");
	va_end(args);

#ifdef DEBUG
	assert(0);
#else
	_exit(1);
#endif
}

Color
random_color()
{
	float r = rand() / (float)RAND_MAX;
	float g = rand() / (float)RAND_MAX;
	float b = rand() / (float)RAND_MAX;
	return Color{r, g, b};
}

void
timer_set(u32 wait_time, Timer *t)
{
	t->wait_time  = wait_time;
	t->start_time = platform_get_time_ms();
}

bool
timer_check_one_shot(Timer *t)
{
	if (t->start_time == 0) {
		return true;
	}

	u32 current_time = platform_get_time_ms();

	if ((current_time - t->start_time) >= t->wait_time) {
		t->start_time = 0;
		return true;
	}

	return false;
}

bool
timer_check_repeating(Timer *t)
{
	u32 current_time = platform_get_time_ms();

	if ((current_time - t->start_time) >= t->wait_time) {
		t->start_time = current_time;
		return true;
	}

	return false;
}

template<typename T, bool initialize_to_zero = true>
Static_Array<T>
make_static_array(size_t capacity, size_t initial_size)
{
	assert(initial_size <= capacity);

	Static_Array<T> a;
	a.data = (T *)malloc(sizeof(T) * capacity);
	if (initialize_to_zero)
		memset(a.data, 0, capacity * sizeof(T));
	a.capacity = capacity;
	a.size = initial_size;
	return a;
}

template<typename T>
void
static_array_add(Static_Array<T> *a, T e)
{
	assert(a->size < a->capacity);
	a->data[a->size++] = e;
}

template<typename T, bool initialize_to_zero = true>
Array<T>
make_array(size_t initial_capacity, size_t initial_size)
{
	assert(initial_size <= initial_capacity);

	Array<T> a;
	a.data = (T *)malloc(sizeof(T) * initial_capacity);
	if (initialize_to_zero)
		memset(a.data, 0, initial_capacity * sizeof(T));
	a.capacity = initial_capacity;
	a.size = initial_size;
	return a;
}

template<typename T>
void
array_remove(Array<T> *a, size_t index)
{
	assert(index >= 0 && index < a->size);

	for (u32 i = index + 1; i < a->size; ++i) {
		a->data[i - 1] = a->data[i];
	}

	a->size -= 1;
}

template<typename T>
void
array_remove_last(Array<T> *a)
{
	assert(a->size > 0);
	array_remove(a, a->size - 1);
}

template<typename T>
T
array_get_last(Array<T> *a)
{
	assert(a->size > 0);
	return (*a)[a->size - 1];
}

template<typename T>
void
array_add(Array<T> *a, T e)
{
	a->data[a->size++] = e;

	if (a->size >= a->capacity) {
		a->data = (T *)realloc(a->data, sizeof(T) * ((a->capacity * 2) + 1));
	}
}

template<typename T>
void
array_reset(Array<T> *a)
{
	a->size = 0;
}

