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
      {new LiteralToken("a"), new Token(RepetitionRangeOpen),
       new NumberToken(4), new Token(RangeArgumentSeparator),
       new NumberToken(7), new Token(RepetitionRangeClose)});
}

Test(repetition_range, first_unspecified_second_hex) {
  check_lexer_output_equals(
      Lexer("b{, 0xd4}").lex(),
      {new LiteralToken("b"), new Token(RepetitionRangeOpen),
       new NumberToken(0), new Token(RangeArgumentSeparator),
       new NumberToken(0xd4), new Token(RepetitionRangeClose)});
}

Test(repetition_range, first_octal_second_unspecified) {
  check_lexer_output_equals(
      Lexer("c{0375, }").lex(),
      {new LiteralToken("c"), new Token(RepetitionRangeOpen),
       new NumberToken(0375), new Token(RangeArgumentSeparator),
       new NumberToken({}), new Token(RepetitionRangeClose)});
}

Test(repetition_range, first_unspecified_second_unspecified) {
  check_lexer_output_equals(
      Lexer("d{,}").lex(),
      {new LiteralToken("d"), new Token(RepetitionRangeOpen),
       new NumberToken(0), new Token(RangeArgumentSeparator),
       new NumberToken({}), new Token(RepetitionRangeClose)});
}

Test(repetition_range, single_value) {
  check_lexer_output_equals(
      Lexer("e{69}").lex(),
      {new LiteralToken("e"), new Token(RepetitionRangeOpen),
       new NumberToken(69), new Token(RangeArgumentSeparator),
       new NumberToken(69), new Token(RepetitionRangeClose)});
}

// Just aborting as a catch-all is pretty bad error handling,
// should be changed to something more sensible
Test(repetition_range, multiple_numbers_in_argument_should_fail,
     .signal = SIGABRT) {
  Lexer("f{0xfeed 0xbeef, 567}").lex();
}

Test(repetition_range, first_argument_greater_than_second_argument_should_fail,
     .signal = SIGABRT) {
  Lexer("f{0xfeed, 0xbeef}").lex();
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

Test(repetition_range, multiple_close_symbols_should_fail, .signal = SIGABRT) {
  Lexer("f{13,}14}").lex();
}

Test(repetition_range, empty_range_should_fail, .signal = SIGABRT) {
  Lexer("g{}").lex();
}

Test(repetition_range, more_than_two_arguments_should_fail, .signal = SIGABRT) {
  Lexer("{1, 12, 123}").lex();
}

Test(repetition_range, unopened_range_should_fail, .signal = SIGABRT) {
  Lexer("h,15}").lex();
}

Test(repetition_range, no_body_before_range_should_fail, .signal = SIGABRT) {
  Lexer("{156, 165}").lex();
}

Test(escape, basic) {
  check_lexer_output_equals(Lexer("\\*").lex(), {new LiteralToken("*")});
}

Test(escape, ASCII_white_space_replace) {
  check_lexer_output_equals(Lexer("\\n").lex(), {new LiteralToken("\n")});
}

Test(escape, switch_to_unescape) {
  check_lexer_output_equals(
      Lexer("\\{{12, 25}").lex(),
      {new LiteralToken("{"), new Token(RepetitionRangeOpen),
       new NumberToken(12), new Token(RangeArgumentSeparator),
       new NumberToken(25), new Token(RepetitionRangeClose)});
}

Test(unicode_escape, conversion_is_valid) {
  check_lexer_output_equals(Lexer("\\u{002A}").lex(), {new LiteralToken("*")});
  check_lexer_output_equals(Lexer("\\u{03BB}").lex(), {new LiteralToken("λ")});
  check_lexer_output_equals(Lexer("\\u{16DD}").lex(), {new LiteralToken("ᛝ")});
  check_lexer_output_equals(Lexer("\\u{10192}").lex(), {new LiteralToken("𐆒")});
}

Test(unicode_escape, invalid_hexstring_should_fail, .signal = SIGABRT) {
  Lexer("\\u{03gbb}").lex();
}

Test(unicode_escape, overflow_should_fail, .signal = SIGABRT) {
  Lexer("\\u{ffffffffff}").lex();
}

Test(unicode_escape, invalid_codepoint_should_fail, .signal = SIGABRT) {
  Lexer("\\u{110000}").lex();
}

Test(unicode_escape, unclosed_escape_should_fail, .signal = SIGABRT) {
  Lexer("\\u{03bb").lex();
}

Test(unicode_escape, unopened_escape_should_fail, .signal = SIGABRT) {
  Lexer("\\u03bb}").lex();
}

Test(unicode_escape, multiple_close_symbols_should_fail, .signal = SIGABRT) {
  Lexer("\\u{03}bb}").lex();
}

Test(literal_range, basic) {
  check_lexer_output_equals(Lexer("[a, z]").lex(),
                            {new Token(LiteralRangeOpen), new LiteralToken("a"),
                             new Token(RangeArgumentSeparator),
                             new LiteralToken("z"),
                             new Token(LiteralRangeClose)});
}

Test(literal_range, only_second_argument_specified) {
  check_lexer_output_equals(
      Lexer("[, r]").lex(),
      {new Token(LiteralRangeOpen), new LiteralToken("\0"),
       new Token(RangeArgumentSeparator), new LiteralToken("r"),
       new Token(LiteralRangeClose)});
}

Test(literal_range, only_first_argument_specified) {
  check_lexer_output_equals(Lexer("c{ü, }").lex(),
                            {new Token(LiteralRangeOpen), new LiteralToken("ü"),
                             new Token(RangeArgumentSeparator),
                             new LiteralToken("􏿿"),
                             new Token(RepetitionRangeClose)});
}

Test(literal_range, first_unspecified_second_unspecified) {
  check_lexer_output_equals(
      Lexer("[,]").lex(),
      {new Token(LiteralRangeOpen), new LiteralToken("\0"),
       new Token(RangeArgumentSeparator), new LiteralToken("􏿿"),
       new Token(RepetitionRangeClose)});
}

Test(literal_range, single_value) {
  check_lexer_output_equals(Lexer("[h]").lex(),
                            {new Token(LiteralRangeOpen), new LiteralToken("h"),
                             new Token(RangeArgumentSeparator),
                             new LiteralToken("h"),
                             new Token(LiteralRangeClose)});
}

Test(literal_range, supports_unicode_literals) {
  check_lexer_output_equals(Lexer("[🌚, 🗿]").lex(),
                            {new Token(LiteralRangeOpen), new LiteralToken("🌚"),
                             new Token(RangeArgumentSeparator),
                             new LiteralToken("🗿"),
                             new Token(LiteralRangeClose)});
}

Test(literal_range, supports_unicode_escapes) {
  check_lexer_output_equals(Lexer("[\\u{03bb}, \\u{03c0}").lex(),
                            {new Token(LiteralRangeOpen), new LiteralToken("λ"),
                             new Token(RangeArgumentSeparator),
                             new LiteralToken("π"),
                             new Token(LiteralRangeClose)});
}

Test(literal_range, first_argument_greater_than_second_argument_should_fail,
     .signal = SIGABRT) {
  Lexer("[f, a]").lex();
}

Test(literal_range, more_than_one_literal_in_argument_should_fail,
     .signal = SIGABRT) {
  Lexer("[fg, a]").lex();
}

Test(literal_range, unclosed_range_should_fail, .signal = SIGABRT) {
  Lexer("[a,b").lex();
}

Test(literal_range, multiple_close_symbols_should_fail, .signal = SIGABRT) {
  Lexer("[a,]b]").lex();
}

Test(literal_range, empty_range_should_fail, .signal = SIGABRT) {
  Lexer("[]").lex();
}

Test(literal_range, more_than_two_arguments_should_fail, .signal = SIGABRT) {
  Lexer("[a, b, c]").lex();
}

Test(literal_range, unopened_range_should_fail, .signal = SIGABRT) {
  Lexer(",15]").lex();
}

Test(subgroup, basic) {
  check_lexer_output_equals(Lexer("(abc)").lex(),
                            {new Token(SubgroupOpen), new LiteralToken("a"),
                             new LiteralToken("b"), new LiteralToken("c"),
                             new Token(SubgroupClose)});
}

Test(subgroup, empty_subgroup_should_fail, .signal = SIGABRT) {
  Lexer("(abc)def()").lex();
}

Test(subgroup, closing_nonexistent_subgroup_should_fail, .signal = SIGABRT) {
  Lexer("(abc)def)").lex();
}