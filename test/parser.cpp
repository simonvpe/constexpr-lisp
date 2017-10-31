#include "catch.hpp"
#include <string>
#include <cx_parser.h>

SCENARIO("Character matching") {
  using cx::parser::bind;
  using cx::parser::combine;
  using cx::parser::fmap;
  using cx::parser::lift;
  using cx::parser::make_char_parser;
  using cx::parser::many1;
  using cx::parser::many;
  using cx::parser::one_of;
  using cx::parser::operator|;
  using cx::parser::option;
  using cx::parser::parse_input_t;
  using std::string_view;

  GIVEN("make_char_parser('a')") {
    constexpr auto match = make_char_parser('a');
    THEN("it should match \"a\"") {
      constexpr auto result = match("a");
      REQUIRE( result );
      CHECK( result->first == 'a' );
      CHECK( result->second == "" );
    }
    THEN("it should NOT match \"b\"") {
      constexpr auto result = match("b");
      CHECK( !result );
    }
    THEN("it should NOT match \"\"") {
      constexpr auto result = match("");
      CHECK( !result );
    }
  }
     
  GIVEN("a character matcher [one_of(\"abc\")]") {
    constexpr auto match = one_of("abc");
    THEN("it should match \"a\"") {
      constexpr auto result = match("a");
      REQUIRE( result );
      CHECK( result->first == 'a' );
      CHECK( result->second == "" );
    }
    THEN("it should match \"b\"") {
      constexpr auto result = match("b");
      REQUIRE( result );
      CHECK( result->first == 'b' );
      CHECK( result->second == "" );
    }
    THEN("it should match \"c\"") {
      constexpr auto result = match("c");
      REQUIRE( result );
      CHECK( result->first == 'c' );
      CHECK( result->second == "" );
    }
    THEN("it should NOT match \"d\"") {
      constexpr auto result = match("d");
      CHECK( !result );
    }    
    THEN("it should NOT match \"\"") {
      constexpr auto result = match("");
      CHECK( !result );
    }
  }

  GIVEN("a character matcher [make_char_parser('a') | make_char_parser('k')]") {
    constexpr auto match = make_char_parser('a') | make_char_parser('b');
    THEN("it should match \"a\"") {
      constexpr auto result = match("a");
      CHECK( result);
    }
    THEN("it should match \"b\"") {
      constexpr auto result = match("b");
      CHECK( result );
    }
    THEN("it should NOT match \"c\"") {
      constexpr auto result = match("c");
      CHECK( !result );
    }
    THEN("it should NOT match \"\"") {
      constexpr auto result = match("");
      CHECK( !result );
    }
  }

  GIVEN("a character matcher [one_of(\"ab\") | one_of(\"bc\"]") {
    constexpr auto match = one_of("ab") | one_of("bc");
    THEN("it should match \"a\"") {
      constexpr auto result = match("a");
      REQUIRE( result );
      CHECK( result->first == 'a' );
      CHECK( result->second == "" );
    }
    THEN("it should match \"b\"") {
      constexpr auto result = match("b");
      REQUIRE( result );
      CHECK( result->first == 'b' );
      CHECK( result->second == "" );
    }
    THEN("it should match \"c\"") {
      constexpr auto result = match("c");
      REQUIRE( result );
      CHECK( result->first == 'c' );
      CHECK( result->second == "" );
    }
    THEN("it should NOT match \"d\"") {
      constexpr auto result = match("d");
      CHECK( !result );
    }
    THEN("it should NOT match \"\"") {
      constexpr auto result = match("");
      CHECK( !result );
    }
  }
  
  GIVEN("a character matcher [fmap(one_of(\"ab\"), toupper]") {
    constexpr auto toupper = [](char c) { 
      return ('a' <= c && c <= 'z') ? c + ('A' - 'a') : c;
    };
    constexpr auto match = fmap(toupper, one_of("ab"));
    THEN("it should upcase \"a\"") {
      constexpr auto result = match("a");
      REQUIRE( result );
      CHECK( result->first == 'A' );
      CHECK( result->second == "" );
    }
    THEN("it should upcase \"b\"") {
      constexpr auto result = match("b");
      REQUIRE( result );
      CHECK( result->first == 'B' );
      CHECK( result->second == "" );
    }
    THEN("it should NOT match \"c\"") {
      constexpr auto result = match("c");
      CHECK( !result);
    }
    THEN("it should NOT match \"\"") {
      constexpr auto result = match("");
      CHECK( !result );
    }
  }

  GIVEN("a matcher [many(one_of(\"ab\"), 0, std::sum)]") {
    constexpr auto sum = [](auto x, auto y) { return x + y; };
    constexpr auto match = many(one_of("ab"), 0, sum);
    THEN("it should sum \"aa\")") {
      constexpr auto result = match("aa");
      REQUIRE( result );
      CHECK( result->first == ('a' + 'a') );
      CHECK( result->second == "" );
    }
    THEN("it should sum \"b\")") {
      constexpr auto result = match("b");
      REQUIRE( result );
      CHECK( result->first == 'b' );
      CHECK( result->second == "" );
    }
    THEN("it should match sum \"\"") {
      constexpr auto result = match("");
      REQUIRE( result );
      CHECK( result->first == 0 );
      CHECK( result->second == "" );
    }
  }

  GIVEN("a matcher [many1(one_of(\"ab\"), 0, sum)]") {
    constexpr auto sum = [](auto x, auto y) { return x + y; };
    constexpr auto match = many1(one_of("ab"), 0, sum);
    THEN("it should sum \"aa\")") {
      constexpr auto result = match("aa");
      REQUIRE( result );
      CHECK( result->first == ('a' + 'a') );
      CHECK( result->second == "" );
    }
    THEN("it should sum \"b\")") {
      constexpr auto result = match("b");
      REQUIRE( result );
      CHECK( result->first == 'b' );
      CHECK( result->second == "" );
    }
    THEN("it should NOT match sum \"\"") {
      constexpr auto result = match("");
      CHECK( !result );
    }
  }
  
  GIVEN("a matcher[bind(one_of(\"ab\"), one_of(\"cd\"))]") {
    constexpr auto match = bind(one_of("ab"), [](char, parse_input_t rest) {
	return one_of("cd")(rest);
    });
    THEN("it should match \"ac\"") {
      constexpr auto result = match("ac");
      REQUIRE( result );
      CHECK( result->first == 'c' );
      CHECK( result->second == "" );
    }
    THEN("it should match \"ad\"") {
      constexpr auto result = match("ad");
      REQUIRE( result );
      CHECK( result->first == 'd' );
      CHECK( result->second == "" );
    }
    THEN("it should match \"bc\"") {
      constexpr auto result = match("bc");
      REQUIRE( result );
      CHECK( result->first == 'c' );
      CHECK( result->second == "" );
    }
    THEN("it should match \"bd\"") {
      constexpr auto result = match("bd");
      REQUIRE( result );
      CHECK( result->first == 'd' );
      CHECK( result->second == "" );
    }
    THEN("it should NOT match \"a\"") {
      constexpr auto result = match("a");
      CHECK( !result );
    }
    THEN("it should NOT match \"be\"") {
      constexpr auto result = match("be");
      CHECK( !result );
    }
  } 

  GIVEN("an option parser [option('+', make_char_parser('-'))]") {
    constexpr auto match = option('+', make_char_parser('-'));
    THEN("it should return '+' if \"a\"") {
      constexpr auto result = match("a");
      REQUIRE( result );
      CHECK( result->first == '+' );
      CHECK( result->second == "a" );
    }
    THEN("it should return '-' if \"-\"") {
      constexpr auto result = match("-");
      REQUIRE( result );
      CHECK( result->first == '-' );
      CHECK( result->second == "" );
    }
  }

  GIVEN("a matcher [lift('a')]") {
    constexpr auto match = lift('a');
    THEN("it should match \"xyz\"") {
      constexpr auto result = match("xyz");
      REQUIRE( result );
      CHECK( result->first == 'a' );
      CHECK( result->second == "xyz" );
    }
  }

  GIVEN("a matcher [combine(one_of(\"ab\"), one_of(\"bc\"), sum)]") {
    constexpr auto match = combine(one_of("ab"), one_of("bc"), [](char a, char b) {
	return a + b;
    });
    THEN("it should match \"ab\"") {
      constexpr auto result = match("ab");
      REQUIRE( result );
      CHECK( result->first == ('a' + 'b') );
      CHECK( result->second == "" );
    }
    THEN("it should match \"bb\"") {
      constexpr auto result = match("bb");
      REQUIRE( result );
      CHECK( result->first == ('b' + 'b') );
      CHECK( result->second == "" );
    }
    THEN("it should match \"ac\"") {
      constexpr auto result = match("ac");
      REQUIRE( result );
      CHECK( result->first == ('a' + 'c') );
      CHECK( result->second == "" );
    }
    THEN("it should match \"bc\"") {
      constexpr auto result = match("bc");
      REQUIRE( result );
      CHECK( result->first == ('b' + 'c') );
      CHECK( result->second == "" );
    }
    THEN("it should NOT match \"cb\"") {
      constexpr auto result = match("cb");
      CHECK( !result );
    }
    THEN("it should NOT match \"b\"") {
      constexpr auto result = match("b");
      CHECK( !result );
    }
    THEN("it should NOT match \"\"") {
      constexpr auto result = match("");
      CHECK( !result );
    }
  }

}
