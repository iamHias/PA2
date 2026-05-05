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
using namespace std::literals;
#endif /* __PROGTEST__ */

// (=^･^=)ฅ^•ﻌ•^ฅ(=^･^=)
//     NAME ALIASES
// (=^･^=)ฅ^•ﻌ•^ฅ(=^･^=)

using Zone    = std::string;
using Name    = std::string;
using Weight  = int; 
using Edge    = std::pair<Zone, Weight>;
using Graph   = std::unordered_map<Zone, std::vector<Edge>>;
using DistMap = std::unordered_map<Zone, Weight>;
using Results = std::set<Name>;


// (=^･^=)ฅ^•ﻌ•^ฅ(=^･^=)
//      CORE MODEL
// (=^･^=)ฅ^•ﻌ•^ฅ(=^･^=)

/**
 * @brief Check if a year is a Gregorian leap year
 * @param y year to check
 * @return if the year is leap, return true, else false
 */

static bool isLeap (int y) {
    return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
}

/**
 * @brief  Get days from the parameter, including the leap years
 * @param y Year (1900 ~ 2100)
 * @param m Month (1 - 12)
 */

static int monthToDays(int y, int m) {
    static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && isLeap(y)) ? 29 : days[m];
}

/**
 * @struct Time
 * @brief Time component that handle all the data management and validation
 */

struct Time {
    int y, m, d, h, min;
    
    /**
     * @brief Validates date logic wheter it matches the assignment spec
     * @return true every part of the time component is in the right interval,
     * @return false if one of the part is out of bound
     */
    bool valid() const {
        return (   
            y >= 1900 && y <= 2199           &&
            m >= 1 && m <= 12                &&
            d >= 1 && d <= monthToDays(y, m) &&
            h >= 0 && h <= 23                &&
            min >= 0 && min <= 59
        );  
    }
    /**
     * @brief Convert Gregorian days into total minutes elapse from 1. 1. 1900
     * @details Using IIFE to dynamically calculate and make a const static lookup table of cumulative day counts
     * @return uint64_t Total elapse minutes from 1.1.1900 
     */
    uint64_t getScalarMins() const {
        /**
         * @brief Immediately Invoked Function Expression - IIFE
         * @note Generate a static lookup table (initialise exactly once and live for the rest of the program)
         * @verbatim 
         * ```
         *             1900  1901   1902   1903    ...  2026
         *  arr[300] = [0]   [365]  [730]  [1095]  ...  [46021]
         *             [0]   [1]    [2]    [3]     ...  [126]
         * 
         * How the table is built (runs only once at startup)
         * a[0] = 0                 // From 1.1.1900 -> 0 days elapse; 
         * a[1] = 0 + 365 = 365     // 1.1.1901;
         * a[2] = 365 + 365 = 730   // 1.1.1901;
         * ```
         * @endverbatim
         * @return array of total days of each years from 1.1.1900
         */
        static const auto yearDays = []() {
            std::vector<uint32_t> arr(300, 0);
            for (size_t i = 1; i < arr.size(); ++i)
            {
                int prevYear = 1900 + i - 1;  // Previous year
                int prevTotal = isLeap(prevYear) ? 366 : 365; // Total days in previos year
                arr[i] = arr[i - 1] + prevTotal;
            }
            return arr;
        }();

        uint64_t totalDays = yearDays[y - 1900];
        for (int month = 1; month < m; ++month)
        {
            totalDays += monthToDays(y, month);
        }

        totalDays += (d-1);

        uint64_t minuteFromDays  = totalDays * 24 * 60;
        uint64_t minuteFromHours = h * 60;
        
        return minuteFromDays + minuteFromHours + min;
    }
};

/**
 * @struct Visit
 * @brief  Represents the most important details about one's visit trip to the base.
 */
struct Visit {
    uint64_t entryTime;    ///< Entry time in total minutes since 1990
    uint64_t exitTime;   ///< Exit time in total minutes
    ///< Notes: If the log  shows a person entered the base but doesn't leave before file end, make the out variable set to UINT64_MAX. In other word it acts as a placeholder for INFINITY and mark the fact that the person is stll inside the base.
    Zone entryZone;    ///< Name of the gate
    Zone exitZone;   /// < Name of the gate
};

// Hash tables that maps a person's name to a chronological vector of their base visits.
using VisitMap = std::unordered_map<Name, std::vector<Visit>>;

/**
 * @struct RawLog
 * @brief Capture isolated, individual raw event log from the file.
 */
struct RawLog {
    uint64_t time; ///> Instead of storing the year, month, day and time seperately, the parser immediately converts the timestamp into total minutes elapsed from 1900. This makes it easier to sort the events chronologically later. 
    Zone zone;     ///> Zone name
    Name name;     ///> Visitor's name 
};

// (=^･^=)ฅ^•ﻌ•^ฅ(=^･^=)
//    CORE ULTILITY
// (=^･^=)ฅ^•ﻌ•^ฅ(=^･^=)

/**
 * @class LogStreamReader
 * @brief Worker class exclusively responsible for file parsing operations.
 */
class LogStreamReader {
private:
    /**
     * @brief Guaranteed-length string extraction avoiding null-termination buffer risks.
     */
    std::string readString (std::ifstream& f, size_t len) const {
        if (len == 0) throw std::runtime_error("Zero-length string detected");
        std::string s(len, '\0');
        if(!f.read(s.data(), len)) throw std::runtime_error("EOF reading string");
        return s;
    }

    /**
     * @brief Read the integer from files
     * @note const means this function won't modify the class it belongs to
     * @param fileStream open file stream
     * @param isLittleEdian true if the file is Little-Edian, false if it is Big-Edian
     * @return T template as a placehoder for (uint16, uint32, uint64)
     */
    template <typename T>
    T readInt (std::ifstream& fileStream, bool isLittleEdian) const {
        const size_t size = sizeof(T);
        // Create an accumulator variable res, in order to build our final integer inside this var bit by bit
        T res = 0;
        // Create an array to hold raw bytes.
        // If T is 32-bit integer, sizeof T is 4, so this creates an array of 4 bytes
        uint8_t bytes[size];
        // This read exactly sizeof(T) bytes from the file into the array b.
        // Our array is unsigned bytes `uint8_t`
        // The reinterpreter_cast tell the compiler: "Look at the memory address of b and pretend it is a char* just for this one. I know these two types `char`and `uint8_t` are completely unrelated, but I promised you they takes 1 byte, so stop complaining and pretend it char array for this one time only"
        if(!fileStream.read(reinterpret_cast<char*>(bytes), size)) throw std::runtime_error("EOF reading int");
        
        // Bit shifting technique based on the Endianess
        int bitShift = isLittleEdian ? 0 : (size - 1) * 8;
        int stepShift = isLittleEdian ? 8 : -8;
        for (size_t i = 0; i < size; i++)
        {
            T shiftByte = static_cast<T>(bytes[i]);
            res |= (shiftByte << bitShift);
            bitShift += stepShift;
        }

        return res;
        
    }
    
    /**
     * @brief Parses a SINGLE text line into a raw log object.
     */
    RawLog parseTEXTLine(const std::string& line, const Zone& zone) const {
        std::istringstream iss(line);
        int y,m,d,h,min;
        char dash1, dash2, colon;
        std::string personName;

        if (!(iss >> y >> dash1 >> m >> dash2 >> d >> h >> colon >> min) 
           || dash1 != '-' || dash2 != '-' || colon != ':' ||
            !(std::getline(iss >> std::ws, personName))) 
        {
            throw std::runtime_error("Invalid text format");
        }

         if (!personName.empty() && personName.back() == '\r') {
            personName.pop_back();
        }

        Time timestamp {y, m, d, h, min};
        if(!timestamp.valid()) throw std::runtime_error("Invalid date");

        return {
            timestamp.getScalarMins(),
            zone, 
            personName
        };
    }

    /** @brief Handles parsing of TEXT formatted chunks. */
    void parseTEXTChunk(std::ifstream& file, std::vector<RawLog>& logs) const {
        std::string headerLine;
        // Read the rest of the line immediately after the "TEXT" signature
        if (!std::getline(file, headerLine)) throw std::runtime_error("Incomplete text header");
        if (!headerLine.empty() && !std::isspace(static_cast<unsigned char>(headerLine[0]))) {
            throw std::runtime_error("Missing space after TEXT signature");
        }

        std::istringstream iss(headerLine);
        Zone zone;
        int count;

        // 1. Check if we have the zone and the count
        if (!(iss >> zone >> count)) throw std::runtime_error("Bad text header format");

        // 2. Check for garbage at the end of the header
        std::string extraGarbage;
        if (iss >> extraGarbage) throw std::runtime_error("Garbage at the end of text header");

        // 3. Count cannot be negative
        if (count < 0) throw std::runtime_error("Negative log count");

        for (int i = 0; i < count; ++i) {
            std::string line;
            if (!std::getline(file, line)) throw std::runtime_error("Incomplete log line");
            logs.push_back(parseTEXTLine(line, zone));
        }
    }
    /** @brief Handles parsing of IIII or MMMM (endianess) binary chunks. */
    void parseBINARYChunk(std::ifstream& file, bool isLE, std::vector<RawLog>& logs) const {
        // `readInt` grabs 2 byte (16 bits) to count the legnth of zone name (napr. 9 chars).
        // `readString` grabs 9 bytes from the `readInt` and save it as the string zone
        Zone zone = readString(file, readInt<uint16_t>(file,isLE));
        // `readInt` grabs exactly 4 bytes to find out how many log entries are in this chunk
        uint32_t count = readInt<uint32_t>(file, isLE);
        for (uint32_t i = 0; i < count; ++i) {
            // Example: April 12, 2026, at 14:30 (2:30 PM) - 2124702622
            // Binary : `Year`011111101010 `Mon`0100 `Day`01100 `Hour`01110 `Min`011110                    
            uint32_t dateTime = readInt<uint32_t>(file, isLE);
            
            Time timeStamp {
                // Extract the `year` 011111101010 from the date time binary string
                static_cast<int> ((dateTime >> 20) & 0xFFF),
                // Extract the `month` 0100 from the date time binary string
                static_cast<int> ((dateTime >> 16) & 0x0F),
                // Extract the `day` 01100 from the date time binary string
                static_cast<int> ((dateTime >> 11) & 0x1F),
                // Extract the `hour` 01110 from the date time binary string
                static_cast<int> ((dateTime >>  6) & 0x1F),
                // Extract the `second` 011110 from the date time binary string
                static_cast<int> ((dateTime      ) & 0x3F)
            };

            if(!timeStamp.valid()) throw std::runtime_error("Invalid date");
            Name personName = readString(file, readInt<uint16_t>(file, isLE));
            logs.push_back({
                timeStamp.getScalarMins(),
                zone,
                personName
            });
        }
    }

public: 
    /**
     * @brief Reads a log file and multiplexes based on chunk signatures.
     * @return A flat vector of independent raw log events.
     */
    std::vector<RawLog> readAll(const std::string& filename) const {
        std::ifstream fileStream(filename, std::ios::binary);
        if (!fileStream.is_open()) throw std::runtime_error("Could not open log file");

        std::vector<RawLog> extractedLogs;
        while (true) {
            char magicSignature[4];
            fileStream.read(magicSignature, 4);

            // If we failed to read 4 bytes...
            if (!fileStream) {
                // Check HOW MANY bytes we actually read.
                if (fileStream.gcount() == 0) {
                    break; // We read 0 bytes. This is a perfect, clean End-Of-File.
                } else {
                    // We read 1, 2, or 3 bytes. This is trailing garbage! (Like the 'Z')
                    throw std::runtime_error("Trailing garbage or incomplete chunk signature");
                }
            }

            if (std::strncmp(magicSignature, "TEXT", 4) == 0) {
                parseTEXTChunk(fileStream, extractedLogs);
            } 
            else if (std::strncmp(magicSignature, "IIII", 4) == 0 || std::strncmp(magicSignature, "MMMM", 4) == 0) {
                parseBINARYChunk(fileStream, magicSignature[0] == 'I', extractedLogs);
            } 
            else {
                throw std::runtime_error("Unknown format signature detected");
            }
        }

        if (extractedLogs.empty()) {
            throw std::runtime_error("Log file contains no data");
        }
        return extractedLogs;
    }
};

/**
 * @class BaseTopology
 * @brief Adjacency list designed specifically to be queried by Dijkstra`s algo
 * 
 */
class BaseTopology {
    private:
        Graph graf; ///< Adjacency list - Example: Zone A : [[{Zone B, 5 mins}, {Zone C, 10 mins}]]
        ///> Mutable - Even if the function is const, allow it to modify this one specific pathCaches variable.
        // Top-down memoization DP approach
        mutable std::unordered_map<Zone, DistMap> pathCache; 

    public: 
    // The & means we are passing the Zone by reference (we are looking at the original string, not making a slow copy of it). 
    // const ensures we don't accidentally change the name of the zone.
        void addEdge (const Zone& a, const Zone& b, Weight w) {
            graf[a].push_back({b, w});
            graf[b].push_back({a, w});
        }

        // Dijkstra algorithm
        const DistMap& getPaths (const Zone& start) const {
            //Cache check----
            //Structure binding - try_emplace returns two things: an iterator (iter) pointing to the data, and a boolean (inserted) telling us if this is brand new. Literally destructuring in JS.
            auto [cacheIterator, inserted] = pathCache.try_emplace(start);
            if (inserted) {
                DistMap distance;
                distance[start] = 0;
                if(graf.count(start)) {
                    using pathCandidate = std::pair<Weight, Zone>;
                    
                    // 1. Setting up the storage
                    // exploreQueue - Special queue that automatically sorts itself when adding (Min-heap)
                    std::priority_queue<pathCandidate, std::vector<pathCandidate>, std::greater<pathCandidate>> exploreQueue;
                
                    // 2. Initialise the start
                    exploreQueue.push({0, start});

                    // 3. Exploring the map
                    while (!exploreQueue.empty()) {
                        auto [currTime, currZone] = exploreQueue.top();
                        exploreQueue.pop();
                        // If we already found a faster way to get here, skip it
                        if(currTime > distance[currZone]) continue;
                        // Look at neighbor (next) zones so that we can physially walk to from here
                        for (const auto& [neighborZone, travelTime] : graf.at(currZone)) {
                            int totalTime = currTime + travelTime;      ///> Create the newTime to check

                            
                            if (!distance.count(neighborZone) ||        ///> If we have never visited this neighbor
                                totalTime < distance[neighborZone]) {   ///> If this new route is faster
                                    distance[neighborZone] = totalTime;
                                    exploreQueue.push({totalTime, neighborZone});
                                }
                        }
                    }
                }
                cacheIterator->second = std::move(distance);
            }
            return cacheIterator->second;
        }
};   

// (=^･^=)ฅ^•ﻌ•^ฅ(=^･^=)
//      CORE LOGIC
// (=^･^=)ฅ^•ﻌ•^ฅ(=^･^=)

class CAuditFilter
{
  private:
    Zone _target;
    uint64_t _start;
    uint64_t _end;

  public:

    /**
     * @brief Constructor sets the target zone and initializes default open bounds for start and end time as scalable minute.
     * @param target The target zone string.
     */
    CAuditFilter(Zone target) : _target(std::move(target)), _start(0), _end(UINT64_MAX) {};
    
    /**
     * @brief Sets the lower bound for the visit overlap. Retains the highest bound if chained.
     */
    CAuditFilter& notBefore (int y, int m, int d, int h, int p) {
        uint64_t inputTime = Time{y,m,d,h,p}.getScalarMins();
        _start = std::max(_start, inputTime);
        return *this; 
    }

    /**
     * @brief Sets the upper bound for the visit overlap. Retains the lowest bound if chained.
     */
    CAuditFilter& notAfter (int y, int m, int d, int h, int p) {
        uint64_t inputTime = Time{y,m,d,h,p}.getScalarMins();
        _end = std::min(_end, inputTime);
        return *this; 
    }
    
    // Getter [ _target ]
    const Zone& target() const { return _target; }
    // Getter [ _start ]
    uint64_t start() const { return _start; }
    // Getter [ _end ]
    uint64_t end() const { return _end; }
};

/**
 * @brief Search engine
 * @class CVisitorLog
 */
class CVisitorLog
{
  private:
    std::shared_ptr<const BaseTopology> topology; 
    VisitMap visits;

    /**
     * @brief Validates if a single stay mathematically fits the requested time bounds.
     * @details
     * Step 1: Calculate the earliest possible arrival at the Target Zone (Entry time + walk time).
     * Step 2: Calculate the latest possible departure from Target Zone (Exit time - walk time).
     * Step 3: Verify they had enough time realistically to reach it before leaving the base entirely.
     * Step 4: Check if their physical presence (Arrival -> Departure) overlaps the Filter Window.
     */
    bool checkVisitOverlap (const Visit& visit, const CAuditFilter& filter, const DistMap& shortestPaths) const {
        auto entryPath = shortestPaths.find(visit.entryZone);
        if(entryPath == shortestPaths.end()) return false; // Target is unreachable from entry point
    
        uint64_t arriveTime = visit.entryTime + entryPath->second;
        uint64_t departTime = visit.exitTime;

        if(visit.exitTime != UINT64_MAX) {
            auto exitPath = shortestPaths.find(visit.exitZone);
            
            if(exitPath == shortestPaths.end() || visit.exitTime < static_cast<uint64_t>(exitPath->second)) {
                return false;
            }

            uint64_t transitTime = static_cast<uint64_t>(exitPath->second);
            if(visit.exitTime < transitTime) {return false;} 
            departTime = visit.exitTime - transitTime;
        }

        if(arriveTime > departTime) return false;
        uint64_t overlapStart = std::max(arriveTime, filter.start());
        uint64_t overlapEnd = std::min(departTime, filter.end());
        return overlapStart <= overlapEnd;
    }

  public:
    CVisitorLog(std::shared_ptr<const BaseTopology> topoMap, VisitMap visitorLogs) 
        : topology(std::move(topoMap)), visits(std::move(visitorLogs)) {}
    
    /**
     * @brief Finds which visitors reached the target zone during the allowed time filter.
     */
    
    Results search ( const CAuditFilter& filter ) const
    {
        Results matches;
        const DistMap& shortestPaths = topology->getPaths(filter.target());

        for (const auto& [personName, visitHistory] : visits) {
            auto doesVisitMatch = [&](const Visit& visit) {return checkVisitOverlap(visit, filter, shortestPaths);};
            if(std::any_of(visitHistory.begin(), visitHistory.end(), doesVisitMatch)) {
                matches.insert(personName);
            }
        }
        return matches;
    }
};

class CMilBase
{
  private:
    std::shared_ptr<BaseTopology> topology;
    VisitMap mapVisits (std::vector<RawLog>& rawLogs) const {
        std::unordered_map<Name, std::vector<RawLog>> personLogs;

        for (const auto& log : rawLogs) {
            personLogs[log.name]. push_back(log);
        }

        VisitMap vmap; // Visit map

        for (auto& [visitorName, visitorLogs] : personLogs) {
            std::sort(visitorLogs.begin(), visitorLogs.end(), [](const RawLog& a, const RawLog&b) {
                return a.time < b.time;
            });

            for (size_t in = 0, out = 1;
                in < visitorLogs.size(); 
                in +=2, out += 2)
            {
                uint64_t exitTime;
                Zone exitZone;

                if(out < visitorLogs.size()) {
                    exitTime = visitorLogs[out].time;
                    exitZone = visitorLogs[out].zone;
                } else {
                    exitTime = UINT64_MAX;
                    exitZone = "";
                }

                vmap[visitorName].push_back ({
                    visitorLogs[in].time,
                    exitTime,
                    visitorLogs[in].zone,
                    exitZone
                });
            }
            
        }
        return vmap;
    }
  public:
    CMilBase() : topology(std::make_shared<BaseTopology>()) {}
    void readBase(const std::string& baseFilename) {
        std::ifstream fileStream(baseFilename);
        if (!fileStream.is_open()) throw std::runtime_error("No base file found");

        std::string line;
        while (std::getline(fileStream, line)) {
            // Skip empty lines or lines that only contain spaces/newlines
            auto isSpace = [](unsigned char c) { return std::isspace(c); };
            if (std::all_of(line.begin(), line.end(), isSpace)) continue;

            std::istringstream iss(line);
            Zone zoneA, zoneB;
            Weight traversalTime;

            // 1. Try to read the 3 required parts
            if (!(iss >> zoneA >> zoneB >> traversalTime)) {
                throw std::runtime_error("Incomplete edge definition");
            }

            // 2. Check if there is extra garbage at the end of the line
            std::string extraGarbage;
            if (iss >> extraGarbage) {
                throw std::runtime_error("Trailing garbage on base line");
            }

            // 3. Negative distances physically shouldn't exist
            if (traversalTime < 0) {
                throw std::runtime_error("Negative travel time not allowed");
            }

            topology->addEdge(zoneA, zoneB, traversalTime);
        }
    }
    

    CVisitorLog processLog  ( const std::string & logFilename )
    {
        LogStreamReader reader;
        std::vector<RawLog> rawLogs = reader.readAll(logFilename);
        
        return CVisitorLog(topology, mapVisits(rawLogs));
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