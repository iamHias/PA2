#ifndef __PROGTEST__
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <deque>
#include <algorithm>
#include <memory>
#include <compare>
#include <stdexcept>
#include <optional>
#include <limits>
#include <array>
#include <cstdint> // Required for fixed-width integers (uint64_t, etc.)
using namespace std::literals;
#endif 

// ==========================================
// DOMAIN ALIASES
// ==========================================
using Zone    = std::string;                     ///< Identifies a physical sector in the military base.
using Name    = std::string;                     ///< The full name of a tracked individual.
using Weight  = int;                             ///< Time delay in minutes for passing a gate.
using Edge    = std::pair<Zone, Weight>;         ///< Connection to an adjacent zone and its traversal cost.
using Graph   = std::unordered_map<Zone, std::vector<Edge>>; ///< Graph topology mapping zones to their connections.
using DistMap = std::unordered_map<Zone, Weight>; ///< Maps zones to the minimum calculated travel time.
using Results = std::set<Name>;                  ///< Set of visitor names fulfilling the query.

// ==========================================
// TIME MANAGEMENT
// ==========================================

/**
 * @brief Checks if a year is a Gregorian leap year.
 * @param y The year to evaluate.
 * @return True if leap year, false otherwise.
 */
static bool isLeap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

/**
 * @brief Gets days in a specific month, accounting for leap years.
 * @param y The year.
 * @param m The month (1-12).
 * @return The number of days in the specified month.
 */
static int moDays(int y, int m) {
    static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && isLeap(y)) ? 29 : days[m];
}

/**
 * @struct Time
 * @brief Parses and normalizes date-time components into comparable scalar representations.
 */
struct Time {
    int y, m, d, h, min;
    
    /**
     * @brief Validates date logic against strict bounds.
     * @return True if the date components form a physically possible Gregorian date.
     */
    bool valid() const {
        return (y >= 1900 && y <= 9999 && 
                m >= 1 && m <= 12 && d >= 1 && d <= moDays(y, m) &&
                h >= 0 && h <= 23 && min >= 0 && min <= 59);
    }

    /**
     * @brief Converts calendar date into total continuous minutes elapsed since Jan 1, 1900.
     * @details Pre-computes years dynamically on first run into a static lookup table 
     * to ensure O(1) performance for year accumulation across heavy parsing workloads.
     * @return Total minutes.
     */
    uint64_t mins() const {
        static const auto yearDays = []() {
            std::array<uint32_t, 8101> a{}; 
            for (size_t i = 1; i < a.size(); ++i) {
                a[i] = a[i-1] + (isLeap(1900 + i - 1) ? 366 : 365);
            }
            return a;
        }();
        
        uint64_t total = yearDays[y - 1900];
        for (int i = 1; i < m; ++i) total += moDays(y, i);
        return (total + d - 1) * 24 * 60 + h * 60 + min;
    }
};

/**
 * @struct Visit
 * @brief Represents a single continuous, unbroken stay inside the base.
 */
struct Visit {
    uint64_t entryTime;       ///< Entry time in scalar minutes.
    uint64_t exitTime;      ///< Exit time in scalar minutes (UINT64_MAX if they never left).
    Zone gate_in;      ///< The border zone entered through.
    Zone gate_out;     ///< The border zone exited through.
};

/// Maps a person's name to a chronological vector of their base visits.
using VisitMap = std::unordered_map<Name, std::vector<Visit>>;

// ==========================================
// CORE CLASSES
// ==========================================

/**
 * @class CAuditFilter
 * @brief Builder class for defining search bounds and the target zone.
 */
class CAuditFilter {
public:
    /**
     * @brief Constructor sets the target zone and initializes default open bounds.
     * @param target The target zone string.
     */
    CAuditFilter(Zone target) 
        : m_target(std::move(target)), m_start(0), m_end(UINT64_MAX) {} 

    /**
     * @brief Sets the lower bound for the visit overlap. Retains the strictest (highest) bound if chained.
     */
    CAuditFilter& notBefore(int y, int m, int d, int h, int mi) {
        m_start = std::max(m_start, Time{y, m, d, h, mi}.mins());
        return *this;
    }

    /**
     * @brief Sets the upper bound for the visit overlap. Retains the strictest (lowest) bound if chained.
     */
    CAuditFilter& notAfter(int y, int m, int d, int h, int mi) {
        m_end = std::min(m_end, Time{y, m, d, h, mi}.mins());
        return *this;
    }

    const Zone& target() const { return m_target; }
    uint64_t start() const { return m_start; }
    uint64_t end() const { return m_end; }

private:
    Zone m_target;     ///< Destination zone to query.
    uint64_t m_start;  ///< Earliest acceptable minute interval.
    uint64_t m_end;    ///< Latest acceptable minute interval.
};

/**
 * @class CVisitorLog
 * @brief Processed query engine holding parsed topology and user visit logs.
 */
class CVisitorLog {
public:
    /**
     * @brief Constructs the query engine using move semantics to securely claim memory.
     */
    CVisitorLog(Graph g, VisitMap v) 
        : graph(std::move(g)), visits(std::move(v)) {}

    /**
     * @brief Calculates which visitors reached the target zone during the allowed time filter.
     * @param filter Specifications of the zone and time bounds.
     * @return Set of uniquely identified names.
     */
    Results search(const CAuditFilter& filter) const {
        Results res;
        // Fetch pre-calculated (or lazy-calculated) path weights.
        const DistMap& paths = getPaths(filter.target());

        for (const auto& [person, vList] : visits) {
            for (const auto& v : vList) {
                // Verify entry path
                auto itIn = paths.find(v.gate_in);
                if (itIn == paths.end()) continue; 
                
                uint64_t arrive = v.entryTime + itIn->second;
                uint64_t depart = v.exitTime;

                // Deduct traversal time required to make it to the exit gate
                if (v.exitTime != UINT64_MAX) {
                    auto itOut = paths.find(v.gate_out);
                    // Invalid if exit path doesn't exist or traversal takes longer than their stay
                    if (itOut == paths.end() || v.exitTime < static_cast<uint64_t>(itOut->second)) continue;
                    depart = v.exitTime - itOut->second;
                }
                
                // Interval Intersection verification
                if (arrive <= depart) {
                    uint64_t overlapA = std::max(arrive, filter.start());
                    uint64_t overlapB = std::min(depart, filter.end());
                    
                    if (overlapA <= overlapB) {
                        res.insert(person);
                        break; // Short-circuit: they matched, move to next person.
                    }
                }
            }
        }
        return res;
    }

private:
    Graph graph;
    VisitMap visits;
    
    /// Thread-safe cache using lazy evaluation to hold Reverse-Dijkstra results.
    mutable std::unordered_map<Zone, DistMap> pathCache;

    /**
     * @brief Caching wrapper calculating minimum travel time from the Target to all gates.
     * @param start The target zone of the current query.
     * @return Zero-copy reference to the computed distance map.
     */
    const DistMap& getPaths(const Zone& start) const {
        auto [it, inserted] = pathCache.try_emplace(start);
        
        if (inserted) {
            DistMap dists;
            dists[start] = 0; // Essential for isolated target zones without outgoing edges

            if (graph.count(start)) {
                using PQEl = std::pair<Weight, Zone>;
                std::priority_queue<PQEl, std::vector<PQEl>, std::greater<PQEl>> pq;
                pq.push({0, start});

                while (!pq.empty()) {
                    auto [d, curr] = pq.top();
                    pq.pop();

                    if (d > dists[curr]) continue;

                    for (const auto& [nb, w] : graph.at(curr)) {
                        int newD = d + w;
                        if (!dists.count(nb) || newD < dists[nb]) {
                            dists[nb] = newD;
                            pq.push({newD, nb});
                        }
                    }
                }
            }
            it->second = std::move(dists);
        }
        return it->second;
    }
};

/**
 * @class CMilBase
 * @brief File parser and factory for constructing the query engine.
 */
class CMilBase {
public:
    /**
     * @brief Parses and constructs the internal graph representation of the base.
     * @param file Path to the txt base file.
     * @throws std::runtime_error If file cannot be read or formatting is malformed.
     */
    void readBase(const std::string& file) {
        std::ifstream f(file);
        if (!f.is_open()) throw std::runtime_error("No base file");

        Zone a, b;
        Weight w;
        while (f >> a >> b >> w) {
            graph[a].push_back({b, w});
            graph[b].push_back({a, w}); // Add undirected edges
        }
        if (!f.eof() && f.fail()) throw std::runtime_error("Bad base format");
    }

    /**
     * @brief Multiplexing parser router for TEXT, IIII, and MMMM log streams.
     * @param file Path to the log file.
     * @return Fully instanced CVisitorLog loaded with the current base layout and parsed history.
     */
    CVisitorLog processLog(const std::string& file) const {
        std::ifstream f(file, std::ios::binary);
        if (!f.is_open()) throw std::runtime_error("No log file");

        std::vector<RawLog> logs;

        while (true) {
            char magic[4];
            if (!f.read(magic, 4)) break; // Standard safe EOF exit point

            if (std::strncmp(magic, "TEXT", 4) == 0) {
                parseTextChunk(f, logs);
            } 
            else if (std::strncmp(magic, "IIII", 4) == 0 || std::strncmp(magic, "MMMM", 4) == 0) {
                parseBinaryChunk(f, magic[0] == 'I', logs);
            } 
            else {
                throw std::runtime_error("Unknown format");
            }
        }

        return CVisitorLog(graph, mapVisits(logs));
    }

private:
    Graph graph;
    
    /// @brief Internal flattened event-log structure before visit interval pairing.
    struct RawLog { uint64_t mins; Zone zone; Name name; };

    /**
     * @brief Handles standard TEXT block extraction.
     */
    void parseTextChunk(std::ifstream& f, std::vector<RawLog>& logs) const {
        Zone zone; 
        int count;
        if (!(f >> zone >> count)) throw std::runtime_error("Bad text header");
        
        // Clear the line buffer correctly before getline begins.
        f.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        for (int i = 0; i < count; ++i) {
            std::string line;
            if (!std::getline(f, line)) throw std::runtime_error("Incomplete log");
            
            std::istringstream iss(line);
            int y, m, d, h, mi;
            char dash1, dash2, colon;
            std::string personName;
            
            if (!(iss >> y >> dash1 >> m >> dash2 >> d >> h >> colon >> mi) ||
                dash1 != '-' || dash2 != '-' || colon != ':' ||
                !(std::getline(iss >> std::ws, personName))) {
                throw std::runtime_error("Invalid text format");
            }
            
            // Critical patch: Ensure Windows \r doesn't poison the name map keys.
            if (!personName.empty() && personName.back() == '\r') {
                personName.pop_back();
            }
            
            Time ts{y, m, d, h, mi};
            if (!ts.valid()) throw std::runtime_error("Bad date");
            logs.push_back({ts.mins(), zone, personName});
        }
    }

    /**
     * @brief Endian-aware binary parser.
     * @param isLE Boolean flag indicating if current chunk acts as Little-Endian.
     */
    void parseBinaryChunk(std::ifstream& f, bool isLE, std::vector<RawLog>& logs) const {
        Zone zone = readStr(f, readInt<uint16_t>(f, isLE));
        uint32_t count = readInt<uint32_t>(f, isLE);

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t dt = readInt<uint32_t>(f, isLE);
            
            // Decodes the bit-shifted datetime layout (Y:12 | M:4 | D:5 | H:5 | min:6)
            Time ts {
                static_cast<int>((dt >> 20) & 0xFFF),
                static_cast<int>((dt >> 16) & 0x0F),
                static_cast<int>((dt >> 11) & 0x1F),
                static_cast<int>((dt >> 6)  & 0x1F),
                static_cast<int>(dt & 0x3F)
            };
            if (!ts.valid()) throw std::runtime_error("Bad date");
            
            Name personName = readStr(f, readInt<uint16_t>(f, isLE));
            logs.push_back({ts.mins(), zone, personName});
        }
    }

    /**
     * @brief Safely reads integer types agnostic of host architecture, applying manual bitwise alignment.
     */
    template<typename T>
    T readInt(std::ifstream& f, bool isLE) const {
        T v = 0;
        uint8_t b[sizeof(T)];
        if (!f.read(reinterpret_cast<char*>(b), sizeof(T))) throw std::runtime_error("EOF");
        for (size_t i = 0; i < sizeof(T); ++i) {
            size_t shift = isLE ? (i * 8) : ((sizeof(T) - 1 - i) * 8);
            v |= (static_cast<T>(b[i]) << shift);
        }
        return v;
    }

    /**
     * @brief Guaranteed-length string extraction avoiding null-termination buffer risks.
     */
    std::string readStr(std::ifstream& f, size_t len) const {
        std::string s(len, '\0');
        if (!f.read(s.data(), len)) throw std::runtime_error("EOF");
        return s;
    }

    /**
     * @brief Transforms flattened entries/exits into structured Intervals (Visits).
     * @param raw An unsorted vector of independent log entries.
     * @return Map linking user names to sequentially ordered Visit structures mapping Entry -> Exit gates.
     */
    VisitMap mapVisits(std::vector<RawLog>& raw) const {
        std::unordered_map<Name, std::vector<RawLog>> byName;
        for (const auto& r : raw) byName[r.name].push_back(r);

        VisitMap vm;
        for (auto& [n, personLogs] : byName) {
            std::sort(personLogs.begin(), personLogs.end(), [](const auto& a, const auto& b) { return a.mins < b.mins; });
            
            // Step by 2 representing an Entry-Exit cycle constraint
            for (size_t i = 0; i < personLogs.size(); i += 2) {
                uint64_t out = (i + 1 < personLogs.size()) ? personLogs[i+1].mins : UINT64_MAX;
                Zone outZone = (i + 1 < personLogs.size()) ? personLogs[i+1].zone : "";
                
                vm[n].push_back({personLogs[i].mins, out, personLogs[i].zone, outZone});
            }
        }
        return vm;
    }
};

#ifndef __PROGTEST__
void basicTests ( const CVisitorLog & log )
{
  assert ( log . search ( CAuditFilter ( "headquarters" ) )
           == ( std::set<std::string> { "Alice Cooper", "George Peterson", "Henry Montgomery", "Jane Bush", "John Smith", "Tim Cook", "Robert Smith" } ) );
  assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) )
           == ( std::set<std::string> { "Alice Cooper", "Henry Montgomery", "Jane Bush", "John Smith", "Robert Smith" } ) );
  assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notAfter ( 2026, 3, 10, 8, 0 ) )
           == ( std::set<std::string> { "Henry Montgomery", "Robert Smith" } ) );
  assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2026, 3, 11, 12, 0 ) )
           == ( std::set<std::string> { "Henry Montgomery", "Jane Bush", "John Smith" } ) );
  assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2026, 3, 10, 9, 0 ) . notAfter ( 2026, 3, 10, 13, 0 ) )
           == ( std::set<std::string> { "Alice Cooper", "Henry Montgomery", "Jane Bush", "John Smith" } ) );
  assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2026, 3, 10, 9, 5 ) . notAfter ( 2026, 3, 10, 9, 5 ) )
           == ( std::set<std::string> { "Henry Montgomery" } ) );
  assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2026, 3, 10, 9, 6 ) . notAfter ( 2026, 3, 10, 9, 6 ) )
           == ( std::set<std::string> { "Henry Montgomery", "John Smith" } ) );
  assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2026, 3, 10, 9, 24 ) . notAfter ( 2026, 3, 10, 9, 24 ) )
           == ( std::set<std::string> { "Alice Cooper", "Henry Montgomery", "John Smith" } ) );
  assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2026, 3, 10, 9, 25 ) . notAfter ( 2026, 3, 10, 9, 25 ) )
           == ( std::set<std::string> { "Alice Cooper", "Henry Montgomery" } ) );
  assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2024, 2, 1, 0, 0 ) . notAfter ( 2024, 3, 31, 0, 0 ) )
           == ( std::set<std::string> { "Robert Smith", "Henry Montgomery" } ) );
  assert ( log . search ( CAuditFilter ( "flyingSaucerHangar" ) . notBefore ( 2025, 2, 1, 0, 0 ) . notAfter ( 2025, 3, 31, 0, 0 ) )
           == ( std::set<std::string> { "Henry Montgomery" } ) );
  assert ( log . search ( CAuditFilter ( "privateParking" )  )
           == ( std::set<std::string> { "<classified>" } ) );
}

int main ()
{
  class CMilBase b;
  b . readBase ( "base.txt" );

  for ( const char * fn : std::initializer_list<const char *> { "in1.log", "in2.log", "in3.log" } )
    basicTests ( b . processLog ( fn ) );
  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */