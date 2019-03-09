#include <iostream>

#include "bitbuf_view.h"

#include <vector>

#include <string_view>

#include <array>

#include "complex.h"



int main()
{

  auto in  = std::vector<std::uint8_t>{250, 238, 199, 175, 130, 99, 25, 255};
//  auto in  = std::vector<std::uint8_t>{255, 255, 255, 255, 255, 255, 255, 255};
  auto out = std::vector<std::uint8_t>(in.size(), 0);

  auto buf_in  = bbv::bitbuf_view{in.data(), in.size()};
  auto buf_out = bbv::bitbuf_view{out.data(), out.size()};

  auto it_in = buf_in.begin();
  auto it_out = buf_out.begin();

  auto const read_bits = std::size_t{63};

  auto temp = std::array<std::uint8_t, 500>{};
  do
  {
    temp.fill(0);

    auto n_remaining = it_in.write(temp.data(), read_bits);
    it_out.read(temp.data(), read_bits - n_remaining);

    if(n_remaining > 0)
      break;
  } while(true);


  for(auto x : in)
    std::cout << (std::uint32_t)x << " ";
  std::cout << std::endl;

  for(auto x : out)
    std::cout << (std::uint32_t)x << " ";
  std::cout << std::endl;

  std::cout << it_in.get_byte_index().whole() << " " << it_in.get_byte_index().part() << std::endl;
  std::cout << it_out.get_byte_index().whole() << " " << it_out.get_byte_index().part() << std::endl;

  return 0;
}