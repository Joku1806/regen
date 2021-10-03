#include "lexer.h"
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

TokenKind Lexer::get_token_kind(const std::string &symbol, LexMode lex_mode) {
  if (lex_mode != Default && symbol.starts_with(','))
    return RangeArgumentSeparator;

  // NOTE: Don't know if std::isxdigit covers all cases, maybe a number starting
  // with a 'x' instead of '0x' is still considered a hexadecimal number.
  if (lex_mode == InRepetitionRange && std::isxdigit(symbol[0]))
    return Number;

  if (symbol.starts_with('('))
    return SubgroupOpen;
  if (symbol.starts_with(')'))
    return SubgroupClose;
  if (symbol.starts_with('?'))
    return Repeat0_1;
  if (symbol.starts_with('+'))
    return Repeat1_Inf;
  if (symbol.starts_with('*'))
    return Repeat0_Inf;
  if (symbol.starts_with('|'))
    return Alternator;
  if (symbol.starts_with('\\'))
    return Escape;
  if (symbol.starts_with('['))
    return LiteralRangeOpen;
  if (symbol.starts_with(']'))
    return LiteralRangeClose;
  if (symbol.starts_with('{'))
    return RepetitionRangeOpen;
  if (symbol.starts_with('}'))
    return RepetitionRangeClose;
  return Literal;
}

std::string Lexer::get_token_kind_name(TokenKind kind) {
  if (kind == SubgroupOpen)
    return "SubgroupOpen";
  if (kind == SubgroupClose)
    return "SubgroupClose";
  if (kind == Repeat0_1)
    return "RepeatZeroToOnce";
  if (kind == Repeat1_Inf)
    return "RepeatOnceToInfinity";
  if (kind == Repeat0_Inf)
    return "RepeatZeroToInfinity";
  if (kind == Alternator)
    return "Alternator";
  if (kind == Escape)
    return "Escape";
  if (kind == Number)
    return "Number";
  if (kind == Literal)
    return "Literal";
  if (kind == LiteralRangeOpen)
    return "LiteralRangeOpen";
  if (kind == LiteralRangeClose)
    return "LiteralRangeClose";
  if (kind == RepetitionRangeOpen)
    return "RepetitionRangeOpen";
  if (kind == RepetitionRangeClose)
    return "RepetitionRangeClose";
  if (kind == RangeArgumentSeparator)
    return "RangeArgumentSeparator";
  return "Unknown Kind";
}

// FIXME: For now these functions all assume that the input is valid unicode
void Lexer::advance_to_next_utf8_character(std::string::const_iterator it) {
  do {
    it++;
  } while ((*it & 0xc0) == 0x80);
}

// TODO: last vs previous
void Lexer::revert_to_last_utf8_character(std::string::const_iterator it) {
  do {
    it--;
  } while ((*it & 0xc0) == 0x80);
}

std::string Lexer::get_utf8_character(std::string::const_iterator it) {
  std::string::const_iterator cpy = it;
  advance_to_next_utf8_character(cpy);
  return std::string(&*it, cpy - it);
}

// If passed character is part of a reserved sequence such as \n or \t, returns
// the ASCII value of that sequence. Returns {} for characters that are not
// reserved.
std::optional<char> Lexer::convert_reserved_ASCII_character(char symbol) {
  switch (symbol) {
  case '0':
    return '\0';
  case 'a':
    return '\a';
  case 'b':
    return '\b';
  case 't':
    return '\t';
  case 'n':
    return '\n';
  case 'v':
    return '\v';
  case 'f':
    return '\f';
  case 'r':
    return '\r';
  }

  return {};
}

uint8_t
Lexer::determine_utf8_character_size(const std::vector<uint8_t> &source_bytes) {
  if (source_bytes.size() == 0) {
    std::cout << "Didn't find any hex literals after unicode escape sequence!"
              << std::endl;
    throw;
  }

  if (source_bytes.size() == 1) {
    return 1;
  }

  // We know that at least one more byte than the original byte count is needed,
  // because there will be headers that take up space. For the first byte, this
  // header contains one set bit for every byte in the utf-8 representation as
  // well as a 0-bit after them. So by looking at this header, you can easily
  // find out how many bytes there are in this utf-8 character. The header of
  // the other bytes will just be '10' at the end of each byte, marking them as
  // continuation bytes.
  //
  // But this means that utf8_bytes will be defined in terms of itself, since
  // there could be more bytes needed depending on the size of the first header.
  // Because there is no way of solving this kind of recursive formula, we will
  // have to loop through every possible value until we find one that matches
  // itself.
  uint8_t utf8_bytes = source_bytes.size() + 1;
  while (utf8_bytes != ceil((3 * utf8_bytes - 1) / 8.0) + source_bytes.size()) {
    utf8_bytes++;
  }

  return utf8_bytes;
}

uint8_t Lexer::get_uint8_slice_from_bitstream(const std::vector<uint8_t> &bytes,
                                              uint32_t start, uint32_t stop) {
  if (start >= bytes.size() * 8 || stop >= bytes.size() * 8) {
    std::cout << "Trying to read bits (" << start << ", " << stop
              << ") past the end of bitstream with size=" << bytes.size()
              << " bytes." << std::endl;
    throw;
  }

  if (start > stop) {
    std::cout << "start=" << start << " has to be <= stop=" << stop << "."
              << std::endl;
    throw;
  }

  if (stop - start > 8) {
    std::cout << "Can't get more than one byte from bitstream!" << std::endl;
    throw;
  }

  uint32_t start_byte = start / 8;
  uint8_t start_offset = start % 8;
  uint32_t stop_byte = stop / 8;
  uint8_t stop_offset = stop % 8;

  uint8_t byte = 0;
  byte |= bytes[start_byte] << start_offset;
  byte |= (bytes[stop_byte] >> (8 - start_offset));
  byte >>= 7 - (stop - start);

  return byte;
}

// FIXME: Rename variables to avoid confusion
std::optional<std::string>
Lexer::convert_escaped_utf8_character(const std::string &utf8_repr) {
  if (utf8_repr[0] != 'u')
    return {};

  std::vector<uint8_t> source_bytes;
  uint8_t converted_byte = 0;
  uint8_t bytes_read = 0;

  for (auto ch : utf8_repr.substr(1)) {
    if (!std::isxdigit(ch)) {
      break;
    }

    if (bytes_read % 2 == 0) {
      converted_byte |= std::stol(&ch, nullptr, 16) << 4;
    } else {
      converted_byte |= std::stol(&ch, nullptr, 16);
    }

    if (bytes_read != 0 && bytes_read % 2 == 0) {
      source_bytes.push_back(converted_byte);
      converted_byte = 0;
    }

    bytes_read++;
  }

  // in case of an uneven number of hex digits, append the last dangling byte
  if (converted_byte != 0) {
    source_bytes.push_back(converted_byte);
  }

  uint8_t converted_size = determine_utf8_character_size(source_bytes);
  if (converted_size == 1) {
    std::string converted = std::string(1, source_bytes.front());
    return converted;
  }

  std::string converted = std::string(converted_size, 0x80);
  for (uint8_t header_bytes = 0; header_bytes < converted_size;
       header_bytes++) {
    converted[0] >>= 1;
    converted[0] |= 0x80;
  }
  converted[0] |=
      get_uint8_slice_from_bitstream(source_bytes, converted_size + 1, 7);

  // NOTE: don't know if for loop break condition is correct
  uint8_t byte_offset = 1;
  for (uint8_t bit_offset = 8; bit_offset < converted.size() * 8 - 6;
       bit_offset += 6) {
    converted[byte_offset] |= get_uint8_slice_from_bitstream(
        source_bytes, bit_offset, bit_offset + 6);
    byte_offset++;
  }

  return converted;
}

void Lexer::lex(const std::string &regex) {
  std::uint32_t currently_open_subgroups = 0;
  LexMode lex_mode = Default;

  for (std::string::const_iterator it = regex.begin(); it != regex.end();
       advance_to_next_utf8_character(it)) {
    if (std::isspace(*it)) {
      continue; // NOTE: Assumes that whitespace characters are ASCII
    }

    TokenKind previous =
        m_tokens.size() == 0 ? SubgroupOpen : m_tokens.back().kind;
    TokenKind current = get_token_kind(&*it, lex_mode);
    if (!m_grammar_allowed[previous][current]) {
      std::cout << "Token of kind " << get_token_kind_name(previous)
                << " followed by a " << get_token_kind_name(current)
                << " is not allowed!" << std::endl;
      throw;
    }

    if (lex_mode == InRepetitionRange) {
      if (current == Number) {
        size_t bytes_parsed = 0;
        std::string number_start = regex.substr(it - regex.begin());
        uint32_t converted_number = std::stol(number_start, &bytes_parsed, 0);
        m_tokens.push_back(ConvertedNumberToken(converted_number));
        it += bytes_parsed; // NOTE: also maybe off-by-one? :^)
        continue;
      }

      if (current == RangeArgumentSeparator) {
        if (m_tokens.back().kind != Number) {
          m_tokens.push_back(ConvertedNumberToken(0));
        }
        m_tokens.push_back(Token(RangeArgumentSeparator));
        continue;
      }

      if (current == RepetitionRangeClose) {
        if (m_tokens.back().kind != Number) {
          m_tokens.push_back(ConvertedNumberToken({}));
        }
        m_tokens.push_back(Token(RepetitionRangeClose));
        lex_mode = Default;
        continue;
      }

      std::cout << "Got illegal token kind " << get_token_kind_name(current)
                << " inside repetition range (following "
                << get_token_kind_name(previous)
                << "). Normally this should have been caught by our grammar "
                   "matrix, check the entries for both of these tokens!"
                << std::endl;
      throw;
    }

    if (lex_mode == InLiteralRange) {
      if (current == Literal) {
        // NOTE: Doesn't check for multiple literals in a row, since I am
        // still not sure if that should be allowed
        m_tokens.push_back(LiteralToken(get_utf8_character(it)));
        continue;
      }

      if (current == RangeArgumentSeparator) {
        m_tokens.push_back(Token(RangeArgumentSeparator));
        continue;
      }

      if (current == LiteralRangeClose) {
        m_tokens.push_back(Token(LiteralRangeClose));
        lex_mode = Default;
        continue;
      }

      std::cout << "Got illegal token kind " << get_token_kind_name(current)
                << " inside literal range (following "
                << get_token_kind_name(previous)
                << "). Normally this should have been caught by our grammar "
                   "matrix, check the entries for both of these tokens!"
                << std::endl;
      throw;
    }

    // FIXME: extract the special cases into several methods
    // In general this function is waaaaaaay too big lol
    if (current == Escape) {
      advance_to_next_utf8_character(it);
      if (it == regex.end()) {
        std::cout << "Trailing escape at the end of the regex is not allowed!"
                  << std::endl;
        throw;
      }

      std::optional<char> reserved = convert_reserved_ASCII_character(*it);
      if (reserved) {
        m_tokens.push_back(LiteralToken(std::string(1, reserved.value())));
        continue;
      }

      std::optional<std::string> unicode_decoded =
          convert_escaped_utf8_character(&*it);
      if (unicode_decoded) {
        m_tokens.push_back(LiteralToken(unicode_decoded.value()));
        it += unicode_decoded.value().size(); // NOTE: is this maybe off-by-one?
        continue;
      }

      TokenKind escaped_kind = get_token_kind(&*it, lex_mode);
      if (escaped_kind != Literal) {
        m_tokens.push_back(LiteralToken(get_utf8_character(it)));
        continue;
      }
      revert_to_last_utf8_character(it);
    }

    if (current == SubgroupOpen) {
      currently_open_subgroups++;
    }

    if (current == SubgroupClose) {
      if (currently_open_subgroups == 0) {
        std::cout << "Trying to close a non-existent subgroup is not allowed!"
                  << std::endl;
        throw; // FIXME: Maybe rethink how errors should be handled in the
               // future.
      }
      currently_open_subgroups--;
    }

    if (current == Literal) {
      m_tokens.push_back(LiteralToken(get_utf8_character(it)));
      continue;
    }

    m_tokens.push_back(Token(current));
  }

  // check if started groups were left open
  if (lex_mode != Default || currently_open_subgroups > 0 ||
      m_tokens.back().kind == Alternator) {
    std::cout << "Leaving a started pattern open is not allowed. Please close "
                 "it explicitly."
              << std::endl;
    throw;
  }
}