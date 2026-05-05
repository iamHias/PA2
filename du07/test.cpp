#include <iostream>
#include <type_traits>
#include <vector>

struct CRange { size_t m_From, m_To; CRange(size_t a, size_t b):m_From(a),m_To(b){} };
struct CRangeRev { size_t m_From, m_To; CRangeRev(size_t a, size_t b):m_From(a),m_To(b){} };

template <typename T>
constexpr size_t count_dim() {
    using ArgT = std::decay_t<T>;
    if constexpr (std::is_same_v<ArgT, CRange> || std::is_same_v<ArgT, CRangeRev>) return 1;
    else return 0;
}

int main() {
    std::cout << count_dim<CRange>() << std::endl;
    std::cout << count_dim<int>() << std::endl;
    std::cout << count_dim<size_t>() << std::endl;
}
