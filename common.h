//
// Created by Adam Al-Hosam on 04/05/2022.
//

#ifndef SIK_ZAD3_COMMON_H
#define SIK_ZAD3_COMMON_H
#include <string>

// tu validation dodac
std::pair<std::string, std::string> extract_host_and_port(std::string addr) {
    size_t dividor_position = addr.rfind(':');
    std::string clean_host = addr.substr(0,dividor_position);
    std::string clean_port = addr.substr(dividor_position + 1);

    return {clean_host, clean_port};
};

// help for my debug
std::ostream& operator<<(std::ostream& os, uint8_t x) {
  os << (char)(x + '0');

  return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> v) {
  for (auto k : v) {
    os << k << '\n';
  }

  return os;
}

template <typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, std::map<T1, T2> v) {
  for (auto [k, val] : v) {
    os << "{" << k << " : " << val << "}" << '\n';
  }

  return os;
}


#endif  // SIK_ZAD3_COMMON_H
