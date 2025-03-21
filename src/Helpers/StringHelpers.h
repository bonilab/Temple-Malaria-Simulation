//
// Created by Nguyen Tran on 1/26/2018.
//

#ifndef POMS_STRINGSPLITHELPER_H
#define POMS_STRINGSPLITHELPER_H

#include <sstream>
#include <string>
#include <vector>

template <class Elem>
using tstring =
    std::basic_string<Elem, std::char_traits<Elem>, std::allocator<Elem>>;

template <class Elem>
using tstringstream =
    std::basic_stringstream<Elem, std::char_traits<Elem>, std::allocator<Elem>>;

class StringHelpers {
public:
  template <typename Elem>
  static std::vector<tstring<Elem>> split(tstring<Elem> text,
                                          Elem const delimiter) {
    auto sstr = tstringstream<Elem>{text};
    auto tokens = std::vector<tstring<Elem>>{};
    auto token = tstring<Elem>{};
    while (std::getline(sstr, token, delimiter)) {
      if (!token.empty()) tokens.push_back(token);
    }

    return tokens;
  }

  template <typename Elem>
  static std::vector<tstring<Elem>> split(tstring<Elem> text,
                                          tstring<Elem> const &delimiters) {
    auto tokens = std::vector<tstring<Elem>>{};

    std::size_t pos, prev_pos = 0;
    while ((pos = text.find_first_of(delimiters, prev_pos))
           != std::string::npos) {
      if (pos > prev_pos)
        tokens.push_back(text.substr(prev_pos, pos - prev_pos));
      prev_pos = pos + 1;
    }

    if (prev_pos < text.length())
      tokens.push_back(text.substr(prev_pos, std::string::npos));

    return tokens;
  }

  // Helper function to concatenate elements of a string vector with a given
  // delimiter
  static std::string join(const std::vector<std::string> &vec,
                          const std::string &delimiter) {
    std::string result;
    for (std::size_t i = 0; i < vec.size(); i++) {
      result += vec[i];
      if (i != vec.size() - 1) { result += delimiter; }
    }
    return result;
  }

  static void replace_all(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
  } 
};

#endif  // POMS_STRINGSPLITHELPER_H
