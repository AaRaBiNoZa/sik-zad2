#ifndef SIK_ZAD2_CONNECTIONUTILS_H
#define SIK_ZAD2_CONNECTIONUTILS_H

#include <string>

/**
 * Used during setup - when given a string with colons inside,
 * returns two strings divided by the last colon.
 */
std::pair<std::string, std::string> extract_host_and_port(
    const std::string& addr) {
  size_t dividor_position = addr.rfind(':');
  std::string clean_host = addr.substr(0, dividor_position);
  std::string clean_port = addr.substr(dividor_position + 1);

  return {clean_host, clean_port};
}

#endif  // SIK_ZAD2_CONNECTIONUTILS_H
