constexpr float DEGREES_TO_RADIANS (M_PI / 180.0f);

f32
clamp(f32 it, f32 minmax)
{
	if (it > minmax)
		return minmax;
	else if (it < -minmax)
		return -minmax;
	return it;
}

f32
clamp(f32 it, f32 min, f32 max)
{
	if (it > max)
		return max;
	else if (it < min)
		return min;
	return it;
}

// Quake approximation.
f32
inv_sqrt(f32 number)
{
	long i;
	f32 x2, y;
	const f32 threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * ( long * ) &y;
	i  = 0x5f3759df - ( i >> 1 );
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );

	return y;
}

inline f32
length2(V3 v)
{
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

inline f32
length2(V2 v)
{
	return v.x*v.x + v.y*v.y;
}

inline f32
length(V2 v)
{
	return sqrt(v.x*v.x + v.y*v.y);
}

inline V3
normalize(V3 v)
{
	return inv_sqrt(length2(v)) * v;
}

inline V2
normalize(V2 v)
{
	return inv_sqrt(length2(v)) * v;
}

inline V2
normal(V2 v)
{
	return V2{-v.y, v.x};
}

inline float
dot(V4 a, float *b)
{
	return a.x*b[0] + a.y*b[1] + a.z*b[2] + a.w*b[3];
}

inline float
dot(V3 a, V3 b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline f32
dot(V2 a, V2 b)
{
	return a.x*b.x + a.y*b.y;
}

inline V3
cross(V3 a, V3 b)
{
	return { a.y*b.z - b.y*a.z,
	         a.z*b.x - b.z*a.x,
	         a.x*b.y - b.x*a.y };
}

inline f32
cross(V2 a, V2 b)
{
	return a.x*b.y - b.x*a.y;
}

V2
project(V2 a, V2 b)
{
	return (dot(a, b) / length2(b)) * b;
}

s32
sign(s32 x) {
    return (x > 0) - (x < 0);
}

f32
sign(f32 x) {
    return (x > 0.0f) - (x < 0.0f);
}

inline f32
cos(V2 a, V2 b)
{
	//if (length2(a) == 0.0f)
	//	a = {0.0f, 1.0f};
	//if (length2(b) == 0.0f)
	//	b = {0.0f, 1.0f};
	return dot(a, b) * inv_sqrt(length2(a)) * inv_sqrt(length2(b)) * sign(b.x);
}

inline f32
sin(V2 a, V2 b)
{
	if (length2(b) == 0.0f)
		b = {1.0f, 0.0f};
	return cross(b, a) * inv_sqrt(length2(a)) * inv_sqrt(length2(b)) * sign(b.x);
}

inline V2
clamp(V2 v, f32 max)
{
	if (length2(v) > (max * max))
		return max * normalize(v);
	return v;
}

f32
round_to_nearest(f32 x, f32 factor)
{
	//printf("%.9g %.9g %.9g\n", x / factor, round(x / factor), round(x / factor) * factor);
	return round(x / factor) * factor;
}

f32
round_to_lowest(f32 x, f32 factor)
{
	return floor(x / factor) * factor;
}

V2
round_to_lowest(V2 v, f32 factor)
{
	v.x = round_to_lowest(v.x, factor);
	v.y = round_to_lowest(v.y, factor);

	return v;
}

f32
round_to_highest(f32 x, f32 factor)
{
	return ceil(x / factor) * factor;
}

V2
round_to_highest(V2 v, f32 factor)
{
	v.x = round_to_highest(v.x, factor);
	v.y = round_to_highest(v.y, factor);

	return v;
}

V2
round_to_nearest(V2 v, f32 factor)
{
	v.x = round_to_nearest(v.x, factor);
	v.y = round_to_nearest(v.y, factor);

	return v;
}

V2
round_to_nearest_pixel(V2 p)
{
	p.x = round_to_nearest(p.x, scaled_meters_per_pixel);
	p.y = round_to_nearest(p.y, scaled_meters_per_pixel);

	return p;
}

f32
round_to_nearest_pixel(f32 dim)
{
	return round_to_nearest(dim, scaled_meters_per_pixel);
}

inline M4
make_view_matrix(V2 center, V2 target)
{
	f32 x_offset = -center.x + (window_scaled_meter_width / 2.0f);
	f32 y_offset = -center.y + (window_scaled_meter_height / 2.0f);

	return M4{ 1, 0, 0, 0,
	           0, 1, 0, 0,
	           0, 0, 1, 0,
	           x_offset, y_offset, 0, 1 };
}

inline V2
make_2d_view_vector(V2 center)
{
	return (V2){ -center.x + (window_scaled_meter_width / 2.0f),
	             -center.y + (window_scaled_meter_height / 2.0f) };
}

inline M4
view_matrix(V3 position, V3 forward)
{
	V3 world_up = {0.0f, 0.0f, 1.0f};
	forward     = normalize(forward);
	V3 side     = normalize(cross(forward, world_up));
	V3 up       = normalize(cross(side, forward));
	return M4{ side.x, up.x, -forward.x, 0,
	           side.y, up.y, -forward.y, 0,
	           side.z, up.z, -forward.z, 0,
	           -dot(side, position), -dot(up, position), dot(forward, position), 1 };
}

inline M4
look_at(V3 camera_position, V3 look_at)
{
	return view_matrix(camera_position, look_at - camera_position);
}

inline M4
make_orthographic_projection(float left, float right, float bottom, float top)
{
	// Assume right handed world space (left handed after view transform).
	// Assume far = 1 and near = -1.
	return { 2.0f / (right - left), 0, 0, 0,
		0, 2.0f / (top - bottom), 0, 0,
		0, 0, -1.0f, 0,
		-(right + left) / (right - left), -(top + bottom) / (top - bottom), 0, 1.0f };
}

inline M4
perspective_matrix(float fovy, float aspect_ratio, float near, float far)
{
	float top = tan((fovy / 2) * DEGREES_TO_RADIANS) * near;
	float right = top * aspect_ratio;
	return M4{ (near / right), 0, 0, 0,
	           0, (near / top), 0, 0,
	           0, 0, (-(far + near) / (far - near)), -1.0f,
	           0, 0, ((-2*far*near) / (far - near)), 0 };
}

inline M4
inverse(M4 m)
{
	const V3 &a = (const V3 &)m[0];
	const V3 &b = (const V3 &)m[1];
	const V3 &c = (const V3 &)m[2];
	const V3 &d = (const V3 &)m[3];

	f32 x = m[0][3];
	f32 y = m[1][3];
	f32 z = m[2][3];
	f32 w = m[3][3];

	V3 s = cross(a, b);
	V3 t = cross(c, d);
	V3 u = a * y - b * x;
	V3 v = c * w - d * z;

	f32 inverse_determinant = 1.0f / (dot(s, v) + dot(t, u));
	s *= inverse_determinant;
	t *= inverse_determinant;
	u *= inverse_determinant;
	v *= inverse_determinant;

	V3 r0 = cross(b, v) + t * y;
	V3 r1 = cross(v, a) - t * x;
	V3 r2 = cross(d, u) + s * w;
	V3 r3 = cross(u, c) - s * z;

	return M4{r0.x, r1.x, r2.x, r3.x,
	          r0.y, r1.y, r2.y, r3.y,
	          r0.z, r1.z, r2.z, r3.z,
	          -dot(b, t), dot(a, t), -dot(d, s), dot(c, s)};
	/*return M4{r0.x, r0.y, r0.z, -dot(b, t),
	          r1.x, r1.y, r1.z, dot(a, t),
	          r2.x, r2.y, r2.z, -dot(d, s),
	          r3.x, r3.y, r3.z, dot(c, s)};*/
}

f32
find_point_half_plane(V2 p, V2 l0, V2 l1)
{
	return (p.x - l1.x) * (l0.y - l1.y) - (l0.x - l1.x) * (p.y - l1.y);
}

// @TODO: Rectangle intersection tests require special handling of how the rectangle edges are handled. Maybe make specific functions for each required case?
// @TODO: Rectangle intersection tests require special handling of how the rectangle edges are handled. Maybe make specific functions for each required case?
// @TODO: Rectangle intersection tests require special handling of how the rectangle edges are handled. Maybe make specific functions for each required case?
// @TODO: Rectangle intersection tests require special handling of how the rectangle edges are handled. Maybe make specific functions for each required case?

bool
intersect_point_rectangle(V2 p, f32 x, f32 y, f32 w, f32 h)
{
	return p.x >= x
	    && p.x <= x + w
	    && p.y >= y
	    && p.y <= y + h;
}

bool
intersect_point_rectangle(V2 p, Rectangle r)
{
	return p.x >= r.x
	    && p.x < r.x + r.w
	    && p.y >= r.y
	    && p.y < r.y + r.h;
}

bool
intersect_point_rectangle(f32 px, f32 py, Rectangle r)
{
	return px >= r.x
	    && px <= r.x + r.w
	    && py >= r.y
	    && py <= r.y + r.h;
}

bool
intersect_rectangle_rectangle(Rectangle a, Rectangle b)
{
	Rectangle minkowski_sum = { b.x - a.x - a.w,
				    b.y - a.y - a.h,
				    a.w + b.w,
				    a.h + b.h };
	
	f32 minkowski_l = minkowski_sum.x;
	f32 minkowski_r = minkowski_sum.x + minkowski_sum.w;
	f32 minkowski_b = minkowski_sum.y;
	f32 minkowski_t = minkowski_sum.y + minkowski_sum.h;

	if (minkowski_l < 0.0f && minkowski_r > 0.0f && minkowski_b < 0.0f && minkowski_t > 0.0f) {
		return true;
	}

	return false;
}

f32
intersect_point_line(V2 p, V2 line0, V2 line1)
{
	//if (sign(line0.x - p.x) == sign(line1.x - p.x) && sign(line0.y - p.y) == sign(line1.y - p.y))
		//return INFINITY;
	f32 u = ((p.x - line0.x) * (line1.x - line0.x) + (p.y - line0.y) * (line1.y - line0.y)) / (length(line1 - line0) * length(line1 - line0));
	if (u >= 0.0f && u <= 1.0f)
		return u;
	return INFINITY;
}

bool
intersect_point_triangle_ccw(V2 pt, V2 v1, V2 v2, V2 v3)
{
	auto sign = [](V2 p1, V2 p2, V2 p3)
	{
		return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
	};

	if (v1 == v2 && v2 == v3)
		return false;

	bool b1, b2, b3;

	b1 = sign(pt, v1, v2) < 0.0f;
	b2 = sign(pt, v2, v3) < 0.0f;
	b3 = sign(pt, v3, v1) < 0.0f;

	return (b1 == b2) && (b2 == b3);
}

bool
intersect_point_quad(V2 p, V2 c1, V2 c2, V2 c3, V2 c4)
{
	bool b1 = find_point_half_plane(p, c1, c2) < 0.0f;
	bool b2 = find_point_half_plane(p, c2, c3) < 0.0f;
	bool b3 = find_point_half_plane(p, c3, c4) < 0.0f;
	bool b4 = find_point_half_plane(p, c4, c1) < 0.0f;

	return (b1 == b2) && (b2 == b3) && (b3 == b4);
}

// Assumes there will always be an intersection. Use when guaranteed intersection.
f32
intersect_ray_line_dangerous(V2 ray_direction, V2 ray_origin, V2 line_start, V2 line_end)
{
	V2 q1 = ray_origin - line_start;
	V2 q2 = line_end - line_start;
	V2 q3 = { -ray_direction.y, ray_direction.x };

	f32 d = dot(q2, q3);

	if (fabs(d) < 0.000001f) {
		printf("------------------------------------------\n");
		print_v2(ray_direction);
		print_v2(ray_origin);
		print_v2(line_start);
		print_v2(line_end);
		print_v2(q1);
		print_v2(q2);
		print_v2(q3);
		printf("%f\n", d);
		_abort("Ray and line somehow parallel.");
	}

	f32 t1 = cross(q2, q1) / d;
	f32 t2 = dot(q1, q3) / d;


	// @TODO: Special cases? Ray origin inside of line segment? Probably corner cases when all points close together.
	// @TODO: Currently fails if the corner is on the very edge of triangle. "No intersection" triggers. Should just clamp t1 and t2.

	// This error case corresponds to when there is no intersection.
	// This should only ever occur as a result of floating point inaccuricies because we this function is guaranteed to intersect.
	// Thus, just fix the values to fall in the correct range.
	if (t1 <= 0.0f || (t2 <= 0.0f && t2 >= 1.0f)) {
		if (t1 <= 0.0f)
			t1 = 0.0f;
		if (t2 <= 0.0f)
			t2 = 0.0f;
		else if (t2 >= 1.0f)
			t2 = 1.0f;
		printf("------------------------------------------\n");
		print_v2(ray_direction);
		print_v2(ray_origin);
		print_v2(line_start);
		print_v2(line_end);
		print_v2(q1);
		print_v2(q2);
		print_v2(q3);
		printf("%f\n", d);
		printf("%f %f\n", t1, t2);
		printf("No intersection detected while resolving bends.");
	}

	return t2;
}

V2
point_on_line(V2 p, V2 line_start, V2 line_end)
{
	f32 len2 = length2(line_end - line_start);

	f32 u = (((p.x - line_start.x) * (line_end.x - line_start.x)) +
	         ((p.y - line_start.y) * (line_end.y - line_start.y))) /
	        len2;

	V2 intersection;
	intersection.x = line_start.x + u * (line_end.x - line_start.x );
	intersection.y = line_start.y + u * (line_end.y - line_start.y );

	return intersection;
}

f32
distance_point_line(V2 p, V2 line_start, V2 line_end)
{
	V2 pol = point_on_line(p, line_start, line_end);
	return length(p - pol);
}

f32
approach(f32 current, f32 target, f32 step)
{
	f32 previous_sign = sign(target - current);
	current += step * sign(target - current);
	if (sign(target - current) != previous_sign)
		current = target;
	return current;
}

V2
approach(V2 current, V2 target, f32 step)
{
	current.x = approach(current.x, target.x, step);
	current.y = approach(current.y, target.y, step);

	return current;
}

