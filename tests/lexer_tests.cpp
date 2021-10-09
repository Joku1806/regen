#include "../src/Lexer.h"
#include <criterion/criterion.h>
#include <vector>

void check_lexer_output_equals(std::vector<Token *> generated,
                               std::vector<Token *> expected) {
  cr_assert_eq(generated.size(), expected.size());

  for (size_t offset = 0; offset < expected.size(); offset++) {
    cr_assert_eq(*generated.at(offset), *expected.at(offset));
  }

  for (size_t offset = 0; offset < expected.size(); offset++) {
    delete generated.at(offset);
    delete expected.at(offset);
  }
}

Test(repetition_range, basic) {
  check_lexer_output_equals(
      Lexer("a{4, 7}").lex(),
      std::vector<Token *>{
          new LiteralToken("a"), new Token(RepetitionRangeOpen),
          new NumberToken(4), new Token(RangeArgumentSeparator),
          new NumberToken(7), new Token(RepetitionRangeClose)});
}

Test(repetition_range, first_unspecified_second_hex) {
  check_lexer_output_equals(
      Lexer("b{, 0xd4}").lex(),
      std::vector<Token *>{
          new LiteralToken("b"), new Token(RepetitionRangeOpen),
          new NumberToken(0), new Token(RangeArgumentSeparator),
          new NumberToken(0xd4), new Token(RepetitionRangeClose)});
}

Test(repetition_range, first_octal_second_unspecified) {
  check_lexer_output_equals(
      Lexer("c{0375, }").lex(),
      std::vector<Token *>{
          new LiteralToken("c"), new Token(RepetitionRangeOpen),
          new NumberToken(0375), new Token(RangeArgumentSeparator),
          new NumberToken({}), new Token(RepetitionRangeClose)});
}

Test(repetition_range, first_unspecified_second_unspecified) {
  check_lexer_output_equals(
      Lexer("d{,}").lex(),
      std::vector<Token *>{
          new LiteralToken("d"), new Token(RepetitionRangeOpen),
          new NumberToken(0), new Token(RangeArgumentSeparator),
          new NumberToken({}), new Token(RepetitionRangeClose)});
}

Test(repetition_range, single_value) {
  check_lexer_output_equals(
      Lexer("e{69}").lex(),
      std::vector<Token *>{
          new LiteralToken("e"), new Token(RepetitionRangeOpen),
          new NumberToken(69), new Token(RangeArgumentSeparator),
          new NumberToken(69), new Token(RepetitionRangeClose)});
}