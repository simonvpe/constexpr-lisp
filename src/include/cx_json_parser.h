#pragma once

#include <cx_algorithm.h>
#include <cx_iterator.h>
#include <cx_json_value.h>
#include <cx_parser.h>
#include <cx_string.h>

#include <functional>
#include <string_view>
#include <type_traits>
#include <variant>

namespace JSON
{
  using namespace cx;
  using namespace cx::parser;

  //----------------------------------------------------------------------------
  // JSON value parsers

  // parse a JSON boolean value

  constexpr auto bool_parser()
  {
    using namespace std::literals;
    return fmap([] (std::string_view) { return true; },
                make_string_parser("true"sv))
      | fmap([] (std::string_view) { return false; },
             make_string_parser("false"sv));
  }

  // parse a JSON null value

  constexpr auto null_parser()
  {
    using namespace std::literals;
    return fmap([] (std::string_view) { return std::monostate{}; },
                make_string_parser("null"sv));
  }

  // parse a JSON number

  constexpr auto number_parser()
  {
    constexpr auto neg_parser = option('+', make_char_parser('-'));
    constexpr auto integral_parser =
      combine(neg_parser,
              fmap([] (char) { return 0; }, make_char_parser('0')) | int1_parser(),
              [] (char sign, int i) { return sign == '+' ? i : -i; });

    constexpr auto frac_parser = make_char_parser('.') < int0_parser();

    constexpr auto mantissa_parser = combine(
        integral_parser, option(0, frac_parser),
        [] (int i, int f) -> double {
          double d = 0;
          while (f > 0) {
            d += f % 10;
            d /= 10;
            f /= 10;
          }
          return i + d;
        });

    constexpr auto e_parser = make_char_parser('e') | make_char_parser('E');
    constexpr auto sign_parser = make_char_parser('+') | neg_parser;
    constexpr auto exponent_parser =
      bind(e_parser < sign_parser,
           [] (const char sign, const auto& sv) {
             return fmap([sign] (int j) { return sign == '+' ? j : -j; },
                         int0_parser())(sv);
           });

    return combine(
        mantissa_parser, option(0, exponent_parser),
        [] (double mantissa, int exp) {
          if (exp > 0) {
            while (exp--) {
              mantissa *= 10;
            }
          } else {
            while (exp++) {
              mantissa /= 10;
            }
          }
          return mantissa;
        });
  }

  // ---------------------------------------------------------------------------
  // parsing JSON strings

  // parse a JSON string char

  // When parsing a JSON string, in general multiple chars in the input may
  // result in different chars in the output (for example, an escaped char, or a
  // unicode code point) - which means we can't just return part of the input as
  // a sub-string_view: we need to actually build a string. So we will use
  // cx::string<4> as the return type of the char parsers to allow for the max
  // utf-8 conversion (note that all char parsers return the same thing so that
  // we can use the alternation combinator), and we will build up a cx::string<>
  // as the return type of the string parser.

  // if a char is escaped, simply convert it to the appropriate thing
  constexpr auto convert_escaped_char(char c)
  {
    switch (c) {
      case 'b': return '\b';
      case 'f': return '\f';
      case 'n': return '\n';
      case 'r': return '\r';
      case 't': return '\t';
      default: return c;
    }
  }

  // convert a unicode code point to utf-8
  constexpr auto to_utf8(uint32_t hexcode)
  {
    cx::basic_string<char, 4> s;
    if (hexcode <= 0x7f) {
      s.push_back(static_cast<char>(hexcode));
    } else if (hexcode <= 0x7ff) {
      s.push_back(static_cast<char>(0xC0 | (hexcode >> 6)));
      s.push_back(static_cast<char>(0x80 | (hexcode & 0x3f)));
    } else if (hexcode <= 0xffff) {
      s.push_back(static_cast<char>(0xE0 | (hexcode >> 12)));
      s.push_back(static_cast<char>(0x80 | ((hexcode >> 6) & 0x3f)));
      s.push_back(static_cast<char>(0x80 | (hexcode & 0x3f)));
    } else if (hexcode <= 0x10ffff) {
      s.push_back(static_cast<char>(0xF0 | (hexcode >> 18)));
      s.push_back(static_cast<char>(0x80 | ((hexcode >> 12) & 0x3f)));
      s.push_back(static_cast<char>(0x80 | ((hexcode >> 6) & 0x3f)));
      s.push_back(static_cast<char>(0x80 | (hexcode & 0x3f)));
    }
    return s;
  }

  constexpr auto to_hex(char c)
  {
    if (c >= '0' && c <= '9') return static_cast<uint16_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<uint16_t>(c - 'a' + 10);
    return static_cast<uint16_t>(c - 'A' + 10);
  }

  constexpr auto unicode_point_parser()
  {
    using namespace std::literals;
    constexpr auto p =
      make_char_parser('\\') <
      make_char_parser('u') <
      exactly_n(
          one_of("0123456789abcdefABCDEF"sv),
          4, 0u,
          [] (uint16_t hexcode, char c) -> uint16_t {
            return (hexcode << 4) + to_hex(c);
          });
    return fmap(to_utf8, p);
  }

  constexpr auto string_char_parser()
  {
    using namespace std::literals;
    constexpr auto slash_parser = make_char_parser('\\');
    constexpr auto special_char_parser =
      make_char_parser('"')
      | make_char_parser('\\')
      | make_char_parser('/')
      | make_char_parser('b')
      | make_char_parser('f')
      | make_char_parser('n')
      | make_char_parser('r')
      | make_char_parser('t');
    constexpr auto escaped_char_parser = fmap(
        convert_escaped_char, slash_parser < special_char_parser);
    constexpr auto p = escaped_char_parser | none_of("\\\""sv);

    return fmap([] (auto c) {
      cx::basic_string<char, 4> s;
      s.push_back(c);
      return s;
    }, p) | unicode_point_parser();
  }

  // parse a JSON string

  // See the comment about the char parsers above. Here we accumulate a
  // cx::string<> (which is arbitrarily sized at 32).

  constexpr auto string_parser()
  {
    constexpr auto quote_parser = make_char_parser('"');
    constexpr auto str_parser =
      many(string_char_parser(),
           cx::string{},
           [] (auto acc, const auto& str) {
             cx::copy(str.cbegin(), str.cend(), cx::back_insert_iterator(acc));
             return acc;
           });
    return quote_parser < str_parser > quote_parser;
  }

  // ---------------------------------------------------------------------------
  // parse the size of a JSON string

  constexpr std::size_t to_utf8_count(uint32_t hexcode)
  {
    if (hexcode <= 0x7f) {
      return 1;
    } else if (hexcode <= 0x7ff) {
      return 2;
    } else if (hexcode <= 0xffff) {
      return 3;
    }
    return 4;
  }

  constexpr auto unicode_point_count_parser()
  {
    using namespace std::literals;
    constexpr auto p =
      make_char_parser('\\') <
      make_char_parser('u') <
      exactly_n(
          one_of("0123456789abcdefABCDEF"sv),
          4, 0u,
          [] (uint16_t hexcode, char c) -> uint16_t {
            return (hexcode << 4) + to_hex(c);
          });
    return fmap(to_utf8_count, p);
  }

  constexpr auto string_char_count_parser()
  {
    using namespace std::literals;
    constexpr auto slash_parser = make_char_parser('\\');
    constexpr auto special_char_parser =
      make_char_parser('"')
      | make_char_parser('\\')
      | make_char_parser('/')
      | make_char_parser('b')
      | make_char_parser('f')
      | make_char_parser('n')
      | make_char_parser('r')
      | make_char_parser('t');
    constexpr auto escaped_char_parser = fmap(
        convert_escaped_char, slash_parser < special_char_parser);
    constexpr auto p = escaped_char_parser | none_of("\\\""sv);

    return fmap([] (auto) -> std::size_t { return 1; }, p)
      | unicode_point_count_parser();
  }

  constexpr auto string_size_parser()
  {
    constexpr auto quote_parser = make_char_parser('"');
    constexpr auto str_parser =
      many(string_char_count_parser(), std::size_t{}, std::plus<>{});
    return quote_parser < str_parser > quote_parser;
  }

  //----------------------------------------------------------------------------
  // JSON number-of-objects-required parser
  // An array is 1 + number of objects in the array
  // An object is 1 + number of objects in the object
  // Anything else is just 1

  template <std::size_t = 0>
  struct numobjects_recur
  {
    // parse a JSON value

    static constexpr auto value_parser()
    {
      using namespace std::literals;
      return [] (const auto& sv) {
               // deduce the return type of this lambda
               if (false) return fail(std::size_t{})(sv);
               constexpr auto p =
                 fmap([] (auto) -> std::size_t { return 1; },
                      make_string_parser("true"sv) | make_string_parser("false"sv)
                      | make_string_parser("null"sv))
                 | fmap([] (auto) -> std::size_t { return 1; },
                        number_parser())
                 | fmap([] (auto) -> std::size_t { return 1; },
                        string_parser())
                 | array_parser()
                 | object_parser();
               return (skip_whitespace() < p)(sv);
             };
    }

    // parse a JSON array

    static constexpr auto array_parser()
    {
      return make_char_parser('[') <
        separated_by(value_parser(),
                     skip_whitespace() < make_char_parser(','),
                     [] () { return std::size_t{1}; }, std::plus<>{})
        > skip_whitespace()
        > (make_char_parser(']') | fail(']', [] { throw "expected ]"; }));
    }

    // parse a JSON object

    static constexpr auto key_value_parser()
    {
      return skip_whitespace()
        < (string_parser()
           | fail(cx::string{}, [] { throw "expected a string as object key"; }))
        < skip_whitespace()
        < (make_char_parser(':')
           | fail(':', [] { throw "expected a colon as object key-value separator"; }))
        < value_parser();
    }

    static constexpr auto object_parser()
    {
      return make_char_parser('{') <
        separated_by(key_value_parser(),
                     skip_whitespace() < make_char_parser(','),
                     [] () { return std::size_t{1}; }, std::plus<>{})
        > skip_whitespace()
        > (make_char_parser('}') | fail('}', [] { throw "expected }"; }));
    }

  };

  // provide the num objects parser outside the struct qualification
  constexpr auto numobjects_parser = numobjects_recur<>::value_parser;

  template <char... Cs>
  constexpr auto numobjects()
  {
    std::initializer_list<char> il{Cs...};
    return numobjects_parser()(std::string_view(il.begin(), il.size()))->first;
  }

  //----------------------------------------------------------------------------
  // JSON string-size-required parser
  // A string is its own size
  // An array is the sum of value sizes within it
  // An object is the sum of key sizes and value sizes within it
  // Anything else is just 0

  template <std::size_t = 0>
  struct stringsize_recur
  {
    // parse a JSON value

    static constexpr auto value_parser()
    {
      using namespace std::literals;
      return [] (const auto& sv) {
               // deduce the return type of this lambda
               if (false) return fail(std::size_t{})(sv);
               constexpr auto p =
                 fmap([] (auto) -> std::size_t { return 0; },
                      make_string_parser("true"sv) | make_string_parser("false"sv)
                      | make_string_parser("null"sv))
                 | fmap([] (auto) -> std::size_t { return 0; },
                        number_parser())
                 | string_size_parser()
                 | array_parser()
                 | object_parser();
               return (skip_whitespace() < p)(sv);
             };
    }

    // parse a JSON array

    static constexpr auto array_parser()
    {
      return make_char_parser('[') <
        separated_by_val(value_parser(),
                         skip_whitespace() < make_char_parser(','),
                         std::size_t{0}, std::plus<>{})
        > skip_whitespace()
        > make_char_parser(']');
    }

    // parse a JSON object

    static constexpr auto key_value_parser()
    {
      constexpr auto p =
        skip_whitespace() < string_size_parser() > skip_whitespace() > make_char_parser(':');
      return bind(p,
                  [&] (std::size_t s1, const auto& sv) {
                    return fmap([s1] (std::size_t s2) { return s1 + s2; },
                                value_parser())(sv);
                  });
    }

    static constexpr auto object_parser()
    {
      return make_char_parser('{') <
        separated_by_val(key_value_parser(),
                         skip_whitespace() < make_char_parser(','),
                         std::size_t{}, std::plus<>{})
        > skip_whitespace()
        > make_char_parser('}');
    }

  };

  // provide the string size parser outside the struct qualification
  constexpr auto stringsize_parser = stringsize_recur<>::value_parser;

  template <char... Cs>
  constexpr auto stringsize()
  {
    std::initializer_list<char> il{Cs...};
    return stringsize_parser()(std::string_view(il.begin(), il.size()))->first;
  }

  //----------------------------------------------------------------------------
  // JSON extent parser
  // Everything returns a monostate: we only need the extent which we can
  // calculate from the leftover part

  template <std::size_t = 0>
  struct extent_recur
  {
    // parse a JSON value

    static constexpr auto value_parser()
    {
      using namespace std::literals;
      return [] (const auto& sv) {
               // deduce the return type of this lambda
               if (false) return fail(std::monostate{})(sv);
               constexpr auto p =
                 fmap([] (auto) { return std::monostate{}; },
                      make_string_parser("true"sv) | make_string_parser("false"sv)
                      | make_string_parser("null"sv))
                 | fmap([] (auto) { return std::monostate{}; },
                        number_parser())
                 | fmap([] (auto) { return std::monostate{}; },
                        string_parser())
                 | array_parser()
                 | object_parser();
               return (skip_whitespace() < p)(sv);
             };
    }

    // parse a JSON array

    static constexpr auto array_parser()
    {
      return make_char_parser('[') <
        separated_by_val(value_parser(),
                         skip_whitespace() < make_char_parser(','),
                         std::monostate{}, [] (auto x, auto) { return x; })
        > skip_whitespace()
        > make_char_parser(']');
    }

    // parse a JSON object

    static constexpr auto key_value_parser()
    {
      return skip_whitespace() < string_parser()
        < skip_whitespace() < make_char_parser(':')
        < value_parser();
    }

    static constexpr auto object_parser()
    {
      return make_char_parser('{') <
        separated_by_val(key_value_parser(),
                         skip_whitespace() < make_char_parser(','),
                         std::monostate{}, [] (auto x, auto) { return x; })
        > skip_whitespace()
        > make_char_parser('}');
    }

  };

  // provide the extent parser outside the struct qualification
  constexpr auto extent_parser = extent_recur<>::value_parser;

  //----------------------------------------------------------------------------
  // JSON parser

  // parse into a vector
  // return the index into the vector resulting from the parsed value

  template <std::size_t NObj, std::size_t NString>
  struct value_recur
  {
    using V = cx::vector<value, NObj>;
    using S = cx::basic_string<char, NString>;

    // parse a JSON value

    static constexpr auto value_parser(V& v, S& s)
    {
      using namespace std::literals;
      return [&] (const auto& sv) {
               // deduce the return type of this lambda
               if (false) return fail(std::size_t{})(sv);
               const auto p =
                 fmap([&] (auto) { v.push_back(value(true)); return v.size()-1; },
                      make_string_parser("true"sv))
                 | fmap([&] (auto) { v.push_back(value(false)); return v.size()-1; },
                      make_string_parser("false"sv))
                 | fmap([&] (auto) { v.push_back(value(std::monostate{})); return v.size()-1; },
                        make_string_parser("null"sv))
                 | fmap([&] (double d) { v.push_back(value(d)); return v.size()-1; },
                        number_parser())
                 | fmap([&] (const cx::string& str) {
                          auto offset = s.size();
                          cx::copy(str.cbegin(), str.cend(),
                                   cx::back_insert_iterator(s));
                          value::ExternalString es { offset, s.size() - offset };
                          v.push_back(value(es));
                          return v.size()-1;
                        },
                        string_parser())
                 | (make_char_parser('[') < array_parser(v, s))
                 | (make_char_parser('{') < object_parser(v, s));
               return (skip_whitespace() < p)(sv);
             };
    }

    // parse a JSON array

    static constexpr auto array_parser(V& v, S& s)
    {
      return [&] (const auto& sv) {
               value val{};
               val.to_Array();
               v.push_back(std::move(val));
               const auto p = separated_by_val(
                   value_parser(v, s), skip_whitespace() < make_char_parser(','),
                   v.size()-1,
                   [&] (std::size_t arr_idx, std::size_t element_idx) {
                     v[arr_idx].to_Array().push_back(element_idx);
                     return arr_idx;
                   }) > skip_whitespace() > make_char_parser(']');
               return p(sv);
             };
    }

    // parse a JSON object

    static constexpr auto key_value_parser(V& v, S& s)
    {
      constexpr auto p =
        skip_whitespace() < string_parser() > skip_whitespace() > make_char_parser(':');
      return bind(p,
                  [&] (const cx::string& str, const auto& sv) {
                    return fmap([&] (std::size_t idx) {
                                  return cx::make_pair(str, idx);
                                },
                      value_parser(v, s))(sv);
                  });
    }

    static constexpr auto object_parser(V& v, S& s)
    {
      return [&] (const auto& sv) {
               value val{};
               val.to_Object();
               v.push_back(std::move(val));
               const auto p = separated_by_val(
                   key_value_parser(v, s), skip_whitespace() < make_char_parser(','),
                   v.size()-1,
                   [&] (std::size_t obj_idx, const auto& kv) {
                     v[obj_idx].to_Object()[kv.first] = kv.second;
                     return obj_idx;
                   }) > skip_whitespace() > make_char_parser('}');
               return p(sv);
             };
    }

  };

  // A value_wrapper wraps a parsed JSON::value and contains the externalized
  // storage.
  template <size_t NumObjects, size_t StringSize>
  struct value_wrapper
  {
    constexpr value_wrapper(parse_input_t s)
    {
      value_recur<NumObjects, StringSize>::value_parser(object_storage, string_storage)(s);
    }

    constexpr operator value_proxy<NumObjects, const cx::vector<value, NumObjects>,
                                   const cx::basic_string<char, StringSize>>() const {
      return value_proxy{0, object_storage, string_storage};
    }
    constexpr operator value_proxy<NumObjects, cx::vector<value, NumObjects>,
                                   cx::basic_string<char, StringSize>>() {
      return value_proxy{0, object_storage, string_storage};
    }

    template <typename K,
              std::enable_if_t<!std::is_integral<K>::value, int> = 0>
    constexpr auto operator[](const K& s) const {
      return value_proxy{0, object_storage, string_storage}[s];
    }
    template <typename K,
              std::enable_if_t<!std::is_integral<K>::value, int> = 0>
    constexpr auto operator[](const K& s) {
      return value_proxy{0, object_storage, string_storage}[s];
    }

    constexpr auto operator[](std::size_t idx) const {
      return value_proxy{0, object_storage, string_storage}[idx];
    }
    constexpr auto operator[](std::size_t idx) {
      return value_proxy{0, object_storage, string_storage}[idx];
    }

    constexpr auto is_Null() const { return object_storage[0].is_Null(); }
    constexpr decltype(auto) to_String() const {
      return value_proxy{0, object_storage, string_storage}.to_String();
    }
    constexpr decltype(auto) to_String() {
      return value_proxy{0, object_storage, string_storage}.to_String();
    }
    constexpr decltype(auto) to_Number() const { return object_storage[0].to_Number(); }
    constexpr decltype(auto) to_Number() { return object_storage[0].to_Number(); }
    constexpr decltype(auto) to_Boolean() const { return object_storage[0].to_Boolean(); }
    constexpr decltype(auto) to_Boolean() { return object_storage[0].to_Boolean(); }

  private:
    cx::vector<value, NumObjects> object_storage;
    cx::basic_string<char, StringSize> string_storage;
  };

  namespace literals
  {
    template <typename T, T... Ts>
    constexpr auto operator "" _json()
    {
      constexpr std::initializer_list<T> il{Ts...};
      constexpr auto N = numobjects<Ts...>();
      constexpr auto S = stringsize<Ts...>();
      return value_wrapper<N, S>(std::string_view(il.begin(), il.size()));
    }
  }

}
