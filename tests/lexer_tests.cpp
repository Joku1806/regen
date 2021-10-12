#include "../src/Lexer.h"
#include <criterion/criterion.h>
#include <signal.h>
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

// Just aborting as a catch-all is pretty bad error handling,
// should be changed to something more sensible
Test(repetition_range, multiple_numbers_in_argument_should_fail,
     .signal = SIGABRT) {
  Lexer("f{0xfeed 0xbeef, 567}").lex();
}

Test(repetition_range, invalid_number_in_argument_should_fail,
     .signal = SIGABRT) {
  Lexer("f{567, 0xgg}").lex();
}

Test(repetition_range, literal_in_argument_should_fail, .signal = SIGABRT) {
  Lexer("g{NaN111, 33}").lex();
}

Test(repetition_range, unclosed_range_should_fail, .signal = SIGABRT) {
  Lexer("f{13,14").lex();
}

Test(repetition_range, empty_range_should_fail, .signal = SIGABRT) {
  Lexer("g{}").lex();
}

Test(repetition_range, unstarted_range_should_fail, .signal = SIGABRT) {
  Lexer("h,15}").lex();
}

Test(repetition_range, no_body_before_range_should_fail, .signal = SIGABRT) {
  Lexer("{156, 165}").lex();
}