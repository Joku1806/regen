#include "UnicodeHelper.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

uint32_t UnicodeHelper::hex_string_to_u32(std::string_view hex) {
  uint32_t converted = 0;
  uint32_t underflow_check = 0;

  auto convert_hexdigit = [](char ch) {
    if (ch >= 'a')
      return ch - 'a' + 10;
    if (ch >= 'A')
      return ch - 'A' + 10;
    return ch - '0';
  };

  for (auto ch : hex) {
    underflow_check = converted << 4u | convert_hexdigit(ch);
    if (underflow_check < converted) {
      throw std::invalid_argument("Codepoint is invalid (too big).");
    }
    converted = underflow_check;
  }

  return converted;
}

std::string UnicodeHelper::escaped_codepoint_to_utf8(std::string_view hex) {
  uint32_t utf32 = hex_string_to_u32(hex);

  if (utf32 <= 0x7F) {
    std::string out = std::string(1, 0);
    out[0] = utf32;
    return out;
  } else if (utf32 <= 0x7FF) {
    std::string out = std::string(2, 0);
    out[0] = 0xC0 | (utf32 >> 6);
    out[1] = 0x80 | (utf32 & 0x3F);
    return out;
  } else if (utf32 <= 0xFFFF) {
    std::string out = std::string(3, 0);
    out[0] = 0xE0 | (utf32 >> 12);
    out[1] = 0x80 | ((utf32 >> 6) & 0x3F);
    out[2] = 0x80 | (utf32 & 0x3F);
    return out;
  } else if (utf32 <= 0x10FFFF) {
    std::string out = std::string(4, 0);
    out[0] = 0xF0 | (utf32 >> 18);
    out[1] = 0x80 | ((utf32 >> 12) & 0x3F);
    out[2] = 0x80 | ((utf32 >> 6) & 0x3F);
    out[3] = 0x80 | (utf32 & 0x3F);
    return out;
  }

  throw std::invalid_argument("Codepoints above 0x10FFFF are not supported.");
}

void UnicodeHelper::consume_leading_utf8_codepoint(std::string_view &view) {
  do {
    view.remove_prefix(1);
  } while ((view[0] & 0xc0) == 0x80);
}

std::string UnicodeHelper::get_leading_utf8_codepoint(std::string_view view) {
  uint8_t header = view[0];
  uint8_t size = 0;
  while (header & 0x80) {
    header <<= 1;
    size++;
  }
  // special case for ASCII character
  if (size == 0) {
    size = 1;
  }

  if ((view[size] & 0xC0) == 0x80) {
    std::cerr << "Leading codepoint is malformed, size " << unsigned(size)
              << " specified in header is "
                 "not correct.\n";
    throw;
  }

  return std::string(view.begin(), size);
}