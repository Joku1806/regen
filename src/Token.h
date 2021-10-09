#include <iostream>
#include <optional>

#define SUBGROUP_OPEN_SYMBOL '('
#define SUBGROUP_CLOSE_SYMBOL ')'
#define REPEAT0_1_SYMBOL '?'
#define REPEAT1_INF_SYMBOL '+'
#define REPEAT0_INF_SYMBOL '*'
#define ALTERNATOR_SYMBOL '|'
#define ESCAPE_SYMBOL '\\'
#define LITERAL_RANGE_OPEN_SYMBOL '['
#define LITERAL_RANGE_CLOSE_SYMBOL ']'
#define REPETITION_RANGE_OPEN_SYMBOL '{'
#define REPETITION_RANGE_CLOSE_SYMBOL '}'
#define RANGE_ARGUMENT_SEPARATOR_SYMBOL ','
#define UNICODE_ESCAPE_SPECIFIER_SYMBOL 'u'
#define UNICODE_ESCAPE_OPEN_SYMBOL '{'
#define UNICODE_ESCAPE_CLOSE_SYMBOL '}'
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

struct Token {
  Token(TokenKind kind) : kind(kind) {}
  TokenKind kind;

  virtual std::ostream &print(std::ostream &os) const;
  virtual bool equals(const Token &other) const;

  friend std::ostream &operator<<(std::ostream &os, const Token &t);
  friend bool operator==(const Token &lhs, const Token &rhs);
};

struct LiteralToken : Token {
  LiteralToken(const std::string &symbol) : Token(Literal), symbol(symbol) {}
  std::string symbol;

  std::ostream &print(std::ostream &os) const override;
  bool equals(const Token &other) const override;
};

struct NumberToken : Token {
  NumberToken(std::optional<uint32_t> value) : Token(Number), value(value) {}
  // no value is used to represent infinity
  std::optional<uint32_t> value;

  std::ostream &print(std::ostream &os) const override;
  bool equals(const Token &other) const override;
};