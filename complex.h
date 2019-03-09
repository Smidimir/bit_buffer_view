#pragma once


template <typename T, T whole_size_v>
struct complex
{
  using data_t = T;

  static auto constexpr whole_size = data_t{whole_size_v};

  explicit
  constexpr
  complex(T whole_v = {}, T part_v = {}) noexcept
      : m_whole{whole_v + part_v / whole_size}
      , m_part{part_v % whole_size}
  {}

  T constexpr& whole() noexcept
  {
    return m_whole;
  }

  T const constexpr& whole() const noexcept
  {
    return m_whole;
  }

  T constexpr& part() noexcept
  {
    return m_part;
  }

  T const constexpr& part() const noexcept
  {
    return m_part;
  }

  complex constexpr&
  operator += (complex<T, whole_size_v> const& rhs) noexcept
  {
    auto const result = *this + rhs;
    m_whole = result.m_whole;
    m_part  = result.m_part;
    return *this;
  }

  complex constexpr&
  operator -= (complex<T, whole_size_v> const& rhs) noexcept
  {
    auto const result = *this - rhs;
    m_whole = result.m_whole;
    m_part  = result.m_part;
    return *this;
  }

private:
  T m_whole;
  T m_part;
};


template <typename T, T whole_size_v>
complex<T, whole_size_v> constexpr
operator + (complex<T, whole_size_v> const& lhs, complex<T, whole_size_v> const& rhs)
{
  return complex<T, whole_size_v>{lhs.whole() + rhs.whole(), lhs.part()+ rhs.part()};
}

template <typename T, T whole_size_v>
complex<T, whole_size_v> constexpr
operator - (complex<T, whole_size_v> const& lhs, complex<T, whole_size_v> const& rhs)
{
  return ( lhs.part() >= rhs.part() )
         ? complex<T, whole_size_v>{ lhs.whole() - rhs.whole()
                                   , lhs.part() - rhs.part() }
         : complex<T, whole_size_v>{ lhs.whole() - rhs.whole() - 1
                                   , lhs.whole_size - (rhs.part() - lhs.part()) };
}

template <typename T, T whole_size_v>
bool constexpr
operator == (complex<T, whole_size_v> const& lhs, complex<T, whole_size_v> const& rhs)
{
  return lhs.whole() == rhs.whole() && lhs.part() == rhs.part();
}

template <typename T, T whole_size_v>
bool constexpr
operator != (complex<T, whole_size_v> const& lhs, complex<T, whole_size_v> const& rhs)
{
  return lhs.whole() != rhs.whole()|| lhs.part() != rhs.part();
}


#define DECLARE_COMPLEX_COMPARISON_OPERATOR(_OP1_, _OP2_) \
    template <typename T, T whole_size_v> \
    bool constexpr \
    operator _OP1_ (complex<T, whole_size_v> const& lhs, complex<T, whole_size_v> const& rhs) noexcept \
    { \
      return lhs.whole() _OP2_ rhs.whole() \
          || \
          (  lhs.whole() == rhs.whole() \
          && lhs.part() _OP1_ rhs.part() ); \
    }

DECLARE_COMPLEX_COMPARISON_OPERATOR(<, <)
DECLARE_COMPLEX_COMPARISON_OPERATOR(<=, <)
DECLARE_COMPLEX_COMPARISON_OPERATOR(>, >)
DECLARE_COMPLEX_COMPARISON_OPERATOR(>=, >)

#undef DECLARE_COMPLEX_COMPARISON_OPERATOR