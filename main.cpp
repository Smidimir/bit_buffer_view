#include <iostream>
#include <array>
#include <vector>

#include "bit_buffer_view.h"


//00000000 00001010

int main(int argc, char** argv)
{
  using data_t = std::uint8_t;
  using iter_t = std::uint16_t;

  std::vector<data_t> in {89, 28, 33, 165, 72, 255, 255};
  std::vector<data_t> out{0, 0, 0, 0, 0, 0, 0};


  bit_buffer_view buffer_in{in.data(), in.size() * sizeof(data_t)};
  bit_buffer_view buffer_out{out.data(), out.size() * sizeof(data_t)};
//
  auto it_in = buffer_in.begin<iter_t>();
  auto it_out = buffer_out.begin<iter_t>();

//  std::uint64_t data = 255u;
//  bit_buffer_view data_buffer{in.data(), in.size() * sizeof(std::uint8_t)};
//  auto data_it = data_buffer.begin<std::uint8_t>();
//
//  std::cout << "---" << std::endl;
//  data_it.write(out.data(), 7);
//  std::cout << (std::uint32_t)out[0] << " " << (std::uint32_t)out[1] << " " << (std::uint32_t)out[2] << std::endl;
//  out[0] = out[1] = out[2] = 0;
//  data_it.write(out.data(), 10);
//  std::cout << (std::uint32_t)out[0] << " " << (std::uint32_t)out[1] << " " << (std::uint32_t)out[2] << std::endl;
//
//  std::cout << data_it.get_byte_index() << " " << data_it.get_byte_bit_index() << std::endl;
//
//  std::cout << "---" << std::endl;
//
//
  auto const n_bits = in.size() * sizeof(data_t) * 8u;
  auto const n_read_once = 13u;
  auto const size = n_bits / n_read_once;

  auto const remaining_bits = n_bits - size * n_read_once - 1;

  auto temp = std::array<std::uint8_t, 10>{};

  for(std::size_t i = 0; i < size; ++i)
  {
    temp.fill(0);
    it_in.write(temp.data(), n_read_once);
    it_out.read(temp.data(), n_read_once);
  }

  temp.fill(0);
  it_in.write(temp.data(), remaining_bits);
  it_out.read(temp.data(), remaining_bits);



  for(auto const& val : in)
  {
    std::cout << (std::uint32_t)val << " ";
  }
  std::cout << std::endl;

  for(auto const& val : out)
  {
    std::cout << (std::uint32_t)val << " ";
  }
  std::cout << std::endl;

  return 0;
}