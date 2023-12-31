/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#ifdef WOLF_MEDIA_STB

#pragma once

#include <wolf.hpp>

namespace wolf::media
{

  class w_image_data
  {
  public:
    // constructor
    W_API w_image_data() noexcept = default;

    // move constructor
    W_API w_image_data(w_image_data &&p_other) noexcept;

    // move assignment operator.
    W_API w_image_data &operator=(w_image_data &&p_other) noexcept;

    // destructor
    W_API ~w_image_data() noexcept;

    uint8_t *raw_data = nullptr;
    int width = -1;
    int height = -1;
    int comp = -1;

  private:
    // copy constructor.
    w_image_data(const w_image_data &) = delete;
    // copy assignment operator.
    w_image_data &operator=(const w_image_data &) = delete;

    // move implementation
    void _move(w_image_data &&p_other) noexcept;
  };

} // namespace wolf::media

#endif
