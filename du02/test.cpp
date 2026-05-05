#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>
#include <compare>
#endif /* __PROGTEST__ */

class InvalidDateException : public std::invalid_argument {
  public:
    InvalidDateException() : std::invalid_argument("Invalid date") {}
};

int format_idx() {
  static int index = std::ios_base::xalloc();
  return index;
}

void format_callback(std::ios_base::event evt, std::ios_base& stream, int idx) {
    if (evt == std::ios_base::erase_event) {
        char* ptr = static_cast<char*>(stream.pword(idx));
        delete[] ptr;
        stream.pword(idx) = nullptr;
    } else if (evt == std::ios_base::copyfmt_event) {
        char* ptr = static_cast<char*>(stream.pword(idx));
        if (ptr) {
            size_t len = 0;
            while (ptr[len]) len++;
            char* new_ptr = new char[len + 1];
            for (size_t i = 0; i <= len; ++i) new_ptr[i] = ptr[i];
            stream.pword(idx) = new_ptr;
        }
    }
}

void apply_format(std::ios_base& ios, const char* format_str) {
    int id = format_idx();
    
    if (ios.iword(id) == 0) {
        ios.register_callback(format_callback, id);
        ios.iword(id) = 1;
    }
    
    char* old_ptr = static_cast<char*>(ios.pword(id));
    delete[] old_ptr;
    
    if (format_str) {
        size_t len = 0;
        while (format_str[len]) len++;
        char* new_ptr = new char[len + 1];
        for (size_t i = 0; i <= len; ++i) new_ptr[i] = format_str[i];
        ios.pword(id) = new_ptr;
    } else {
        ios.pword(id) = nullptr;
    }
}

class date_format {
  const char* format;
  public: 
    date_format(const char* f) : format(f) {}
    
    friend std::ostream& operator <<(std::ostream& os, const date_format& m) {
      apply_format(os, m.format);
      return os;
    }
    
    friend std::istream& operator >>(std::istream& is, const date_format& m) {
      apply_format(is, m.format);
      return is;
    }
};

class CDate
{
  private: 
    int m_Year;
    int m_Month;
    int m_Day;
    
    static bool is_leap_year (int year) {
      return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    }

    static int days_in_month (int year, int month) {
      static const int days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
      if(month == 2 && is_leap_year(year)) return 29;
      return days[month];   
    }

    int to_julian_day() const {
        return (1461 * (m_Year + 4800 + (m_Month - 14) / 12)) / 4 + (367 * (m_Month - 2 - 12 * ((m_Month - 14) / 12))) / 12 - (3 * ((m_Year + 4900 + (m_Month - 14) / 12) / 100)) / 4 + m_Day - 32075;
    }  
    
    void from_julian_day(int jdn) {
      int l = jdn + 68569, n = 4 * l / 146097; l -= (146097 * n + 3) / 4;
      int i = 4000 * (l + 1) / 1461001; l = l - 1461 * i / 4 + 31; int j = 80 * l / 2447;
        m_Day = l - 2447 * j / 80; l = j / 11; 
        m_Month = j + 2 - 12 * l; 
        m_Year = 100 * (n - 49) + i + l;
    }

    static bool read_digits (std::istream& is, int count, int& val) {
      val = 0;
      for (int i = 0; i < count; ++i) {
        int curr = is.get();
        if(curr < '0' || curr > '9') return false;
        val = val * 10 + (curr - '0');
      }
      return true;
    }
    
  public : 
    static bool isValid(int year, int month, int day) {
        if (month < 1 || month > 12) return false;
        if (day < 1 || day > days_in_month(year, month)) return false;
        return true;
    }

    CDate(int year, int month, int day) {
        if (!isValid(year, month, day)) throw InvalidDateException();
        m_Year = year; 
        m_Month = month; 
        m_Day = day;
    }

    CDate operator+(int days) const {
        CDate result = *this;
        result.from_julian_day(to_julian_day() + days);
        return result;
    }

    CDate operator-(int days) const {
        CDate result = *this;
        result.from_julian_day(to_julian_day() - days);
        return result;
    }

    int operator-(const CDate& otherDate) const {
        return to_julian_day() - otherDate.to_julian_day();
    }

    CDate& operator++() {
        from_julian_day(to_julian_day() + 1);
        return *this;
    }

    CDate operator++(int) {
        CDate tempDate = *this;
        ++(*this);
        return tempDate;
    }

    CDate& operator--() {
        from_julian_day(to_julian_day() - 1);
        return *this;
    }

    CDate operator--(int) {
        CDate tempDate = *this;
        --(*this);
        return tempDate;
    }

    auto operator<=>(const CDate&) const = default; 

    friend std::ostream& operator<<(std::ostream& os, const CDate& dateObj) {
        void* formatPointer = os.pword(format_idx());
        const char* formatString = formatPointer ? static_cast<const char*>(formatPointer) : "%Y-%m-%d";
        
        std::string resultString;
        char numberBuffer[5];
        
        for (; *formatString; ++formatString) {
            bool isFormatSpecifier = (*formatString == '%' && *(formatString + 1));
            if (isFormatSpecifier && (*(formatString + 1) == 'Y' || *(formatString + 1) == 'm' || *(formatString + 1) == 'd')) {
                ++formatString;
                if (*formatString == 'Y') { snprintf(numberBuffer, 5, "%04d", dateObj.m_Year); resultString += numberBuffer; }
                if (*formatString == 'm') { snprintf(numberBuffer, 3, "%02d", dateObj.m_Month); resultString += numberBuffer; }
                if (*formatString == 'd') { snprintf(numberBuffer, 3, "%02d", dateObj.m_Day); resultString += numberBuffer; }
            } else {
                if (isFormatSpecifier) ++formatString;
                resultString += *formatString;
            }
        }
        return os << resultString;
    }

    friend std::istream& operator>>(std::istream& is, CDate& dateObj) {
        void* formatPointer = is.pword(format_idx());
        const char* formatString = formatPointer ? static_cast<const char*>(formatPointer) : "%Y-%m-%d";
        
        int parsedYear = 0, parsedMonth = 0, parsedDay = 0, foundComponentsMask = 0;
        
        for (; *formatString && is; ++formatString) {
            bool isFormatSpecifier = (*formatString == '%' && *(formatString + 1));
            if (isFormatSpecifier && (*(formatString + 1) == 'Y' || *(formatString + 1) == 'm' || *(formatString + 1) == 'd')) {
                ++formatString;
                if      (*formatString == 'Y' && !(foundComponentsMask & 1) && read_digits(is, 4, parsedYear))   foundComponentsMask |= 1;
                else if (*formatString == 'm' && !(foundComponentsMask & 2) && read_digits(is, 2, parsedMonth)) foundComponentsMask |= 2;
                else if (*formatString == 'd' && !(foundComponentsMask & 4) && read_digits(is, 2, parsedDay))   foundComponentsMask |= 4;
                else break; 
            } else {
                if (isFormatSpecifier) ++formatString;
                if (is.get() != *formatString) break;
            }
        }
        
        if (foundComponentsMask == 7 && *formatString == 0 && isValid(parsedYear, parsedMonth, parsedDay)) {
            dateObj.m_Year = parsedYear; 
            dateObj.m_Month = parsedMonth; 
            dateObj.m_Day = parsedDay;
        } else {
            is.setstate(std::ios_base::failbit);
        }
        return is;
    }
};

#ifndef __PROGTEST__
int main ()
{
  std::ostringstream oss;
  std::istringstream iss;

  CDate a ( 2000, 1, 2 );
  CDate b ( 2010, 2, 3 );
  CDate c ( 2004, 2, 10 );
  oss . str ("");
  oss << a;
  assert ( oss . str () == "2000-01-02" );
  oss . str ("");
  oss << b;
  assert ( oss . str () == "2010-02-03" );
  oss . str ("");
  oss << c;
  assert ( oss . str () == "2004-02-10" );
  a = a + 1500;
  oss . str ("");
  oss << a;
  assert ( oss . str () == "2004-02-10" );
  b = b - 2000;
  oss . str ("");
  oss << b;
  assert ( oss . str () == "2004-08-13" );
  assert ( b - a == 185 );
  assert ( ( b == a ) == false );
  assert ( ( b != a ) == true );
  assert ( ( b <= a ) == false );
  assert ( ( b < a ) == false );
  assert ( ( b >= a ) == true );
  assert ( ( b > a ) == true );
  assert ( ( c == a ) == true );
  assert ( ( c != a ) == false );
  assert ( ( c <= a ) == true );
  assert ( ( c < a ) == false );
  assert ( ( c >= a ) == true );
  assert ( ( c > a ) == false );
  a = ++c;
  oss . str ( "" );
  oss << a << " " << c;
  assert ( oss . str () == "2004-02-11 2004-02-11" );
  a = --c;
  oss . str ( "" );
  oss << a << " " << c;
  assert ( oss . str () == "2004-02-10 2004-02-10" );
  a = c++;
  oss . str ( "" );
  oss << a << " " << c;
  assert ( oss . str () == "2004-02-10 2004-02-11" );
  a = c--;
  oss . str ( "" );
  oss << a << " " << c;
  assert ( oss . str () == "2004-02-11 2004-02-10" );
  iss . clear ();
  iss . str ( "2015-09-03" );
  assert ( ( iss >> a ) );
  oss . str ("");
  oss << a;
  assert ( oss . str () == "2015-09-03" );
  a = a + 70;
  oss . str ("");
  oss << a;
  assert ( oss . str () == "2015-11-12" );
  oss . str ("");
  oss << std::setw ( 20 ) << a;
  assert ( oss . str () == "          2015-11-12" );

  CDate d ( 2000, 1, 1 );
  try
  {
    CDate e ( 2000, 32, 1 );
    assert ( "No exception thrown!" == nullptr );
  }
  catch ( ... )
  {
  }
  iss . clear ();
  iss . str ( "2000-12-33" );
  assert ( ! ( iss >> d ) );
  oss . str ("");
  oss << d;
  assert ( oss . str () == "2000-01-01" );
  iss . clear ();
  iss . str ( "2000-11-31" );
  assert ( ! ( iss >> d ) );
  oss . str ("");
  oss << d;
  assert ( oss . str () == "2000-01-01" );
  iss . clear ();
  iss . str ( "2000-02-29" );
  assert ( ( iss >> d ) );
  oss . str ("");
  oss << d;
  assert ( oss . str () == "2000-02-29" );
  iss . clear ();
  iss . str ( "2001-02-29" );
  assert ( ! ( iss >> d ) );
  oss . str ("");
  oss << d;
  assert ( oss . str () == "2000-02-29" );

  CDate f ( 2000, 5, 12 );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2000-05-12" );
  oss . str ("");
  oss << date_format ( "%Y/%m/%d" ) << f;
  assert ( oss . str () == "2000/05/12" );
  oss . str ("");
  oss << date_format ( "%d.%m.%Y" ) << f;
  assert ( oss . str () == "12.05.2000" );
  oss . str ("");
  oss << date_format ( "%m/%d/%Y" ) << f;
  assert ( oss . str () == "05/12/2000" );
  oss . str ("");
  oss << date_format ( "%Y%m%d" ) << f;
  assert ( oss . str () == "20000512" );
  oss . str ("");
  oss << date_format ( "hello kitty" ) << f;
  assert ( oss . str () == "hello kitty" );
  oss . str ("");
  oss << date_format ( "%d%d%d%d%d%d%m%m%m%Y%Y%Y%%%%%%%%%%" ) << f;
  assert ( oss . str () == "121212121212050505200020002000%%%%%" );
  oss . str ("");
  oss << date_format ( "%Y-%m-%d" ) << f;
  assert ( oss . str () == "2000-05-12" );
  iss . clear ();
  iss . str ( "2001-01-1" );
  assert ( ! ( iss >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2000-05-12" );
  iss . clear ();
  iss . str ( "2001-1-01" );
  assert ( ! ( iss >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2000-05-12" );
  iss . clear ();
  iss . str ( "2001-001-01" );
  assert ( ! ( iss >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2000-05-12" );
  iss . clear ();
  iss . str ( "2001-01-02" );
  assert ( ( iss >> date_format ( "%Y-%m-%d" ) >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2001-01-02" );
  iss . clear ();
  iss . str ( "05.06.2003" );
  assert ( ( iss >> date_format ( "%d.%m.%Y" ) >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2003-06-05" );
  iss . clear ();
  iss . str ( "07/08/2004" );
  assert ( ( iss >> date_format ( "%m/%d/%Y" ) >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2004-07-08" );
  iss . clear ();
  iss . str ( "2002*03*04" );
  assert ( ( iss >> date_format ( "%Y*%m*%d" ) >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2002-03-04" );
  iss . clear ();
  iss . str ( "C++09format10PA22006rulez" );
  assert ( ( iss >> date_format ( "C++%mformat%dPA2%Yrulez" ) >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2006-09-10" );
  iss . clear ();
  iss . str ( "%12%13%2010%" );
  assert ( ( iss >> date_format ( "%%%m%%%d%%%Y%%" ) >> f ) );
  oss . str ("");
  oss << f;
  assert ( oss . str () == "2010-12-13" );

  CDate g ( 2000, 6, 8 );
  iss . clear ();
  iss . str ( "2001-11-33" );
  assert ( ! ( iss >> date_format ( "%Y-%m-%d" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-06-08" );
  iss . clear ();
  iss . str ( "29.02.2003" );
  assert ( ! ( iss >> date_format ( "%d.%m.%Y" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-06-08" );
  iss . clear ();
  iss . str ( "14/02/2004" );
  assert ( ! ( iss >> date_format ( "%m/%d/%Y" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-06-08" );
  iss . clear ();
  iss . str ( "2002-03" );
  assert ( ! ( iss >> date_format ( "%Y-%m" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-06-08" );
  iss . clear ();
  iss . str ( "hello kitty" );
  assert ( ! ( iss >> date_format ( "hello kitty" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-06-08" );
  iss . clear ();
  iss . str ( "2005-07-12-07" );
  assert ( ! ( iss >> date_format ( "%Y-%m-%d-%m" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-06-08" );
  iss . clear ();
  iss . str ( "20000101" );
  assert ( ( iss >> date_format ( "%Y%m%d" ) >> g ) );
  oss . str ("");
  oss << g;
  assert ( oss . str () == "2000-01-01" );

  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
