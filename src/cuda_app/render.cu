#include "render.hpp"
#include "cuda_def.cuh"

constexpr int THREADS_PER_BLOCK = 1024;

constexpr int calc_thread_blocks(u32 n_elements)
{
    return (n_elements + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
}


class MandelbrotProps
{
public:
    u32 max_iter;
    r64 min_re;
    r64 min_im;
    r64 re_step;
    r64 im_step;
    DeviceMatrix iterations;
};


GPU_KERNAL
static void gpu_mandelbrot(MandelbrotProps props)
{
    auto const width = props.iterations.width;
    auto const height = props.iterations.height;
    auto n_elements = width * height;
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    if (i >= n_elements)
    {
        return;
    }

    auto y = i / width;
    auto x = i - y * width;

    r64 const ci = props.min_im + y * props.im_step;
    u32 iter = 0;
    r64 const cr = props.min_re + x * props.re_step;

    r64 re = 0.0;
    r64 im = 0.0;
    r64 re2 = 0.0;
    r64 im2 = 0.0;

    while (iter < props.max_iter && re2 + im2 <= 4.0)
    {
        im = (re + re) * im + ci;
        re = re2 - im2 + cr;
        im2 = im * im;
        re2 = re * re;

        ++iter;
    }

    props.iterations.data[i] = iter - 1;
}



GPU_KERNAL
static void gpu_set_color(DeviceImage image)
{
    auto const width = image.width;
    auto const height = image.height;
    auto n_pixels = width * height;
    int i = blockDim.x * blockIdx.x + threadIdx.x;
    if (i >= n_pixels)
    {
        return;
    }

    // i = y * width + x

    pixel_t p = {};
    p.alpha = 255;
    p.red = 0;
    p.green = 0;
    p.blue = 0;

    auto y = i / width;
    auto x = i - y * width;

    if(y < height / 3)
    {
        p.red = 255;
    }
    else if(y < height * 2 / 3)
    {
        p.green = 255;
    }
    else
    {
        p.blue = 255;
    }

    if(x < width / 3)
    {
        p.red = 255;
    }
    else if(x < width * 2 / 3)
    {
        p.green = 255;
    }
    else
    {
        p.blue = 255;
    }

    image.data[i] = p;
}





void render(AppState& state)
{
    auto& d_screen = state.device.pixels;
    u32 n_pixels = d_screen.width * d_screen.height;
    int blocks = calc_thread_blocks(n_pixels);
    
    MandelbrotProps m_props{};
    m_props.max_iter = state.max_iter;
	m_props.min_re = MBT_MIN_X + state.screen_pos.x;
	m_props.min_im = MBT_MIN_Y + state.screen_pos.y;
	m_props.re_step = screen_width(state) / d_screen.width;
	m_props.im_step = screen_height(state) / d_screen.height;
    m_props.iterations = state.device.iterations;    

    bool proc = cuda_no_errors();
    assert(proc);

    gpu_mandelbrot<<<blocks, THREADS_PER_BLOCK>>>(m_props);

    proc &= cuda_launch_success();
    assert(proc);

    gpu_set_color<<<blocks, THREADS_PER_BLOCK>>>(d_screen);

    proc &= cuda_launch_success();
    assert(proc);

    auto& h_screen = state.screen_buffer;
    proc &= copy_to_host(d_screen, h_screen);
    assert(proc);
}