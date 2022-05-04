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


#endif  // SIK_ZAD3_COMMON_H
