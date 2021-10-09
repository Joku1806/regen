#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class UnicodeHelper {
public:
  // Tries to convert unicode codepoints such as \u{03BB} to utf-8. Throws if
  // the hex string inside \u{} is not a valid utf-8 character.
  static std::string escaped_codepoint_to_utf8(std::string_view utf8);

  // Removes prefix from passed view such that the next codepoint will now be at
  // the start of view. As view is passed by reference, this will of course
  // affect the original view!
  static void consume_leading_utf8_codepoint(std::string_view &view);

  // Returns the utf8 codepoint at the start of view
  static std::string get_leading_utf8_codepoint(std::string_view view);

private:
  // Converts hex into the corresponding utf32 representation. Throws
  // std::invalid_argument if hex does not represent a valid codepoint.
  static uint32_t hex_string_to_u32(std::string_view hex);
};