#include "Token.h"

std::ostream &operator<<(std::ostream &os, const TokenKind &kind) {
  if (kind == SubgroupOpen) {
    os << "SubgroupOpen";
  } else if (kind == SubgroupClose) {
    os << "SubgroupClose";
  } else if (kind == Repeat0_1) {
    os << "RepeatZeroToOnce";
  } else if (kind == Repeat1_Inf) {
    os << "RepeatOnceToInfinity";
  } else if (kind == Repeat0_Inf) {
    os << "RepeatZeroToInfinity";
  } else if (kind == Alternator) {
    os << "Alternator";
  } else if (kind == Escape) {
    os << "Escape";
  } else if (kind == Literal) {
    os << "Literal";
  } else if (kind == Number) {
    os << "Number";
  } else if (kind == LiteralRangeOpen) {
    os << "LiteralRangeOpen";
  } else if (kind == LiteralRangeClose) {
    os << "LiteralRangeClose";
  } else if (kind == RepetitionRangeOpen) {
    os << "RepetitionRangeOpen";
  } else if (kind == RepetitionRangeClose) {
    os << "RepetitionRangeClose";
  } else if (kind == RangeArgumentSeparator) {
    os << "RangeArgumentSeparator";
  } else {
    os << "???";
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const Token &t) {
  return t.print(os);
}

std::ostream &Token::print(std::ostream &os) const {
  os << kind;
  return os;
}

std::ostream &LiteralToken::print(std::ostream &os) const {
  os << kind << "(" << symbol << ")";
  return os;
}

std::ostream &NumberToken::print(std::ostream &os) const {
  os << kind << "(";
  if (value) {
    os << value.value();
  } else {
    os << "Inf";
  }
  os << ")";

  return os;
}

bool operator==(const Token &lhs, const Token &rhs) { return lhs.equals(rhs); }

bool Token::equals(const Token &other) const { return kind == other.kind; }

bool LiteralToken::equals(const Token &other) const {
  if (other.kind != Literal) {
    return false;
  }
  return symbol == static_cast<const LiteralToken &>(other).symbol;
}

bool NumberToken::equals(const Token &other) const {
  if (other.kind != Number) {
    return false;
  }
  return value == static_cast<const NumberToken &>(other).value;
}