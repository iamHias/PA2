#ifndef __PROGTEST__
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <memory>
#include <optional>
#include <iterator>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <cassert>

class CRange
{
  public:
                                       CRange                                  ( size_t                                from,
                                                                                 size_t                                to  )
      : m_From ( from ),
        m_To  ( to )
    {
      if ( from > to )
        throw std::invalid_argument ( "invalid range" );
    }
    size_t                             m_From;
    size_t                             m_To;
};

class CRangeRev
{
  public:
                                       CRangeRev                               ( size_t                                from,
                                                                                 size_t                                to  )
      : m_From ( from ),
        m_To  ( to )
    {
      if ( from < to )
        throw std::invalid_argument ( "invalid range" );
    }
    size_t                             m_From;
    size_t                             m_To;
};
#endif /* __PROGTEST__ */

template <typename T_, size_t DIM_>
class CTensor
{
  public:
    // constructor -- create + set all elements
    // constructor -- from initializer_list
    // destructor (opt)
    // slice ()
    // operator ()
    // operator <<
  private:
    // todo
};

template <typename T_>
class CTensorView
{
  public:
    // operator ()
    // operator <<
  private:
    // todo
};

#ifndef __PROGTEST__
template <typename T_>
std::string  toString ( const T_ & x )
{
  std::ostringstream oss;
  oss << x;
  return oss . str ();
}

int main ()
{
  CTensor<int,2> m1 ( 0, 3, 5 );
  CTensor<int,2> m2 { { 1, 2, 3 }, { 4, 5, 6 } };
  CTensor<int,3> m3
  {
    {
       { 10, 11, 12, 13, 14 },
       { 15, 16, 17, 18, 19 },
       { 20, 21, 22, 23, 24 },
       { 25, 26, 27, 28, 29 },
    },
    {
       { 31, 32, 33, 34, 35 },
       { 36, 37, 38, 39, 40 },
       { 41, 42, 43, 44, 45 },
       { 46, 47, 48, 49, 50 },
    },
    {
       { 60, 61, 62, 63, 64 },
       { 65, 66, 67, 68, 69 },
       { 70, 71, 72, 73, 74 },
       { 75, 76, 77, 78, 79 },
    }
  };
  CTensor<std::string,2> m4
  {
    { "test", "progtest" },
    { "PA1", "PA2" }
  };

  assert ( toString ( m1 ) == R"({
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0}
})" );

  assert ( toString ( m2 ) == R"({
  {1, 2, 3},
  {4, 5, 6}
})" );

  assert ( toString ( m3 ) == R"({
  {
    {10, 11, 12, 13, 14},
    {15, 16, 17, 18, 19},
    {20, 21, 22, 23, 24},
    {25, 26, 27, 28, 29}
  },
  {
    {31, 32, 33, 34, 35},
    {36, 37, 38, 39, 40},
    {41, 42, 43, 44, 45},
    {46, 47, 48, 49, 50}
  },
  {
    {60, 61, 62, 63, 64},
    {65, 66, 67, 68, 69},
    {70, 71, 72, 73, 74},
    {75, 76, 77, 78, 79}
  }
})" );

  assert ( toString ( m4 ) == R"({
  {test, progtest},
  {PA1, PA2}
})" );

  assert ( toString ( m2 . slice ( CRange ( 0, 1 ), CRange ( 1, 2 ) ) ) == R"({
  {2, 3},
  {5, 6}
})" );
  assert ( toString ( m2 . slice ( 0, CRange ( 1, 2 ) ) ) == "{2, 3}" );
  assert ( toString ( m2 . slice ( 1 ) ) == "{4, 5, 6}" );
  assert ( toString ( m2 . slice ( CRange ( 0, 1 ), 2 ) ) == "{3, 6}" );

  auto v3 = m3 . slice ( CRange ( 1, 2 ), CRange ( 0, 2 ), CRange ( 0, 3 ) );
  assert ( toString ( v3 ) == R"({
  {
    {31, 32, 33, 34},
    {36, 37, 38, 39},
    {41, 42, 43, 44}
  },
  {
    {60, 61, 62, 63},
    {65, 66, 67, 68},
    {70, 71, 72, 73}
  }
})" );

  auto v2 = m3 . slice ( 1, CRange ( 0, 2 ), CRange ( 0, 3 ) );
  assert ( toString ( v2 ) == R"({
  {31, 32, 33, 34},
  {36, 37, 38, 39},
  {41, 42, 43, 44}
})" );


  v2 = m3 . slice ( CRange ( 1, 2 ), 2, CRange ( 0, 3 ) );
  assert ( toString ( v2 ) == R"({
  {41, 42, 43, 44},
  {70, 71, 72, 73}
})" );

  v2 = m3 . slice ( CRange ( 1, 2 ), CRange ( 0, 2 ), 3 );
  assert ( toString ( v2 ) == R"({
  {34, 39, 44},
  {63, 68, 73}
})" );


  v2 = m3 . slice ( CRangeRev ( 2, 0 ), 1, CRange ( 4, 4 ) );
  assert ( toString ( v2 ) == R"({
  {69},
  {40},
  {19}
})" );

  v2 = m3 . slice ( CRangeRev ( 2, 0 ), 2 );
  assert ( toString ( v2 ) == R"({
  {70, 71, 72, 73, 74},
  {41, 42, 43, 44, 45},
  {20, 21, 22, 23, 24}
})" );

  assert ( v2 ( 1, 1 ) == 42 );
  v2 ( 1, 1 ) = 666;
  assert ( v2 ( 1, 1 ) == 666 );
  assert ( toString ( v2 ) == R"({
  {70, 71, 72, 73, 74},
  {41, 666, 43, 44, 45},
  {20, 21, 22, 23, 24}
})" );

  assert ( toString ( m3 ) == R"({
  {
    {10, 11, 12, 13, 14},
    {15, 16, 17, 18, 19},
    {20, 21, 22, 23, 24},
    {25, 26, 27, 28, 29}
  },
  {
    {31, 32, 33, 34, 35},
    {36, 37, 38, 39, 40},
    {41, 666, 43, 44, 45},
    {46, 47, 48, 49, 50}
  },
  {
    {60, 61, 62, 63, 64},
    {65, 66, 67, 68, 69},
    {70, 71, 72, 73, 74},
    {75, 76, 77, 78, 79}
  }
})" );

  auto v1 = m3 . slice ( 1, 2, CRange ( 1, 2 ) );
  assert ( toString ( v1 ) == "{666, 43}" );

  v1 = m3 . slice ( 1, 2, CRange ( 1, 1 ) );
  assert ( toString ( v1 ) == "{666}" );

  v1 = m3 . slice ( CRange ( 0, 2 ), 2, 3 );
  assert ( toString ( v1 ) == "{23, 44, 73}" );

  assert ( toString ( m4 . slice ( CRangeRev ( 1, 0 ), CRangeRev ( 1, 0 ) ) ) == R"({
  {PA2, PA1},
  {progtest, test}
})" );

  try
  {
    m3 ( 2, 3, 5 ) ++;
    assert ( "missing an exception" == nullptr );
  }
  catch ( const std::exception & e )
  {
  }

  try
  {
    v2 ( 2, 5 ) --;
    assert ( "missing an exception" == nullptr );
  }
  catch ( const std::exception & e )
  {
  }

  try
  {
    CTensor<int, 2> m5
    {
      { 3, 5, 8 },
      { 2, 7 }
    };
    assert ( "missing an exception" == nullptr );
  }
  catch ( const std::exception & e )
  {
  }

  try
  {
    m1 . slice ( CRange ( 0, 4 ), CRange ( 0, 2 ) );
    assert ( "missing an exception" == nullptr );
  }
  catch ( const std::exception & e )
  {
  }

  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
