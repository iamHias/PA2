#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// ================================================================================================
// CIPv4 — Represents an IPv4 address (e.g. 192.168.1.1)
// ================================================================================================

class CIPv4 {
public:
    CIPv4() {
        m_Addr[0] = m_Addr[1] = m_Addr[2] = m_Addr[3] = 0;
    }

    explicit CIPv4(const std::string& src) {
        std::istringstream iss(src);
        if (!(iss >> *this) || !iss.eof())
            throw std::invalid_argument(src);
    }

    bool operator==(const CIPv4& x) const = default;

    friend std::ostream& operator<<(std::ostream& os, const CIPv4& x) {
        return os << static_cast<uint32_t>(x.m_Addr[0]) << '.'
                  << static_cast<uint32_t>(x.m_Addr[1]) << '.'
                  << static_cast<uint32_t>(x.m_Addr[2]) << '.'
                  << static_cast<uint32_t>(x.m_Addr[3]);
    }

    friend std::istream& operator>>(std::istream& is, CIPv4& x) {
        uint8_t addr[4];

        for (int i = 0; i < 4; i++)
            CIPv4::parseDotNum(is, addr[i], i > 0);

        if (is)
            for (int i = 0; i < 4; i++)
                x.m_Addr[i] = addr[i];

        return is;
    }

private:
    static void parseDotNum(std::istream& is, uint8_t& n, bool dot) {
        if (dot) {
            char c;
            if (!is.get(c) || c != '.') {
                is.setstate(std::ios::failbit);
                return;
            }
        }

        int x;
        if (!(is >> x) || x < 0 || x > 255) {
            is.setstate(std::ios::failbit);
            return;
        }

        n = static_cast<uint8_t>(x);
    }

    uint8_t m_Addr[4];
};

// ================================================================================================
// CIPv6 — Represents an IPv6 address (e.g. 2001:db8:85a3:0:0:8a2e:370:7334)
// ================================================================================================

class CIPv6 {
public:
    CIPv6() {
        for (int i = 0; i < 8; i++)
            m_Addr[i] = 0;
    }

    explicit CIPv6(const std::string& src) {
        std::istringstream iss(src);
        if (!(iss >> *this) || !iss.eof())
            throw std::invalid_argument(src);
    }

    bool operator==(const CIPv6& x) const = default;

    friend std::ostream& operator<<(std::ostream& os, const CIPv6& x) {
        const auto oldFlags = os.flags();
        os << std::hex;

        for (int i = 0; i < 8; i++) {
            if (i > 0) os << ':';
            os << x.m_Addr[i];
        }

        os.flags(oldFlags);
        return os;
    }

    friend std::istream& operator>>(std::istream& is, CIPv6& x) {
        uint16_t addr[8];
        const auto oldFlags = is.flags();
        is >> std::hex;

        for (int i = 0; i < 8; i++)
            CIPv6::parseColonHex(is, addr[i], i > 0);

        if (is)
            for (int i = 0; i < 8; i++)
                x.m_Addr[i] = addr[i];

        is.flags(oldFlags);
        return is;
    }

private:
    static void parseColonHex(std::istream& is, uint16_t& n, bool colon) {
        if (colon) {
            char c;
            if (!is.get(c) || c != ':') {
                is.setstate(std::ios::failbit);
                return;
            }
        }

        int x;
        if (!(is >> std::hex >> x) || x < 0 || x > 65535) {
            is.setstate(std::ios::failbit);
            return;
        }

        n = static_cast<uint16_t>(x);
    }

    uint16_t m_Addr[8];
};