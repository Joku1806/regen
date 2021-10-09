#include "Lexer.h"
#include "UnicodeHelper.h"
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

TokenKind Lexer::identify_current_token_kind() {
  if (m_mode != Default &&
      m_view.starts_with(RANGE_ARGUMENT_SEPARATOR_SYMBOL)) {
    return RangeArgumentSeparator;
  }

  if (m_mode == InRepetitionRange && std::isxdigit(m_view[0])) {
    return Number;
  }

  if (m_view.starts_with(SUBGROUP_OPEN_SYMBOL)) {
    return SubgroupOpen;
  } else if (m_view.starts_with(SUBGROUP_CLOSE_SYMBOL)) {
    return SubgroupClose;
  } else if (m_view.starts_with(REPEAT0_1_SYMBOL)) {
    return Repeat0_1;
  } else if (m_view.starts_with(REPEAT1_INF_SYMBOL)) {
    return Repeat1_Inf;
  } else if (m_view.starts_with(REPEAT0_INF_SYMBOL)) {
    return Repeat0_Inf;
  } else if (m_view.starts_with(ALTERNATOR_SYMBOL)) {
    return Alternator;
  } else if (m_view.starts_with(ESCAPE_SYMBOL)) {
    return Escape;
  } else if (m_view.starts_with(LITERAL_RANGE_OPEN_SYMBOL)) {
    return LiteralRangeOpen;
  } else if (m_view.starts_with(LITERAL_RANGE_CLOSE_SYMBOL)) {
    return LiteralRangeClose;
  } else if (m_view.starts_with(REPETITION_RANGE_OPEN_SYMBOL)) {
    return RepetitionRangeOpen;
  } else if (m_view.starts_with(REPETITION_RANGE_CLOSE_SYMBOL)) {
    return RepetitionRangeClose;
  }

  return Literal;
}

// If passed character is part of a reserved sequence such as \n or \t, returns
// the ASCII value of that sequence. Returns {} for characters that are not
// reserved.
// FIXME: Don't like that it uses m_view[0] explicitly
std::optional<char> Lexer::convert_reserved_ASCII_character() {
  if (m_view[0] == '0') {
    return '\0';
  } else if (m_view[0] == 'a') {
    return '\a';
  } else if (m_view[0] == 'b') {
    return '\b';
  } else if (m_view[0] == 't') {
    return '\t';
  } else if (m_view[0] == 'n') {
    return '\n';
  } else if (m_view[0] == 'v') {
    return '\v';
  } else if (m_view[0] == 'f') {
    return '\f';
  } else if (m_view[0] == 'r') {
    return '\r';
  }

  return {};
}

void Lexer::handle_token_in_RepetitionRange() {
  if (m_current == Number) {
    if (m_currently_lexed_range_values >= 2) {
      std::cerr
          << "More than 2 numbers are not allowed in a repetition range.\n";
      throw;
    }

    size_t bytes_parsed = 0;
    uint32_t converted_number = std::stol(m_view.begin(), &bytes_parsed, 0);
    m_tokens.push_back(new NumberToken(converted_number));
    // NOTE: Don't think this can even become negative, but still something to
    // watch out for
    m_view.remove_prefix(bytes_parsed - 1);
    m_currently_lexed_range_values++;
    return;
  }

  if (m_current == RangeArgumentSeparator) {
    if (m_previous != Number) {
      m_tokens.push_back(new NumberToken(0));
    }

    m_tokens.push_back(new Token(RangeArgumentSeparator));
    return;
  }

  if (m_current == RepetitionRangeClose) {
    if (m_previous != Number) {
      m_tokens.push_back(new NumberToken({}));
    }

    m_tokens.push_back(new Token(RepetitionRangeClose));
    m_mode = Default;
    m_currently_lexed_range_values = 0;
    return;
  }

  std::cerr << "Got illegal token kind " << m_current
            << " inside repetition range (following " << m_previous
            << "). Normally this should have been caught by our grammar "
               "matrix, check the entries for both of these tokens!\n";
  throw;
}

void Lexer::handle_token_in_LiteralRange() {
  if (m_current == Literal) {
    if (m_previous == Literal) {
      std::cerr << "If you want a range with multiple literals inside, please "
                   "use multiple ranges with one literal instead.\n";
    }

    if (m_currently_lexed_range_values >= 2) {
      std::cerr << "More than 2 literals are not allowed in a literal range.\n";
      throw;
    }

    m_tokens.push_back(
        new LiteralToken(UnicodeHelper::get_leading_utf8_codepoint(m_view)));
    m_currently_lexed_range_values++;
    return;
  }

  if (m_current == RangeArgumentSeparator) {
    m_tokens.push_back(new Token(RangeArgumentSeparator));
    return;
  }

  if (m_current == LiteralRangeClose) {
    m_tokens.push_back(new Token(LiteralRangeClose));
    m_mode = Default;
    m_currently_lexed_range_values = 0;
    return;
  }

  std::cerr << "Got illegal token kind " << m_current
            << " inside literal range (following " << m_previous
            << "). Normally this should have been caught by our grammar "
               "matrix, check the entries for both of these tokens!\n";
  throw;
}

void Lexer::handle_escaped_token() {
  UnicodeHelper::consume_leading_utf8_codepoint(m_view);
  if (m_view.length() == 0) {
    std::cerr << "Trailing escape at the end of the regex is not allowed!\n";
    throw;
  }

  std::optional<char> reserved = convert_reserved_ASCII_character();
  if (reserved) {
    m_tokens.push_back(new LiteralToken(std::string(1, reserved.value())));
    UnicodeHelper::consume_leading_utf8_codepoint(m_view);
    return;
  }

  if (m_view[0] == UNICODE_ESCAPE_SPECIFIER_SYMBOL) {
    m_view.remove_prefix(1);

    if (m_view[0] != UNICODE_ESCAPE_OPEN_SYMBOL) {
      std::cerr << "Escaped codepoint is malformed, expected "
                << UNICODE_ESCAPE_OPEN_SYMBOL << ".\n";
      throw;
    }
    m_view.remove_prefix(1);

    const char *hex_start = &m_view[0];
    for (auto ch : m_view) {
      if (!std::isxdigit(ch)) {
        break;
      };
      m_view.remove_prefix(1);
    }

    if (m_view[0] != UNICODE_ESCAPE_CLOSE_SYMBOL) {
      std::cerr << "Escaped codepoint is malformed, expected "
                << UNICODE_ESCAPE_CLOSE_SYMBOL << ".\n";
      throw;
    }

    std::string_view hexstring(hex_start, &m_view[0] - hex_start);
    m_tokens.push_back(
        new LiteralToken(UnicodeHelper::escaped_codepoint_to_utf8(hexstring)));
    UnicodeHelper::consume_leading_utf8_codepoint(m_view);
    return;
  }

  UnicodeHelper::consume_leading_utf8_codepoint(m_view);
  m_tokens.push_back(
      new LiteralToken(UnicodeHelper::get_leading_utf8_codepoint(m_view)));
  UnicodeHelper::consume_leading_utf8_codepoint(m_view);
}

std::vector<Token *> Lexer::lex() {
  while (m_view.length() > 0) {
    if (std::isspace(m_view[0])) {
      m_view.remove_prefix(1);
      continue;
    }

    m_previous = m_tokens.size() == 0 ? SubgroupOpen : m_tokens.back()->kind;
    m_current = identify_current_token_kind();
    if (!m_grammar_allowed[m_previous][m_current]) {
      std::cerr << "Token of kind " << m_previous << " followed by a "
                << m_current << " is not allowed!\n";
      throw;
    }

    if (m_mode == InRepetitionRange) {
      handle_token_in_RepetitionRange();
      UnicodeHelper::consume_leading_utf8_codepoint(m_view);
      continue;
    } else if (m_mode == InLiteralRange) {
      handle_token_in_LiteralRange();
      UnicodeHelper::consume_leading_utf8_codepoint(m_view);
      continue;
    } else if (m_current == Escape) {
      handle_escaped_token();
      continue;
    } else if (m_current == Literal) {
      m_tokens.push_back(
          new LiteralToken(UnicodeHelper::get_leading_utf8_codepoint(m_view)));
      UnicodeHelper::consume_leading_utf8_codepoint(m_view);
      continue;
    } else if (m_current == SubgroupOpen) {
      m_currently_open_subgroups++;
    } else if (m_current == SubgroupClose) {
      if (m_currently_open_subgroups == 0) {
        std::cerr
            << "Trying to close a non-existent subgroup is not allowed!\n";
        throw;
      }
      m_currently_open_subgroups--;
    } else if (m_current == RepetitionRangeOpen) {
      m_mode = InRepetitionRange;
    } else if (m_current == LiteralRangeOpen) {
      m_mode = InLiteralRange;
    }

    m_tokens.push_back(new Token(m_current));
    UnicodeHelper::consume_leading_utf8_codepoint(m_view);
  }

  // check if started groups were left open
  if (m_mode != Default || m_currently_open_subgroups > 0 ||
      m_current == Alternator) {
    std::cerr
        << "Leaving a started expression open is not allowed. Please close "
           "it explicitly.\n";
    throw;
  }

  return m_tokens;
}