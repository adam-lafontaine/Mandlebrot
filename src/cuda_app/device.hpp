#pragma once

#include "../utils/types.hpp"

#include <cstddef>
#include <cassert>
#include <array>
#include <vector>


bool cuda_memcpy_to_device(const void* host_src, void* device_dst, size_t n_bytes);

bool cuda_memcpy_to_host(const void* device_src, void* host_dst, size_t n_bytes);


bool cuda_no_errors();

bool cuda_launch_success();


template <typename T>
class DeviceArray
{
public:
    T* data = nullptr;
    u32 n_elements = 0;
};


template <class T, size_t N>
bool copy_to_device(std::array<T, N> const& src, DeviceArray<T>& dst)
{
    assert(dst.data);
    assert(dst.n_elements);
    assert(dst.n_elements == src.size());

    auto bytes = N * sizeof(T);

    return cuda_memcpy_to_device(src.data(), dst.data, bytes);
}


template <typename T>
bool copy_to_device(std::vector<T> const& src, DeviceArray<T>& dst)
{
    assert(dst.data);
    assert(dst.n_elements);
    assert(dst.n_elements == src.size());

    auto bytes = src.size() * sizeof(T);

    return cuda_memcpy_to_device(src.data(), dst.data, bytes);
}


constexpr auto RGB_CHANNELS = 3u;
constexpr auto RGBA_CHANNELS = 4u;


typedef union Pixel
{
	struct
	{
        u8 blue;
		u8 green;
		u8 red;
		u8 alpha;		
	};

	u8 channels[RGBA_CHANNELS];

	u32 value;

} pixel_t;

using pixel_t = Pixel;


class Image
{
public:

	u32 width;
	u32 height;

	pixel_t* data;
};

using image_t = Image;


class DeviceImage
{
public:

    u32 width;
    u32 height;

    pixel_t* data;
};


bool copy_to_device(image_t const& src, DeviceImage const& dst);

bool copy_to_host(DeviceImage const& src, image_t const& dst);



class DeviceMatrix
{
public:
	u32 width;
	u32 height;

	u32* data_src;
    u32* data_dst;
};


class DeviceColorPalette
{
public:
    u8* channels[RGB_CHANNELS];

    u32 n_colors = 0;
};


template <size_t N>
bool copy_to_device(std::array< std::array<u8, N>, RGB_CHANNELS> const& src, DeviceColorPalette& dst)
{
    assert(dst.channels[0]);
    assert(dst.n_colors);
    assert(dst.n_colors == src[0].size());

    auto bytes = src[0].size() * sizeof(u8);
    for(u32 c = 0; c < RGB_CHANNELS; ++c)
    {
        if(!cuda_memcpy_to_device(src[c].data(), dst.channels[c], bytes))
        {
            return false;
        }
    }

    return true;
}


namespace device
{
    using u8 = uint8_t;

    class MemoryBuffer
    {
    public:
        u8* data = nullptr;
        size_t capacity = 0;
        size_t size = 0;
    };


    bool malloc(MemoryBuffer& buffer, size_t n_bytes);

    bool unified_malloc(MemoryBuffer& buffer, size_t n_bytes);

    bool free(MemoryBuffer& buffer);

    u8* push_bytes(MemoryBuffer& buffer, size_t n_bytes);

    bool pop_bytes(MemoryBuffer& buffer, size_t n_bytes);


    template <typename T>
    inline bool push_device_array(MemoryBuffer& buffer, DeviceArray<T>& arr, u32 n_elements)
    {
        auto data = push_bytes(buffer, n_elements * sizeof(T));

        if(data)
        {
            arr.n_elements = n_elements;
            arr.data = (T*)data;

            return true;
        }

        return false;
    }


    bool push_device_image(MemoryBuffer& buffer, DeviceImage& image, u32 width, u32 height);

    bool push_device_matrix(MemoryBuffer& buffer, DeviceMatrix& matrix, u32 width, u32 height);

    bool push_device_palette(MemoryBuffer& buffer, DeviceColorPalette& palette, u32 n_colors);

}