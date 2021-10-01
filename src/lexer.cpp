#include "lexer.h"
#include <cctype>
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

std::string get_token_kind_name(TokenKind kind) {
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
std::optional<char> convert_reserved_ASCII_character(char symbol) {
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

std::optional<std::string> convert_unicode_character(std::string utf8_repr) {
  if (utf8_repr[0] != 'u')
    return {};
  // TODO
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

      std::optional<std::string> unicode_decoded = convert_utf8_character(&*it);
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