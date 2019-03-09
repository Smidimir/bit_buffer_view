#include "bitbuf_view.h"

using namespace bbv;

/*
 * bitbuf_view::
 * */

bitbuf_view::
bitbuf_view(void* p_buffer, data_size_t n_buffer_size) noexcept
    : mp_buffer_begin{ reinterpret_cast<data_byte_t*>(p_buffer) }
    , mp_buffer_end{ mp_buffer_begin + n_buffer_size }
{
}

void
bitbuf_view::
set_buffer(void* p_buffer, data_size_t n_buffer_size) noexcept
{
  mp_buffer_begin = reinterpret_cast<data_byte_t*>(p_buffer);
  mp_buffer_end = mp_buffer_begin + n_buffer_size;
}

void*
bitbuf_view::
data() const noexcept
{
  return mp_buffer_begin;
}

data_size_t const
bitbuf_view::
size() const noexcept
{
  return mp_buffer_end - mp_buffer_begin;
}

bitbuf_view::iterator
bitbuf_view::
begin() noexcept
{
  return iterator{ *this };
}

bitbuf_view::iterator
bitbuf_view::end() noexcept
{
  return iterator{ *this, size() };
}


/*
 * bitbuf_view::iterator
 * */

bitbuf_view::iterator::
iterator(bitbuf_view& bit_buffer_view) noexcept
  : mp_bit_buffer_view{ &bit_buffer_view }
  , mp_block_it{ reinterpret_cast<data_block_t*>(bit_buffer_view.mp_buffer_begin) }
  , mn_bit_index{ 0 }
{

}

bitbuf_view::iterator::
iterator( bitbuf_view& bit_buffer_view
        , data_index_t n_byte_index
        , data_index_t n_bit_index ) noexcept
          : mp_bit_buffer_view{ &bit_buffer_view }
          , mp_block_it{ reinterpret_cast<data_block_t*>(bit_buffer_view.mp_buffer_begin)
                       + data_index_t{ n_byte_index / sn_data_block_byte_size } }
          , mn_bit_index{ (n_byte_index % data_index_t{ sn_data_block_byte_size }) * 8u + n_bit_index }
{
  if(mn_bit_index >= sn_data_block_bit_size)
  {
    mp_block_it += mn_bit_index / sn_data_block_bit_size;
    mn_bit_index = mn_bit_index % sn_data_block_bit_size;
  }
}

bitbuf_view::iterator::
iterator( bitbuf_view& bit_buffer_view
        , complex_byte_t const& complex_byte) noexcept
          : iterator( bit_buffer_view
                    , complex_byte.whole()
                    , complex_byte.part()
                    )
{
}

bitbuf_view::iterator
bitbuf_view::iterator::
operator + (complex_byte_t const& rhs) const noexcept
{
  return iterator{*mp_bit_buffer_view, get_byte_index() + rhs};
}

bitbuf_view::iterator
bitbuf_view::iterator::
operator - (complex_byte_t const& rhs) const noexcept
{
  return iterator{*mp_bit_buffer_view, get_byte_index() - rhs};
}

bool
bitbuf_view::iterator::
operator == (iterator const& rhs) const noexcept
{
  return mp_block_it  == rhs.mp_block_it
      && mn_bit_index == rhs.mn_bit_index;
}

bool
bitbuf_view::iterator::
operator != (iterator const& rhs) const noexcept
{
  return !(*this == rhs);
}

#define DECLARE_ITERATOR_COMPARISON_OPERATOR(_OP1_, _OP2_) \
    bool \
    bitbuf_view::iterator:: \
    operator _OP1_ (iterator const& rhs) const noexcept \
    { \
      return mp_block_it _OP2_ rhs.mp_block_it \
          || \
          (  mp_block_it == rhs.mp_block_it \
          && mn_bit_index  _OP1_  rhs.mn_bit_index ); \
    }

DECLARE_ITERATOR_COMPARISON_OPERATOR(<, <)
DECLARE_ITERATOR_COMPARISON_OPERATOR(<=, <)
DECLARE_ITERATOR_COMPARISON_OPERATOR(>, >)
DECLARE_ITERATOR_COMPARISON_OPERATOR(>=, >)

#undef DECLARE_ITERATOR_COMPARISON_OPERATOR

#define DECLARE_ITERATOR_TO_COMPLEX_BITE_INDEX_COMPARISON_OPERATOR(_OP_) \
    bool \
    bitbuf_view::iterator:: \
    operator _OP_ (complex_byte_t const& rhs) const noexcept \
    { \
      return get_byte_index() _OP_ rhs; \
    }

DECLARE_ITERATOR_TO_COMPLEX_BITE_INDEX_COMPARISON_OPERATOR(==)
DECLARE_ITERATOR_TO_COMPLEX_BITE_INDEX_COMPARISON_OPERATOR(!=)
DECLARE_ITERATOR_TO_COMPLEX_BITE_INDEX_COMPARISON_OPERATOR(<)
DECLARE_ITERATOR_TO_COMPLEX_BITE_INDEX_COMPARISON_OPERATOR(<=)
DECLARE_ITERATOR_TO_COMPLEX_BITE_INDEX_COMPARISON_OPERATOR(>)
DECLARE_ITERATOR_TO_COMPLEX_BITE_INDEX_COMPARISON_OPERATOR(>=)

#undef DECLARE_ITERATOR_TO_COMPLEX_BITE_INDEX_COMPARISON_OPERATOR

data_size_t
bitbuf_view::iterator::
read(void* data, data_size_t n_data_bits) noexcept
{
  auto const buffer_bytes = complex_byte_t(mp_bit_buffer_view->mp_buffer_end - mp_bit_buffer_view->mp_buffer_begin);
  auto const buffer_bytes_left = buffer_bytes - get_byte_index();
  auto const data_bytes_left = complex_byte_t{0, n_data_bits};
  auto const n_data_bits_left = [&]() -> data_size_t
  {
    if(buffer_bytes_left >= data_bytes_left)
      return n_data_bits;
    else
      return buffer_bytes_left.whole() * 8u + buffer_bytes_left.part();
  } ();

  auto constexpr ones = ~static_cast<data_block_t>(0);

  auto data_block_it = reinterpret_cast<data_block_t*>(data);

  auto const n_data_blocks = n_data_bits_left / sn_data_block_bit_size;
  auto const n_remaining_bits_in_block = sn_data_block_bit_size - mn_bit_index;

  auto const p_end = mp_block_it + n_data_blocks;
  while(mp_block_it != p_end)
  {
    *mp_block_it |= static_cast<data_block_t>(*data_block_it << mn_bit_index);
    ++mp_block_it;

    *mp_block_it |= static_cast<data_block_t>(*data_block_it >> n_remaining_bits_in_block);
    ++data_block_it;
  }

  // TODO: deal with end of buffer edge case
  auto const n_remaining_data_bits = n_data_bits_left % sn_data_block_bit_size;
  if(n_remaining_data_bits > 0)
  {
    auto const n_remaining_block_bits = sn_data_block_bit_size - mn_bit_index;
    if(n_remaining_data_bits <= n_remaining_block_bits)
    {
      auto const n_shift_size      = static_cast<data_block_t>(n_remaining_block_bits - n_remaining_data_bits);
      auto const n_data_mask       = static_cast<data_block_t>(ones >> n_shift_size);
      auto const n_data_to_process = static_cast<data_block_t>(*data_block_it << mn_bit_index);
      *mp_block_it                |= static_cast<data_block_t>(n_data_to_process & n_data_mask);
      mn_bit_index += n_remaining_data_bits;

      if(mn_bit_index == sn_data_block_bit_size)
      {
        ++mp_block_it;
        mn_bit_index = 0;
      }
    }
    else
    {
      auto const n_data_to_process_first = static_cast<data_block_t>(*data_block_it << mn_bit_index);
      *mp_block_it                      |= static_cast<data_block_t>(n_data_to_process_first);

      ++mp_block_it;
      mn_bit_index = static_cast<data_index_t>(n_remaining_data_bits - n_remaining_block_bits);

      auto const n_shift_size            = static_cast<data_block_t>(sn_data_block_bit_size - mn_bit_index);
      auto const n_data_mask             = static_cast<data_block_t>(ones >> n_shift_size);
      auto const n_data_to_process_last  = static_cast<data_block_t>(*data_block_it >> n_remaining_block_bits);
      *mp_block_it                      |= static_cast<data_block_t>(n_data_to_process_last & n_data_mask);
    }
  }

  return n_data_bits - n_data_bits_left;
}

data_size_t
bitbuf_view::iterator::
write(void* data, data_size_t n_data_bits) noexcept
{
  auto const buffer_bytes = complex_byte_t(mp_bit_buffer_view->mp_buffer_end - mp_bit_buffer_view->mp_buffer_begin);
  auto const buffer_bytes_left = buffer_bytes - get_byte_index();
  auto const data_bytes_left = complex_byte_t{0, n_data_bits};
  auto const n_data_bits_left = [&]() -> data_size_t
  {
    if(buffer_bytes_left >= data_bytes_left)
      return n_data_bits;
    else
      return buffer_bytes_left.whole() * 8u + buffer_bytes_left.part();
  } ();

  auto constexpr ones = ~static_cast<data_block_t>(0);

  auto data_block_it = reinterpret_cast<data_block_t*>(data);

  auto const n_data_blocks = n_data_bits_left / sn_data_block_bit_size;
  auto const n_remaining_bits_in_block = sn_data_block_bit_size - mn_bit_index;

  auto const p_end = mp_block_it + n_data_blocks;
  while(mp_block_it != p_end)
  {
    *data_block_it |= static_cast<data_block_t>(*mp_block_it >> mn_bit_index);
    ++mp_block_it;

    *data_block_it |= static_cast<data_block_t>(*mp_block_it << n_remaining_bits_in_block);
    ++data_block_it;
  }

  // TODO: deal with end of buffer edge case
  auto const n_remaining_data_bits = n_data_bits_left % sn_data_block_bit_size;
  if(n_remaining_data_bits > 0)
  {
    auto const n_remaining_block_bits = sn_data_block_bit_size - mn_bit_index;
    if(n_remaining_data_bits <= n_remaining_block_bits)
    {
      auto const n_shift_size      = static_cast<data_block_t>(n_remaining_block_bits - n_remaining_data_bits);
      auto const n_data_mask       = static_cast<data_block_t>(ones >> n_shift_size);
      auto const n_data_to_process = static_cast<data_block_t>(*mp_block_it >> mn_bit_index);
      *data_block_it              |= static_cast<data_block_t>(n_data_to_process & n_data_mask);
      mn_bit_index += n_remaining_data_bits;

      if(mn_bit_index == sn_data_block_bit_size)
      {
        ++mp_block_it;
        mn_bit_index = 0;
      }
    }
    else
    {
      auto const n_data_to_process_first = static_cast<data_block_t>(*mp_block_it >> mn_bit_index);
      *data_block_it                    |= static_cast<data_block_t>(n_data_to_process_first);

      ++mp_block_it;
      mn_bit_index = static_cast<data_index_t>(n_remaining_data_bits - n_remaining_block_bits);

      auto const n_shift_size            = static_cast<data_block_t>(sn_data_block_bit_size - n_remaining_data_bits);
      auto const n_data_mask             = static_cast<data_block_t>(ones >> n_shift_size);
      auto const n_data_to_process_last  = static_cast<data_block_t>(*mp_block_it << n_remaining_block_bits);
      *data_block_it                    |= static_cast<data_block_t>(n_data_to_process_last & n_data_mask);
    }
  }

  return n_data_bits - n_data_bits_left;
}

complex_byte_t
bitbuf_view::iterator::
get_byte_index() const noexcept
{
  return complex_byte_t( reinterpret_cast<data_byte_t*>(mp_block_it) - mp_bit_buffer_view->mp_buffer_begin
                       , mn_bit_index );
}
