#pragma once

#include "complex.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>


#define DEFAULT_SPECIAL_MEMBER_FUNCTION(_NAME_) \
_NAME_() noexcept = default; \
_NAME_(_NAME_ const&) noexcept = default; \
_NAME_(_NAME_ &&) noexcept = default; \
_NAME_& operator = (_NAME_ const&) noexcept = default; \
_NAME_& operator = (_NAME_&&) noexcept = default; \
~_NAME_() noexcept = default;

#define bitsizeof(x) (sizeof(x) * 8u)





namespace bbv
{

using data_index_t = std::size_t;
using data_size_t = std::size_t;
using data_byte_t = std::byte;

using complex_byte_t = complex<std::size_t, 8>;


class bitbuf_view
{
public:

  class iterator
  {
  public:
    using data_block_t  = std::uint_fast32_t;

    static data_size_t constexpr sn_data_block_byte_size = sizeof(data_block_t);
    static data_size_t constexpr sn_data_block_bit_size = bitsizeof(data_block_t);

  public:
    DEFAULT_SPECIAL_MEMBER_FUNCTION(iterator)

    explicit
    iterator( bitbuf_view& bit_buffer_view
            ) noexcept;

    explicit
    iterator( bitbuf_view& bit_buffer_view
            , data_index_t n_byte_index
            , data_index_t n_bit_index = data_index_t{ 0 }
            ) noexcept;

    explicit
    iterator( bitbuf_view& bit_buffer_view
            , complex_byte_t const& complex_byte
            ) noexcept;

    iterator
    operator + (complex_byte_t const& rhs) const noexcept;

    iterator
    operator - (complex_byte_t const& rhs) const noexcept;

#define DECLARE_COMPARISON(_TYPE_, _OP_) \
    bool \
    operator _OP_ (_TYPE_ const& rhs) const noexcept;

#define DECLARE_COMPARISON_SET(_TYPE_) \
    DECLARE_COMPARISON(_TYPE_, ==) \
    DECLARE_COMPARISON(_TYPE_, !=) \
    DECLARE_COMPARISON(_TYPE_, <) \
    DECLARE_COMPARISON(_TYPE_, <=) \
    DECLARE_COMPARISON(_TYPE_, >) \
    DECLARE_COMPARISON(_TYPE_, >=)

    DECLARE_COMPARISON_SET(iterator)
    DECLARE_COMPARISON_SET(complex_byte_t)

#undef DECLARE_COMPARISON_SET
#undef DECLARE_COMPARISON

    template<typename T>
    data_size_t
    read(T* data) noexcept;

    template<typename T>
    data_size_t
    read(T&& data) noexcept;

    data_size_t
    read(void* data, data_size_t n_read_bits) noexcept;

    template<typename T>
    data_size_t
    write(T* data) noexcept;

    template<typename T>
    data_size_t
    write(T&& data) noexcept;

    data_size_t
    write(void* data, data_size_t n_write_bits) noexcept;

    complex_byte_t
    get_byte_index() const noexcept;


  private:
    bitbuf_view* mp_bit_buffer_view = nullptr;

    data_block_t* mp_block_it = nullptr;
    data_index_t mn_bit_index = 0;

  };

public:
  DEFAULT_SPECIAL_MEMBER_FUNCTION(bitbuf_view)

  explicit
  bitbuf_view(void* p_buffer, data_size_t n_buffer_size) noexcept;

  void
  set_buffer(void* p_buffer, data_size_t n_buffer_size) noexcept;

  void*
  data() const noexcept;

  data_size_t const
  size() const noexcept;

  iterator
  begin() noexcept;

  iterator
  end() noexcept;

private:
  data_byte_t* mp_buffer_begin;
  data_byte_t* mp_buffer_end;
};

#undef DEFAULT_SPECIAL_MEMBER_FUNCTION


template<typename T>
data_size_t
bitbuf_view::iterator::
read(T* data) noexcept
{
  return this->read(data, bitsizeof(T));
}

template<typename T>
data_size_t
bitbuf_view::iterator::
read(T&& data) noexcept
{
  return this->read(&data, bitsizeof(std::decay_t<T>));
}

template<typename T>
data_size_t
bitbuf_view::iterator::
write(T* data) noexcept
{
  return this->write(data, bitsizeof(T));
}

template<typename T>
data_size_t
bitbuf_view::iterator::
write(T&& data) noexcept
{
  return this->write(&data, bitsizeof(std::decay_t<T>));
}

} // namespace bbv