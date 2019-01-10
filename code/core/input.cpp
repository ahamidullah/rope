//Vec2i default_mouse_start_position = { screen_dimensions.x/2, screen_dimensions.y/2 };

// @TODO: Make the screen dimensions a true global and use it to set the default start position.
/*
Input
make_input(Vec2i mouse_start_position, float mouse_sensitivity = 0.1f)
{
	Input i;
	i.mouse = { mouse_start_position, {0, 0}, mouse_sensitivity, 0 };
	i.keyboard = { {0}, {0} };
	return i;
}

void debug_print(const char *fmt, ...);

void
input_key_down(Keyboard *kb, unsigned scancode, Key_Symbol sym)
{
}

void
input_key_up(Keyboard *kb, unsigned scancode)
{
	kb->keys[scancode] = false;
	kb->keys_toggle[scancode] = false;
}

bool
input_is_key_down(const Keyboard *kb, Key_Symbol ks)
{
	return kb->keys[platform_keysym_to_scancode(ks)];
}

// has the key moved from up to down? if so, unset it in keys_toggle
// if multiple people people are looking for the same key press, only the first will get it -- bad
// but I don't want to use callbacks so this will do for now
bool
input_was_key_pressed(Keyboard *kb, Key_Symbol ks)
{
	unsigned sc = platform_keysym_to_scancode(ks);
	//SDL_Scancode sc = SDL_GetScancodeFromKey(k);
	if (kb->keys_toggle[sc]) {
		kb->keys_toggle[sc] = false;
		return true;
	}
	return false;
}

void
input_mbutton_down(Mouse_Button button, Mouse *m)
{
	m->buttons |= (unsigned)button;
}

void
input_mbutton_up(Mouse_Button button, Mouse *m)
{
	m->buttons &= ~(unsigned)button;
}
*/

/*
void
input_update_mouse(Mouse *m)
{
	SDL_GetRelativeMouseState(&m->motion.x, &m->motion.y);
	SDL_GetMouseState(&m->pos.x, &m->pos.y);
}
*/

