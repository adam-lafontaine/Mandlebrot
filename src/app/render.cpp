#include "render.hpp"
#include "colors.hpp"
#include "../utils/index_range.hpp"

#include <cassert>
#include <algorithm>
#include <execution>
#include <cmath>
#include <functional>

// platform dependent e.g. win32_main.cpp
u32 platform_to_color_32(u8 red, u8 green, u8 blue);


static Range2Du32 get_range(mat_u32_t const& mat)
{
	Range2Du32 r{};
	r.x_begin = 0;
	r.x_end = mat.width;
	r.y_begin = 0;
	r.y_end = mat.height;

	return r;
}


static pixel_t to_platform_pixel(u8 red, u8 green, u8 blue)
{
	pixel_t p = {};
	p.value = platform_to_color_32(red, green, blue);

	return p;
}


static pixel_t to_platform_pixel(pixel_t const& p)
{
	return to_platform_pixel(p.red, p.green, p.blue);
}


static pixel_t to_rgb_64(u32 iterations, u32 max_iter, u32 rgb_option)
{
	if (iterations >= max_iter)
	{
		return to_platform_pixel(0, 0, 0);
	}

	auto i = iterations % 64;

	u8 color_map[] = { palettes64[0][i], palettes64[1][i], palettes64[2][i] };

	u32 c1 = 0;
	u32 c2 = 0;
	u32 c3 = 0;

	switch (rgb_option)
	{
	case 1:
		c1 = 0;
		c2 = 1;
		c3 = 2;
		break;
	case 2:
		c1 = 0;
		c2 = 2;
		c3 = 1;
		break;
	case 3:
		c1 = 1;
		c2 = 0;
		c3 = 2;
		break;
	case 4:
		c1 = 1;
		c2 = 2;
		c3 = 0;
		break;
	case 5:
		c1 = 2;
		c2 = 0;
		c3 = 1;
		break;
	case 6:
		c1 = 2;
		c2 = 1;
		c3 = 0;
		break;
	}

	return to_platform_pixel(color_map[c1], color_map[c2], color_map[c3]);
}


static pixel_t to_rgb_32(u32 iterations, u32 max_iter, u32 rgb_option)
{
	if (iterations >= max_iter)
	{
		return to_platform_pixel(0, 0, 0);
	}

	auto i = iterations % 32;

	u8 color_map[] = { palettes32[0][i], palettes32[1][i], palettes32[2][i] };

	u32 c1 = 0;
	u32 c2 = 0;
	u32 c3 = 0;

	switch (rgb_option)
	{
	case 1:
		c1 = 0;
		c2 = 1;
		c3 = 2;
		break;
	case 2:
		c1 = 0;
		c2 = 2;
		c3 = 1;
		break;
	case 3:
		c1 = 1;
		c2 = 0;
		c3 = 2;
		break;
	case 4:
		c1 = 1;
		c2 = 2;
		c3 = 0;
		break;
	case 5:
		c1 = 2;
		c2 = 0;
		c3 = 1;
		break;
	case 6:
		c1 = 2;
		c2 = 1;
		c3 = 0;
		break;
	}

	return to_platform_pixel(color_map[c1], color_map[c2], color_map[c3]);
}


static pixel_t to_rgb_16(u32 iterations, u32 max_iter, u32 rgb_option)
{
	if (iterations >= max_iter)
	{
		return to_platform_pixel(0, 0, 0);
	}

	auto i = iterations % 16;

	u8 color_map[] = { palettes16[0][i], palettes16[1][i], palettes16[2][i] };

	u32 c1 = 0;
	u32 c2 = 0;
	u32 c3 = 0;

	switch (rgb_option)
	{
	case 1:
		c1 = 0;
		c2 = 1;
		c3 = 2;
		break;
	case 2:
		c1 = 0;
		c2 = 2;
		c3 = 1;
		break;
	case 3:
		c1 = 1;
		c2 = 0;
		c3 = 2;
		break;
	case 4:
		c1 = 1;
		c2 = 2;
		c3 = 0;
		break;
	case 5:
		c1 = 2;
		c2 = 0;
		c3 = 1;
		break;
	case 6:
		c1 = 2;
		c2 = 1;
		c3 = 0;
		break;
	}

	return to_platform_pixel(color_map[c1], color_map[c2], color_map[c3]);
}


static void draw(image_t const& dst, AppState const& state)
{
	auto& mat = state.iterations;

	auto [mat_min, mat_max] = std::minmax_element(mat.begin(), mat.end());
	auto min = *mat_min;
	auto max = *mat_max;

	auto diff = max - min;

	auto const to_platform_color = [&](u32 i)
	{
		return to_rgb_64(i - min, diff, state.rgb_option);
	};

	std::transform(std::execution::par, mat.begin(), mat.end(), dst.begin(), to_platform_color);
}


static void for_each_row(mat_u32_t const& mat, std::function<void(u32 y)> const& func)
{
	UnsignedRange y_ids(0u, mat.height);
	auto const y_id_begin = y_ids.begin();
	auto const y_id_end = y_ids.end();

	std::for_each(std::execution::par, y_id_begin, y_id_end, func);
}


static void copy_left(mat_u32_t const& mat, u32 n_cols)
{
	u32 const x_len = mat.width - n_cols;

	auto const copy_row_part = [&](u32 y)
	{
		auto row = mat.row_begin(y);
		for (u32 ix = 0; ix < x_len; ++ix)
		{
			auto src_x = ix + n_cols;
			auto dst_x = src_x - n_cols;
			row[dst_x] = row[src_x];
		}
	};

	for_each_row(mat, copy_row_part);
}


static void copy_right(mat_u32_t const& mat, u32 n_cols)
{
	u32 const x_len = mat.width - n_cols;

	auto const copy_row_part = [&](u32 y)
	{
		auto row = mat.row_begin(y);
		for (u32 ix = 0; ix < x_len; ++ix)
		{
			auto src_x = x_len - 1 - ix;
			auto dst_x = src_x + n_cols;
			row[dst_x] = row[src_x];
		}
	};

	for_each_row(mat, copy_row_part);
}


static void copy(mat_u32_t const& mat, Vec2Di32 const& direction)
{
	auto up = direction.y < 0;
	auto right = direction.x > 0;

	auto const n_cols = static_cast<u32>(std::abs(direction.x));
	auto const n_rows = static_cast<u32>(std::abs(direction.y));

	if (n_cols == 0 && n_rows == 0)
	{
		return;
	}

	if (n_rows == 0)
	{
		if (right)
		{
			copy_right(mat, n_cols);
		}
		else
		{
			copy_left(mat, n_cols);
		}

		return;
	}

	u32 const x_len = mat.width - n_cols;
	u32 const y_len = mat.height - n_rows;

	u32 src_x_begin = right ? 0 : n_cols;
	u32 dst_x_begin = src_x_begin + direction.x;

	u32 src_y_begin = 0;
	i32 dy = 0;
	if (up)
	{
		src_y_begin = n_rows;
		dy = 1;
	}
	else
	{
		src_y_begin = y_len - 1;
		dy = -1;
	}

	for (u32 iy = 0; iy < y_len; ++iy)
	{
		auto src_y = src_y_begin + dy * iy;
		auto dst_y = src_y + direction.y;

		auto src_begin = mat.row_begin(src_y) + src_x_begin;
		auto dst_begin = mat.row_begin(dst_y) + dst_x_begin;

		auto src_end = src_begin + x_len;

		std::copy(std::execution::par, src_begin, src_end, dst_begin);
	}
}


static void mandelbrot(AppState& state)
{
	auto& dst = state.iterations;

	auto const max_iter = state.max_iter;
	constexpr r64 limit = 4.0;

	auto const x_pos = state.screen_pos.x;
	auto const y_pos = state.screen_pos.y;

	auto const min_re = MBT_MIN_X + x_pos;
	auto const min_im = MBT_MIN_Y + y_pos;

	auto const re_step = screen_width(state) / dst.width;
	auto const im_step = screen_height(state) / dst.height;

	auto const do_mandelbrot = [&](Range2Du32 const& range)
	{
		auto x_ids = UnsignedRange(range.x_begin, range.x_end);
		auto y_ids = UnsignedRange(range.y_begin, range.y_end);

		auto y_id_begin = y_ids.begin();
		auto y_id_end = y_ids.end();
		auto x_id_begin = x_ids.begin();
		auto x_id_end = x_ids.end();

		auto const do_row = [&](u32 y)
		{
			r64 const ci = min_im + y * im_step;
			auto row = dst.row_begin(y);

			auto const do_x = [&](u32 x)
			{
				u32 iter = 0;
				r64 const cr = min_re + x * re_step;

				r64 re = 0.0;
				r64 im = 0.0;
				r64 re2 = 0.0;
				r64 im2 = 0.0;
				r64 re2_old = 0.0;
				r64 im2_old = 0.0;

				while (iter < max_iter && re2 + im2 <= limit)
				{
					re2_old = re2;
					im2_old = im2;
					im = (re + re) * im + ci;
					re = re2 - im2 + cr;
					im2 = im * im;
					re2 = re * re;

					++iter;
				}

				--iter;

				row[x] = iter;
			};

			std::for_each(std::execution::par, x_id_begin, x_id_end, do_x);
		};

		std::for_each(std::execution::par, y_id_begin, y_id_end, do_row);
	};

	auto& shift = state.pixel_shift;

	auto do_left = shift.x > 0;
	auto do_top = shift.y > 0;
	//auto do_right = shift.x < 0;	
	//auto do_bottom = shift.y < 0;

	auto const n_cols = static_cast<u32>(std::abs(shift.x));
	auto const n_rows = static_cast<u32>(std::abs(shift.y));

	auto no_horizontal = n_cols == 0;
	auto no_vertical = n_rows == 0;

	auto r = get_range(dst);

	if (no_horizontal && no_vertical)
	{
		do_mandelbrot(r);
		return;
	}

	if (no_horizontal)
	{
		if (do_top)
		{
			r.y_end = n_rows;
		}
		else // if (do_bottom)
		{
			r.y_begin = dst.height - 1 - n_rows;
		}

		do_mandelbrot(r);
		return;
	}

	if (no_vertical)
	{
		if (do_left)
		{
			r.x_end = n_cols;
		}
		else // if (do_right)
		{
			r.x_begin = dst.width - 1 - n_cols;
		}

		do_mandelbrot(r);
		return;
	}

	auto r2 = r;

	if (do_top)
	{
		r.y_end = n_rows;
		r2.y_begin = n_rows;
	}
	else // if (do_bottom)
	{
		r.y_begin = dst.height - 1 - n_rows;
		r2.y_end = dst.height - 1 - n_rows;
	}

	if (do_left)
	{
		r2.x_end = n_cols;
	}
	else // if (do_right)
	{
		r2.x_begin = dst.width - 1 - n_cols;
	}

	do_mandelbrot(r);
	do_mandelbrot(r2);
}


void render(image_t const& dst, AppState& state)
{
	copy(state.iterations, state.pixel_shift);
	mandelbrot(state);

	draw(dst, state);
}