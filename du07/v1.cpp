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

class CRange {
public:
    CRange(size_t from, size_t to) : m_From(from), m_To(to) {
        if (from > to) throw std::invalid_argument("invalid range");
    }
    size_t m_From;
    size_t m_To;
};

class CRangeRev {
public:
    CRangeRev(size_t from, size_t to) : m_From(from), m_To(to) {
        if (from < to) throw std::invalid_argument("invalid range");
    }
    size_t m_From;
    size_t m_To;
};
#endif /* __PROGTEST__ */

namespace detail {
    template <typename Type, size_t D>
    struct NestedList {
        using type = std::initializer_list<typename NestedList<Type, D - 1>::type>;
    };
    template <typename Type>
    struct NestedList<Type, 1> {
        using type = std::initializer_list<Type>;
    };

    struct ViewParam {
        bool isFixed = false;
        size_t fixedIdx = 0;
        bool isRev = false;
        size_t lo = 0, hi = 0;
        bool omitted = true;

        ViewParam() = default;
        ViewParam(size_t i) : isFixed(true), fixedIdx(i), omitted(false) {}
        ViewParam(int i) : isFixed(true), fixedIdx(static_cast<size_t>(i)), omitted(false) {}
        ViewParam(CRange r) : lo(r.m_From), hi(r.m_To), omitted(false) {}
        ViewParam(CRangeRev r) : isRev(true), lo(r.m_From), hi(r.m_To), omitted(false) {}
    };

    template <typename T>
    void printData(std::ostream& os, const T* data, const size_t* dims, const ptrdiff_t* strides, 
                   size_t totalDims, size_t currentDim, size_t offset, int indent) {
        auto printIndent = [&os](int levels) { for(int i = 0; i < levels * 2; ++i) os << ' '; };
        
        printIndent(indent);
        os << "{";
        if (currentDim == totalDims - 1) {
            for (size_t i = 0; i < dims[currentDim]; ++i) {
                os << data[offset + i * strides[currentDim]];
                if (i < dims[currentDim] - 1) os << ", ";
            }
            os << "}";
        } else {
            os << "\n";
            for (size_t i = 0; i < dims[currentDim]; ++i) {
                printData(os, data, dims, strides, totalDims, currentDim + 1, 
                          offset + i * strides[currentDim], indent + 1);
                if (i < dims[currentDim] - 1) os << ",\n";
                else os << "\n";
            }
            printIndent(indent);
            os << "}";
        }
    }
}

template <typename T_, size_t VIEW_DIM_>
class CTensorView {
public:
    T_* m_DataPtr;
    size_t m_BaseOffset;
    size_t m_Dims[VIEW_DIM_];
    ptrdiff_t m_Strides[VIEW_DIM_];
    size_t m_OriginalDim;

    CTensorView(T_* data, size_t origDim) : m_DataPtr(data), m_BaseOffset(0), m_OriginalDim(origDim) {
        for(size_t i = 0; i < VIEW_DIM_; ++i) { m_Dims[i] = 0; m_Strides[i] = 0; }
    }

    template <typename... Indices, typename = std::enable_if_t<sizeof...(Indices) == VIEW_DIM_>>
    T_& operator()(Indices... indices) {
        size_t idx[] = { static_cast<size_t>(indices)... };
        size_t flat = m_BaseOffset;
        for (size_t i = 0; i < VIEW_DIM_; ++i) {
            if (idx[i] >= m_Dims[i]) throw std::out_of_range("out of bounds");
            flat += idx[i] * m_Strides[i];
        }
        return m_DataPtr[flat];
    }

    template <typename... Indices, typename = std::enable_if_t<sizeof...(Indices) == VIEW_DIM_>>
    const T_& operator()(Indices... indices) const {
        size_t idx[] = { static_cast<size_t>(indices)... };
        size_t flat = m_BaseOffset;
        for (size_t i = 0; i < VIEW_DIM_; ++i) {
            if (idx[i] >= m_Dims[i]) throw std::out_of_range("out of bounds");
            flat += idx[i] * m_Strides[i];
        }
        return m_DataPtr[flat];
    }

    size_t originalTensorDimension() const { return m_OriginalDim; }

    template <typename... Args>
    auto slice(Args... args) const {
        static_assert(sizeof...(Args) <= VIEW_DIM_, "Too many arguments");
        constexpr size_t NEW_DIM = VIEW_DIM_ - (std::is_integral_v<Args> + ... + 0);
        CTensorView<T_, NEW_DIM> view(m_DataPtr, m_OriginalDim);
        
        detail::ViewParam params[VIEW_DIM_]; 
        if constexpr (sizeof...(Args) > 0) {
            detail::ViewParam provided[] = { detail::ViewParam(args)... };
            for(size_t i = 0; i < sizeof...(Args); ++i) params[i] = provided[i];
        }

        size_t newOffset = m_BaseOffset;
        size_t outIdx = 0;

        for (size_t i = 0; i < VIEW_DIM_; ++i) {
            if (params[i].omitted) params[i] = detail::ViewParam(CRange(0, m_Dims[i] - 1));

            if (params[i].isFixed) {
                if (params[i].fixedIdx >= m_Dims[i]) throw std::out_of_range("out of bounds");
                newOffset += params[i].fixedIdx * m_Strides[i];
            } else {
                if (params[i].lo >= m_Dims[i] || params[i].hi >= m_Dims[i]) throw std::out_of_range("out of bounds");
                if (params[i].isRev) {
                    newOffset += params[i].lo * m_Strides[i];
                    view.m_Strides[outIdx] = -m_Strides[i];
                    view.m_Dims[outIdx] = params[i].lo - params[i].hi + 1;
                } else {
                    newOffset += params[i].lo * m_Strides[i];
                    view.m_Strides[outIdx] = m_Strides[i];
                    view.m_Dims[outIdx] = params[i].hi - params[i].lo + 1;
                }
                outIdx++;
            }
        }
        view.m_BaseOffset = newOffset;
        return view;
    }

    friend std::ostream& operator<<(std::ostream& os, const CTensorView& v) {
        detail::printData(os, v.m_DataPtr, v.m_Dims, v.m_Strides, VIEW_DIM_, 0, v.m_BaseOffset, 0);
        return os;
    }
};

template <typename T_, size_t DIM_>
class CTensor {
private:
    std::vector<T_> m_Data;
    size_t m_Dims[DIM_];
    ptrdiff_t m_Strides[DIM_];

    void computeStrides() {
        ptrdiff_t current = 1;
        for (int i = DIM_ - 1; i >= 0; --i) {
            m_Strides[i] = current;
            current *= m_Dims[i];
        }
    }

    template <typename ListType>
    void unpackList(const ListType& list, size_t currentDim) {
        if (currentDim >= DIM_) return;
        if (m_Dims[currentDim] == 0) m_Dims[currentDim] = list.size();
        else if (m_Dims[currentDim] != list.size()) throw std::invalid_argument("invalid hypercube");

        if constexpr (std::is_same_v<typename ListType::value_type, T_>) {
            for (const auto& item : list) m_Data.push_back(item);
        } else {
            for (const auto& sublist : list) unpackList(sublist, currentDim + 1);
        }
    }

public:
    template <typename... Sizes, typename = std::enable_if_t<sizeof...(Sizes) == DIM_>>
    CTensor(const T_& v, Sizes... sizes) {
        size_t dims[] = { static_cast<size_t>(sizes)... };
        size_t total = 1;
        for(size_t i = 0; i < DIM_; ++i) {
            m_Dims[i] = dims[i];
            total *= dims[i];
        }
        m_Data.assign(total, v);
        computeStrides();
    }

    CTensor(typename detail::NestedList<T_, DIM_>::type list) {
        for(size_t i = 0; i < DIM_; ++i) m_Dims[i] = 0;
        unpackList(list, 0);
        computeStrides();
    }

    template <typename... Indices, typename = std::enable_if_t<sizeof...(Indices) == DIM_>>
    T_& operator()(Indices... indices) {
        size_t idx[] = { static_cast<size_t>(indices)... };
        size_t flat = 0;
        for (size_t i = 0; i < DIM_; ++i) {
            if (idx[i] >= m_Dims[i]) throw std::out_of_range("out of bounds");
            flat += idx[i] * m_Strides[i];
        }
        return m_Data[flat];
    }

    template <typename... Indices, typename = std::enable_if_t<sizeof...(Indices) == DIM_>>
    const T_& operator()(Indices... indices) const {
        size_t idx[] = { static_cast<size_t>(indices)... };
        size_t flat = 0;
        for (size_t i = 0; i < DIM_; ++i) {
            if (idx[i] >= m_Dims[i]) throw std::out_of_range("out of bounds");
            flat += idx[i] * m_Strides[i];
        }
        return m_Data[flat];
    }

    template <typename... Args>
    auto slice(Args... args) {
        static_assert(sizeof...(Args) <= DIM_, "Too many arguments");
        constexpr size_t NEW_DIM = DIM_ - (std::is_integral_v<Args> + ... + 0);
        CTensorView<T_, NEW_DIM> view(m_Data.data(), DIM_);
        
        detail::ViewParam params[DIM_]; 
        if constexpr (sizeof...(Args) > 0) {
            detail::ViewParam provided[] = { detail::ViewParam(args)... };
            for(size_t i = 0; i < sizeof...(Args); ++i) params[i] = provided[i];
        }

        size_t baseOffset = 0;
        size_t outIdx = 0;

        for (size_t i = 0; i < DIM_; ++i) {
            if (params[i].omitted) params[i] = detail::ViewParam(CRange(0, m_Dims[i] - 1));

            if (params[i].isFixed) {
                if (params[i].fixedIdx >= m_Dims[i]) throw std::out_of_range("out of bounds");
                baseOffset += params[i].fixedIdx * m_Strides[i];
            } else {
                if (params[i].lo >= m_Dims[i] || params[i].hi >= m_Dims[i]) throw std::out_of_range("out of bounds");
                if (params[i].isRev) {
                    baseOffset += params[i].lo * m_Strides[i];
                    view.m_Strides[outIdx] = -m_Strides[i];
                    view.m_Dims[outIdx] = params[i].lo - params[i].hi + 1;
                } else {
                    baseOffset += params[i].lo * m_Strides[i];
                    view.m_Strides[outIdx] = m_Strides[i];
                    view.m_Dims[outIdx] = params[i].hi - params[i].lo + 1;
                }
                outIdx++;
            }
        }
        view.m_BaseOffset = baseOffset;
        return view;
    }

    friend std::ostream& operator<<(std::ostream& os, const CTensor& t) {
        detail::printData(os, t.m_Data.data(), t.m_Dims, t.m_Strides, DIM_, 0, 0, 0);
        return os;
    }
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
