#pragma once


#include <cstdint>
#include <ratio>
#include <type_traits>
#include <tuple>


#define DEFAULT_SPECIAL_MEMBER_FUNCTION(_NAME_) \
_NAME_() noexcept = default; \
_NAME_(_NAME_ const&) noexcept = default; \
_NAME_(_NAME_ &&) noexcept = default; \
_NAME_& operator = (_NAME_ const&) noexcept = default; \
_NAME_& operator = (_NAME_&&) noexcept = default; \
~_NAME_() noexcept = default;


template <typename T>
auto constexpr bit_sizeof = sizeof(T) * 8u;


class bit_buffer_view
{
public:
  using data_index_t = std::size_t;
  using data_size_t = std::size_t;
  using data_byte_t = std::uint8_t;

public:

  template <typename TDataBlock>
  class iterator
  {
  public:


  public:
    using bit_buffer_view_t = bit_buffer_view;
    using data_index_t = bit_buffer_view_t::data_index_t;
    using data_size_t = bit_buffer_view_t::data_size_t;
    using data_byte_t = bit_buffer_view_t::data_byte_t;
    using data_block_t = TDataBlock;

    static data_size_t constexpr sn_data_block_byte_size = sizeof(data_block_t);
    static data_size_t constexpr sn_data_block_bit_size = bit_sizeof<data_block_t>;

  public:
    DEFAULT_SPECIAL_MEMBER_FUNCTION(iterator)

    explicit iterator( bit_buffer_view_t& bit_buffer_view
        , data_index_t n_byte_index = data_index_t{0}
        , data_index_t n_bit_index = data_index_t{0} ) noexcept
        : mp_bit_buffer_view{ &bit_buffer_view }
          , mp_block_it{ reinterpret_cast<data_block_t*>(bit_buffer_view.mp_buffer) + data_index_t{ n_byte_index / sn_data_block_byte_size } }
          , mn_bit_index{ (n_byte_index % data_index_t{ sn_data_block_byte_size }) * 8u }
    {
      if(mn_bit_index >= sn_data_block_bit_size)
      {
        mp_block_it += 1;
        mn_bit_index -= sn_data_block_bit_size;
      }
    }

    template <typename>
    friend class iterator;

    template <typename T>
    iterator& operator += (iterator<T> const& rhs) noexcept
    {
      auto const n_rhs_byte_index = rhs.get_byte_index();

      auto const n_rhs_block_index = n_rhs_byte_index / sn_data_block_byte_size;

      auto const n_bit_index = mn_bit_index + rhs.get_byte_bit_index() + (n_rhs_byte_index % sn_data_block_byte_size) * 8u;

      mp_block_it += n_rhs_block_index + n_bit_index / sn_data_block_bit_size;
      mn_bit_index =                     n_bit_index % sn_data_block_bit_size;

      return *this;
    }

    template <typename T>
    iterator& operator -= (iterator<T> const& rhs) noexcept
    {
      auto const n_rhs_byte_index = rhs.get_byte_index();

      auto const n_rhs_block_index = n_rhs_byte_index / sn_data_block_byte_size;

      auto const n_rsh_bit_index = rhs.get_byte_bit_index() + (n_rhs_byte_index % sn_data_block_byte_size) * 8u;

      if(mn_bit_index >= n_rsh_bit_index)
      {
        mp_block_it  -= n_rhs_block_index;
        mn_bit_index -= n_rsh_bit_index;
      }
      else
      {
        mp_block_it  -= data_size_t{1} + n_rhs_block_index;
        mn_bit_index  = sn_data_block_bit_size - (n_rsh_bit_index - mn_bit_index);
      }

      return *this;
    }

    bit_buffer_view_t* get_buffer_view() const noexcept
    {
      return mp_bit_buffer_view;
    }

    inline
    data_index_t get_block_index() const noexcept
    {
      return mp_block_it - reinterpret_cast<data_block_t*>(mp_bit_buffer_view->mp_buffer);
    }

    inline
    data_index_t get_block_bit_index() const noexcept
    {
      return mn_bit_index;
    }

    inline
    data_index_t get_byte_index() const noexcept
    {
      return data_index_t(reinterpret_cast<data_byte_t*>(mp_block_it) - mp_bit_buffer_view->mp_buffer) + (mn_bit_index / 8u);
    }

    inline
    data_index_t get_byte_bit_index() const noexcept
    {
      return mn_bit_index % 8u;
    }

    template <typename T>
    data_size_t read(T* data) noexcept
    {
      return this->read(data, bit_sizeof<T>);
    }

    template <typename T>
    data_size_t read(T&& data) noexcept
    {
      return this->read(&data, bit_sizeof<std::decay_t<T>>);
    }

    template <typename T>
    data_size_t read(T const* const data, data_size_t n_read_bits) noexcept
    {
      auto constexpr ones = static_cast<data_block_t>(~0);

      auto read_block_it = reinterpret_cast<data_block_t const*>(data);

      auto const n_read_blocks = n_read_bits / sn_data_block_bit_size;
      auto const n_remaining_bits_in_block = sn_data_block_bit_size - mn_bit_index;

      auto const p_end = mp_block_it + n_read_blocks;
      while(mp_block_it != p_end)
      {
        (*mp_block_it) |= static_cast<data_block_t>(*read_block_it << mn_bit_index);
        ++mp_block_it;

        (*mp_block_it) |= static_cast<data_block_t>(*read_block_it >> n_remaining_bits_in_block);
        ++read_block_it;
      }

      // TODO: deal with end of buffer edge case
      // TODO: make like write function
      auto const n_remaining_read_bits = n_read_bits % sn_data_block_bit_size;
      if(n_remaining_read_bits > 0)
      {
        auto const n_remaining_block_bits = sn_data_block_bit_size - mn_bit_index;
        if(n_remaining_read_bits <= n_remaining_block_bits)
        {
          auto const n_shift_size     = static_cast<data_block_t>(n_remaining_block_bits - n_remaining_read_bits);
          auto const n_data_read_mask = static_cast<data_block_t>(ones >> n_shift_size);
          auto const n_data_to_read   = static_cast<data_block_t>(*read_block_it << mn_bit_index);
          *mp_block_it               |= static_cast<data_block_t>(n_data_to_read & n_data_read_mask);
          mn_bit_index += n_remaining_read_bits;

          if(mn_bit_index == sn_data_block_bit_size)
          {
            ++mp_block_it;
            mn_bit_index = 0;
          }
        }
        else
        {
          auto const n_data_to_write_first = static_cast<data_block_t>(*read_block_it << mn_bit_index);
          *mp_block_it                    |= static_cast<data_block_t>(n_data_to_write_first);

          ++mp_block_it;
          mn_bit_index                   = static_cast<data_index_t>(n_remaining_read_bits - n_remaining_block_bits);

          auto const n_shift_size        = static_cast<data_block_t>(sn_data_block_bit_size - mn_bit_index);
          auto const n_data_read_mask    = static_cast<data_block_t>(ones >> n_shift_size);
          auto const n_data_to_read_last = static_cast<data_block_t>(*read_block_it >> n_remaining_block_bits);
          *mp_block_it                  |= static_cast<data_block_t>(n_data_to_read_last & n_data_read_mask);
        }
      }

//// last working variant
//      auto const n_remaining_read_bits = n_read_bits % sn_data_block_bit_size;
//      if(n_remaining_read_bits > 0)
//      {
//        auto const n_data_read_mask = static_cast<data_block_t>(~(~data_block_t(0) << (mn_bit_index + n_remaining_read_bits)));
//        (*mp_block_it) |= static_cast<data_block_t>(static_cast<data_block_t>(*read_block_it << mn_bit_index) & n_data_read_mask);
//
//        mn_bit_index += n_remaining_read_bits;
//
//        if(mn_bit_index >= sn_data_block_bit_size)
//        {
//          ++mp_block_it;
//          mn_bit_index -= sn_data_block_bit_size;
//
//          auto const n_data_read_mask = static_cast<data_block_t>(~(~data_block_t(0) << (mn_bit_index)));
//          (*mp_block_it) |= static_cast<data_block_t>(static_cast<data_block_t>(*read_block_it >> (n_remaining_read_bits - mn_bit_index)) & n_data_read_mask);
//        }
//      }

      // TODO, amount of unreaded bits
      return {};
    }


    template <typename T>
    data_size_t write(T* data) noexcept
    {
      return this->write(data, bit_sizeof<T>);
    }

    template <typename T>
    data_size_t write(T&& data) noexcept
    {
      return this->write(&data, bit_sizeof<std::decay_t<T>>);
    }

    template <typename T>
    data_size_t write(T* const data, data_size_t n_write_bits) noexcept
    {
      auto constexpr ones = static_cast<data_block_t>(~0);

      auto write_block_it = reinterpret_cast<data_block_t*>(data);

      auto const n_write_blocks = n_write_bits / sn_data_block_bit_size;
      auto const n_remaining_bits_in_block = sn_data_block_bit_size - mn_bit_index;

      auto const p_end = mp_block_it + n_write_blocks;
      while(mp_block_it != p_end)
      {
        auto const& block = *mp_block_it;
        auto const& write = *write_block_it;
        *write_block_it |= static_cast<data_block_t>(*mp_block_it >> mn_bit_index);
        ++mp_block_it;

        *write_block_it |= static_cast<data_block_t>(*mp_block_it << n_remaining_bits_in_block);
        ++write_block_it;
      }



      // TODO: deal with end of buffer edge case
      auto const n_remaining_write_bits = n_write_bits % sn_data_block_bit_size;

      if(n_remaining_write_bits > 0)
      {
        auto const n_remaining_block_bits = sn_data_block_bit_size - mn_bit_index;
        if(n_remaining_write_bits <= n_remaining_block_bits)
        {
          auto const n_shift_size      = static_cast<data_block_t>(n_remaining_block_bits - n_remaining_write_bits);
          auto const n_data_write_mask = static_cast<data_block_t>(ones >> n_shift_size);
          auto const n_data_to_write   = static_cast<data_block_t>(*mp_block_it >> mn_bit_index);
          *write_block_it             |= static_cast<data_block_t>(n_data_to_write & n_data_write_mask);
          mn_bit_index += n_remaining_write_bits;

          if(mn_bit_index == sn_data_block_bit_size)
          {
            ++mp_block_it;
            mn_bit_index = 0;
          }
        }
        else
        {
          auto const n_data_to_write_first = static_cast<data_block_t>(*mp_block_it >> mn_bit_index);
          *write_block_it                 |= static_cast<data_block_t>(n_data_to_write_first);

          ++mp_block_it;
          mn_bit_index                    = static_cast<data_index_t>(n_remaining_write_bits - n_remaining_block_bits);

          auto const n_shift_size         = static_cast<data_block_t>(sn_data_block_bit_size - n_remaining_write_bits);
          auto const n_data_write_mask    = static_cast<data_block_t>(ones >> n_shift_size);
          auto const n_data_to_write_last = static_cast<data_block_t>(*mp_block_it << n_remaining_block_bits);
          *write_block_it                |= static_cast<data_block_t>(n_data_to_write_last & n_data_write_mask);
        }
      }

      return {};
    }

  private:
    bit_buffer_view* mp_bit_buffer_view = nullptr;

    data_block_t* mp_block_it = nullptr;
    data_index_t mn_bit_index = 0;
  };

public:
  DEFAULT_SPECIAL_MEMBER_FUNCTION(bit_buffer_view)

  explicit bit_buffer_view(void* p_buffer, data_size_t n_buffer_size) noexcept
    : mp_buffer{ reinterpret_cast<data_byte_t*>(p_buffer) }
    , mn_buffer_size{n_buffer_size}
  {}

  data_size_t const& size() const noexcept
  {
    return mn_buffer_size;
  }

  template <typename TDataBlock>
  iterator<TDataBlock> begin() noexcept
  {
    return iterator<TDataBlock>{*this};
  }

  template <typename TDataBlock>
  iterator<TDataBlock> end() noexcept
  {
    return iterator<TDataBlock>{*this, mn_buffer_size};
  }

private:
  data_byte_t* mp_buffer;
  data_size_t mn_buffer_size;

};


#undef DEFAULT_SPECIAL_MEMBER_FUNCTION


template <typename TDataBlock>
using bit_buffer_view_iterator_t = typename bit_buffer_view:: template iterator<TDataBlock>;

template <typename TDataBlockLhs, typename TDataBlockRhs>
inline bool operator == ( bit_buffer_view_iterator_t<TDataBlockLhs> const& lhs
                        , bit_buffer_view_iterator_t<TDataBlockRhs> const& rhs ) noexcept
{
  return lhs.get_byte_index()  == rhs.get_byte_index()
      && lhs.get_byte_bit_index()   == rhs.get_byte_bit_index();
}

template <typename TDataBlockLhs, typename TDataBlockRhs>
inline bool operator != ( bit_buffer_view_iterator_t<TDataBlockLhs> const& lhs
                        , bit_buffer_view_iterator_t<TDataBlockRhs> const& rhs ) noexcept
{
  return !(lhs == rhs);
}


#define BIT_BUF_DECLARE_ITERATOR_COMPERISON_OPERTOR(_OP1_, _OP2_) \
template <typename TDataBlockLhs, typename TDataBlockRhs> \
inline bool operator _OP1_ ( bit_buffer_view_iterator_t<TDataBlockLhs> const& lhs \
                           , bit_buffer_view_iterator_t<TDataBlockRhs> const& rhs ) noexcept \
{ \
  return lhs.get_byte_index() _OP2_ rhs.get_byte_index() \
      || \
      (  lhs.get_byte_index() == rhs.get_byte_index() \
      && lhs.get_bit_index()  _OP1_  rhs.get_bit_index() ); \
}

BIT_BUF_DECLARE_ITERATOR_COMPERISON_OPERTOR(<, <)
BIT_BUF_DECLARE_ITERATOR_COMPERISON_OPERTOR(<=, <)
BIT_BUF_DECLARE_ITERATOR_COMPERISON_OPERTOR(>, >)
BIT_BUF_DECLARE_ITERATOR_COMPERISON_OPERTOR(>=, >)

#undef BIT_BUF_DECLARE_ITERATOR_COMPERISON_OPERTOR

template <typename TDataBlockLhs, typename TDataBlockRhs>
inline auto operator + ( bit_buffer_view_iterator_t<TDataBlockLhs> const& lhs
                       , bit_buffer_view_iterator_t<TDataBlockRhs> const& rhs ) noexcept
{
  return bit_buffer_view_iterator_t<TDataBlockLhs>{lhs} += rhs;
}

template <typename TDataBlockLhs, typename TDataBlockRhs>
inline auto operator - ( bit_buffer_view_iterator_t<TDataBlockLhs> const& lhs
                       , bit_buffer_view_iterator_t<TDataBlockRhs> const& rhs ) noexcept
{
  return bit_buffer_view_iterator_t<TDataBlockLhs>{lhs} -= rhs;
}