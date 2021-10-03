#pragma once

#include <optional>
#include <string>
#include <vector>

#define NUMBER_OF_TOKEN_KINDS 15
enum TokenKind {
  Alternator,
  Concatenator,
  Escape,
  Literal,
  LiteralRangeClose,
  LiteralRangeOpen,
  Number,
  RangeArgumentSeparator,
  Repeat0_1,
  Repeat0_Inf,
  Repeat1_Inf,
  RepetitionRangeClose,
  RepetitionRangeOpen,
  SubgroupClose,
  SubgroupOpen,
};

enum LexMode {
  Default,
  InLiteralRange,
  InRepetitionRange,
};

struct Token {
  Token(TokenKind kind) : kind(kind) {}
  TokenKind kind;
};

struct LiteralToken : Token {
  LiteralToken(const std::string &symbol) : Token(Literal), symbol(symbol) {}
  std::string symbol;
};

struct ConvertedNumberToken : Token {
  ConvertedNumberToken(std::optional<uint32_t> value)
      : Token(Number), value(value) {}
  // no value is used to represent infinity
  std::optional<uint32_t> value;
};

class Lexer {
public:
  Lexer() {}
  ~Lexer() {}
  void lex(const std::string &regex);

private:
  // Provides a compact and fool-proof way of checking which kinds of tokens
  // are allowed after one another. For example,
  // [TokenKind::Alternator][TokenKind::Literal] = true means that an Alternator
  // can be followed by a Literal. Everything that is not set in this 2D array
  // is not syntactically correct. These rules don't include the Concatenator
  // operator as well as any whitespace for the following reasons:
  //
  // 1. Concatenator: This is only helpful for the AST generation and can't be
  // generated by user input itself.
  // 2. Whitespace: Doesn't carry any information and wouldn't do any harm to be
  // included, but is supposed to be stripped by the lexer.
  //
  // In both cases, not including them in the table serves as a kind of tripwire
  // for knowing that something is seriously wrong with our symbol
  // detection/lexing routine :^)
  static constexpr bool
      m_grammar_allowed[NUMBER_OF_TOKEN_KINDS][NUMBER_OF_TOKEN_KINDS] = {
          [TokenKind::Alternator][TokenKind::Literal] = true,
          [TokenKind::Alternator][TokenKind::LiteralRangeOpen] = true,
          [TokenKind::Alternator][TokenKind::SubgroupOpen] = true,

          [TokenKind::Literal][TokenKind::Alternator] = true,
          [TokenKind::Literal][TokenKind::Literal] = true,
          [TokenKind::Literal][TokenKind::LiteralRangeClose] = true,
          [TokenKind::Literal][TokenKind::LiteralRangeOpen] = true,
          [TokenKind::Literal][TokenKind::Number] = true,
          [TokenKind::Literal][TokenKind::RangeArgumentSeparator] = true,
          [TokenKind::Literal][TokenKind::Repeat0_1] = true,
          [TokenKind::Literal][TokenKind::Repeat0_Inf] = true,
          [TokenKind::Literal][TokenKind::Repeat1_Inf] = true,
          [TokenKind::Literal][TokenKind::RepetitionRangeClose] = true,
          [TokenKind::Literal][TokenKind::RepetitionRangeOpen] = true,
          [TokenKind::Literal][TokenKind::SubgroupClose] = true,
          [TokenKind::Literal][TokenKind::SubgroupOpen] = true,

          [TokenKind::LiteralRangeClose][TokenKind::Alternator] = true,
          [TokenKind::LiteralRangeClose][TokenKind::Literal] = true,
          [TokenKind::LiteralRangeClose][TokenKind::LiteralRangeOpen] = true,
          [TokenKind::LiteralRangeClose][TokenKind::Repeat0_1] = true,
          [TokenKind::LiteralRangeClose][TokenKind::Repeat0_Inf] = true,
          [TokenKind::LiteralRangeClose][TokenKind::Repeat1_Inf] = true,
          [TokenKind::LiteralRangeClose][TokenKind::RepetitionRangeOpen] = true,
          [TokenKind::LiteralRangeClose][TokenKind::SubgroupClose] = true,
          [TokenKind::LiteralRangeClose][TokenKind::SubgroupOpen] = true,

          [TokenKind::LiteralRangeOpen][TokenKind::Literal] = true,

          [TokenKind::Number][TokenKind::RangeArgumentSeparator] = true,
          [TokenKind::Number][TokenKind::RepetitionRangeClose] = true,

          [TokenKind::RangeArgumentSeparator][TokenKind::Literal] = true,
          [TokenKind::RangeArgumentSeparator][TokenKind::Number] = true,
          [TokenKind::RangeArgumentSeparator][TokenKind::RepetitionRangeClose] =
              true,

          [TokenKind::Repeat0_1][TokenKind::Alternator] = true,
          [TokenKind::Repeat0_1][TokenKind::Literal] = true,
          [TokenKind::Repeat0_1][TokenKind::LiteralRangeOpen] = true,
          [TokenKind::Repeat0_1][TokenKind::SubgroupClose] = true,
          [TokenKind::Repeat0_1][TokenKind::SubgroupOpen] = true,

          [TokenKind::Repeat0_Inf][TokenKind::Alternator] = true,
          [TokenKind::Repeat0_Inf][TokenKind::Literal] = true,
          [TokenKind::Repeat0_Inf][TokenKind::LiteralRangeOpen] = true,
          [TokenKind::Repeat0_Inf][TokenKind::SubgroupClose] = true,
          [TokenKind::Repeat0_Inf][TokenKind::SubgroupOpen] = true,

          [TokenKind::Repeat1_Inf][TokenKind::Alternator] = true,
          [TokenKind::Repeat1_Inf][TokenKind::Literal] = true,
          [TokenKind::Repeat1_Inf][TokenKind::LiteralRangeOpen] = true,
          [TokenKind::Repeat1_Inf][TokenKind::SubgroupClose] = true,
          [TokenKind::Repeat1_Inf][TokenKind::SubgroupOpen] = true,

          [TokenKind::RepetitionRangeClose][TokenKind::Alternator] = true,
          [TokenKind::RepetitionRangeClose][TokenKind::Literal] = true,
          [TokenKind::RepetitionRangeClose][TokenKind::LiteralRangeOpen] = true,
          [TokenKind::RepetitionRangeClose][TokenKind::SubgroupClose] = true,
          [TokenKind::RepetitionRangeClose][TokenKind::SubgroupOpen] = true,

          [TokenKind::RepetitionRangeOpen][TokenKind::Number] = true,
          [TokenKind::RepetitionRangeOpen][TokenKind::RangeArgumentSeparator] =
              true,

          [TokenKind::SubgroupClose][TokenKind::Alternator] = true,
          [TokenKind::SubgroupClose][TokenKind::Literal] = true,
          [TokenKind::SubgroupClose][TokenKind::LiteralRangeOpen] = true,
          [TokenKind::SubgroupClose][TokenKind::Repeat0_1] = true,
          [TokenKind::SubgroupClose][TokenKind::Repeat0_Inf] = true,
          [TokenKind::SubgroupClose][TokenKind::Repeat1_Inf] = true,
          [TokenKind::SubgroupClose][TokenKind::RepetitionRangeOpen] = true,
          [TokenKind::SubgroupClose][TokenKind::SubgroupClose] = true,
          [TokenKind::SubgroupClose][TokenKind::SubgroupOpen] = true,

          [TokenKind::SubgroupOpen][TokenKind::Literal] = true,
          [TokenKind::SubgroupOpen][TokenKind::LiteralRangeOpen] = true,
          [TokenKind::SubgroupOpen][TokenKind::SubgroupOpen] = true,
  };
  LexMode m_mode;
  std::string m_regex;
  std::vector<Token> m_tokens;
  uint8_t
  determine_utf8_character_size(const std::vector<uint8_t> &source_bytes);
  uint8_t get_uint8_slice_from_bitstream(const std::vector<uint8_t> &bytes,
                                         uint32_t start, uint32_t stop);
  // FIXME: description is not accurate right now
  // Tries to convert unicode characters such as \u03BB to their actual utf-8
  // representation. Returns {} if symbol doesn't start with \u or the
  // string following \u is not a valid encoded utf-8 character.
  std::optional<std::string>
  convert_escaped_utf8_character(const std::string &utf8_repr);
  // Tries to convert commonly encoded characters such as \n, \t, etc...
  // to their actual ASCII value. Returns {} if symbol is not one of those
  // characters. This method makes it possible to distinguish these kinds of
  // characters from normal escaped symbols, that would otherwise be interpreted
  // as part of the regex grammar.
  std::optional<char> convert_reserved_ASCII_character(char symbol);
  TokenKind get_token_kind(const std::string &symbol, LexMode lex_mode);
  std::string get_token_kind_name(TokenKind kind);
  void revert_to_last_utf8_character(std::string::const_iterator it);
  void advance_to_next_utf8_character(std::string::const_iterator it);
  std::string get_utf8_character(std::string::const_iterator it);
};