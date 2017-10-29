#include "catch.hpp"
#include <string>
#include <cx_parser.h>

SCENARIO("Character matching") {
  
  GIVEN("make_char_parser('a')") {
    constexpr auto match = cx::parser::make_char_parser('a');
    THEN("it should match 'a'") {
      CHECK( match("a") );
    }
    THEN("it should NOT match 'b'") {
      CHECK( !match("b") );
    }
  }
     
  GIVEN("a character matcher [one_of(\"abc\")]") {
    constexpr auto match = cx::parser::one_of("abc");
    THEN("it should match \"a\"") {
      CHECK( match("a") );
    }
    THEN("it should match \"b\"") {
      CHECK( match("b") );
    }
    THEN("it should match \"c\"") {
      CHECK( match("c") );
    }
    THEN("it should NOT match \"d\"") {
      CHECK( !match("d") );
    }    
  }

  // GIVEN("a character matcher [exactly('a') | exactly('k')]") {
  //   constexpr auto match = exactly('a') | exactly('b');
  //   THEN("it should match \"a\"") {
  //     CHECK( match("a") );
  //   }
  //   THEN("it should match \"b\"") {
  //     CHECK( match("b") );
  //   }
  //   THEN("it should NOT match \"c\"") {
  //     CHECK( !match("c") );
  //   }
  // }

  // GIVEN("a character matcher [one_of(\"ab\") | one_of(\"bc\"]") {
  //   constexpr auto match = one_of("ab") | one_of("bc");
  //   THEN("it should match \"a\"") {
  //     CHECK( match("a") );
  //   }
  //   THEN("it should match \"b\"") {
  //     CHECK( match("b") );
  //   }
  //   THEN("it should match \"c\"") {
  //     CHECK( match("c") );
  //   }
  //   THEN("it should NOT match \"d\"") {
  //     CHECK( !match("d") );
  //   }
  // }
  
  // GIVEN("a character matcher [fmap(one_of(\"ab\"), toupper]") {
  //   constexpr auto toupper = [](char c) { return std::toupper(c); };
  //   constexpr auto match = fmap(one_of("ab"), toupper);
  //   THEN("it should upcase \"a\"") {
  //     const auto result = match("a");
  //     REQUIRE( result );
  //     CHECK( get<0>(result.value()) == 'A' );
  //   }
  //   THEN("it should upcase \"b\"") {
  //     const auto result = match("b");
  //     REQUIRE( result );
  //     CHECK( get<0>(result.value()) == 'B' );
  //   }
  //   THEN("it should NOT match \"c\"") {
  //     CHECK( !match("c") );
  //   }
  // }

  // GIVEN("a matcher [many(one_of(\"ab\"), 0, std::sum)]") {
  //   constexpr auto sum = [](auto x, auto y) { return x + y; };
  //   constexpr auto match = many(one_of("ab"), 0, sum);
  //   THEN("it should sum \"aa\")") {
  //     const auto result = match("aa");
  //     REQUIRE( result );
  //     CHECK( get<0>(result.value()) == ('a' + 'a') );
  //   }
  //   THEN("it should sum \"b\")") {
  //     const auto result = match("b");
  //     REQUIRE( result );
  //     CHECK( get<0>(result.value()) == 'b' );
  //   }
  //   THEN("it should match sum \"\"") {
  //     const auto result = match("");
  //     REQUIRE( result );
  //     CHECK( get<0>(result.value()) == 0 );
  //   }
  // }

  // GIVEN("a matcher [many1(one_of(\"ab\"), 0, std::sum)]") {
  //   constexpr auto sum = [](auto x, auto y) { return x + y; };
  //   constexpr auto match = many1(one_of("ab"), 0, sum);
  //   THEN("it should sum \"aa\")") {
  //     const auto result = match("aa");
  //     REQUIRE( result );
  //     CHECK( get<0>(result.value()) == ('a' + 'a') );
  //   }
  //   THEN("it should sum \"b\")") {
  //     const auto result = match("b");
  //     REQUIRE( result );
  //     CHECK( get<0>(result.value()) == 'b' );
  //   }
  //   THEN("it should NOT match sum \"\"") {
  //     const auto result = match("");
  //     CHECK( !result );
  //   }
  // }

  
  // GIVEN("a digits matcher [digits]") {
  //   constexpr auto match = digits();
  //   THEN("it should match \"0\"") {
  //     const auto result = match("0");
  //     REQUIRE( result );
  //     CHECK( get<0>(result.value()) == 0 );
  //   }
  //   THEN("it should match \"012345\"") {
  //     const auto result = match("012345");
  //     REQUIRE( result );
  //     CHECK( get<0>(result.value()) == 12345 );
  //   }
  //   THEN("it should NOT match \"\"") {
  //     REQUIRE( !match("") );
  //   }
  //   THEN("it should NOT match \"a\"") {
  //     REQUIRE( !match("a") );
  //   }
  // }

  // GIVEN("an option parser [option('+', exactly('-'))]") {
  //   constexpr auto match = option('+', exactly('-'));
  //   THEN("it should return '+' if \"a\"") {
  //     const auto result = match("a");
  //     REQUIRE( result );
  //     CHECK( get<0>(result.value()) == '+' );
  //   }
  //   THEN("it should return '-' if \"-\"") {
  //     const auto result = match("-");
  //     REQUIRE( result );
  //     CHECK( get<0>(result.value()) == '-' );
  //   }
  // }

  // GIVEN("a number parser [number()]") {
  //   constexpr auto match = number();

  // }
}
