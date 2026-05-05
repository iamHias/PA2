#ifndef __PROGTEST__
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cctype>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>
#include <list>
#include <map>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>
#include <memory>
#include <functional>
#include <stdexcept>
#include <compare>
#include <iterator>
#endif /* __PROGTEST */

#include <unordered_map> 

#ifndef __PROGTEST__

// MOCK IMPLEMENTATION FOR LOCAL TESTING
class CTimeStamp
{
  public:
    CTimeStamp(int year, int month, int day, int hour, int minute, int sec)
        : y(year), m(month), d(day), h(hour), min(minute), s(sec) {}

    std::strong_ordering operator<=>(const CTimeStamp& x) const {
        if (auto cmp = y <=> x.y; cmp != 0) return cmp;
        if (auto cmp = m <=> x.m; cmp != 0) return cmp;
        if (auto cmp = d <=> x.d; cmp != 0) return cmp;
        if (auto cmp = h <=> x.h; cmp != 0) return cmp;
        if (auto cmp = min <=> x.min; cmp != 0) return cmp;
        return s <=> x.s;
    }

    bool operator==(const CTimeStamp& x) const {
        return (*this <=> x) == 0;
    }

    friend std::ostream& operator<<(std::ostream& os, const CTimeStamp& x) {
        os << std::setfill('0') << std::setw(4) << x.y << "-"
           << std::setw(2) << x.m << "-"
           << std::setw(2) << x.d << " "
           << std::setw(2) << x.h << ":"
           << std::setw(2) << x.min << ":"
           << std::setw(2) << x.s;
        return os;
    }

  private:
    int y, m, d, h, min, s;
};

// MOCK IMPLEMENTATION FOR LOCAL TESTING
class CMailBody
{
  public:
    CMailBody(int size, const char data[]) : m_Size(size) {
        m_Data = new char[size + 1];
        std::memcpy(m_Data, data, size);
        m_Data[size] = '\0';
    }
    
    // Rule of 3 added for local memory safety
    CMailBody(const CMailBody& other) : m_Size(other.m_Size) {
        m_Data = new char[m_Size + 1];
        std::memcpy(m_Data, other.m_Data, m_Size + 1);
    }
    
    CMailBody& operator=(const CMailBody& other) {
        if (this != &other) {
            delete[] m_Data;
            m_Size = other.m_Size;
            m_Data = new char[m_Size + 1];
            std::memcpy(m_Data, other.m_Data, m_Size + 1);
        }
        return *this;
    }
    
    ~CMailBody() { delete[] m_Data; }

    friend std::ostream & operator << ( std::ostream & os, const CMailBody & x ) {
      return os << "mail body: " << x . m_Size << " B";
    }
  private:
    int            m_Size;
    char         * m_Data;
};

class CAttach
{
  public:
                   CAttach                                 ( int               x )
      : m_X (x)
    {
    }
    void           addRef                                  ()
    { 
      m_RefCnt ++; 
    }
    void           release                                 ()
    { 
      if ( !--m_RefCnt ) 
        delete this; 
    }
  private:
    int            m_X;
    int            m_RefCnt = 1;
                   CAttach                                 ( const CAttach   & x );
    CAttach      & operator =                              ( const CAttach   & x );
                   ~CAttach                                () = default;
    friend std::ostream & operator <<                      ( std::ostream    & os,
                                                             const CAttach   & x )
    {
      return os << "attachment: " << x . m_X << " B";
    }
};
#endif /* __PROGTEST__, DO NOT remove */

class CMail
{
  public:
    CMail ( const CTimeStamp & timeStamp, const std::string& from, const CMailBody  & body, CAttach * attach ) 
        : m_Arrival(timeStamp), m_Sender(from), m_Content(body), m_Enclosure(attach) {
      if(m_Enclosure) m_Enclosure->addRef();
    }

    CMail ( const CMail& other ) 
        : m_Arrival(other.m_Arrival), m_Sender(other.m_Sender), m_Content(other.m_Content), m_Enclosure(other.m_Enclosure) {
      if (m_Enclosure) m_Enclosure->addRef();
    }

    CMail ( CMail && other ) noexcept 
        : m_Arrival(std::move(other.m_Arrival)), m_Sender(std::move(other.m_Sender)), m_Content(std::move(other.m_Content)), m_Enclosure(other.m_Enclosure) {
      other.m_Enclosure = nullptr;
    }
    
    CMail & operator= (const CMail & other) {
      if (this == &other) return *this;
      if (m_Enclosure) m_Enclosure->release();

      m_Enclosure = other.m_Enclosure;
      m_Content = other.m_Content;
      m_Sender = other.m_Sender;
      m_Arrival = other.m_Arrival;

      if (m_Enclosure) m_Enclosure->addRef();
      return *this;
    }
    
    CMail & operator= (CMail && other) noexcept {
      if (this == &other ) return *this;
      if (m_Enclosure) m_Enclosure->release();

      m_Arrival = std::move(other.m_Arrival);
      m_Sender = std::move(other.m_Sender);
      m_Content = std::move(other.m_Content);
      m_Enclosure = other.m_Enclosure;

      other.m_Enclosure = nullptr;
      return *this;
    }

    ~CMail() { if (m_Enclosure) m_Enclosure->release(); }
    
    const std::string   & from                             () const { return m_Sender; }
    const CMailBody     & body                             () const { return m_Content; }
    const CTimeStamp    & timeStamp                        () const { return m_Arrival; }
    
    CAttach             * attachment                       () const {
      if(m_Enclosure) m_Enclosure->addRef();
      return m_Enclosure;
    }

    friend std::ostream & operator <<                      ( std::ostream     & os,
                                                             const CMail      & x ) {
        os << x.m_Arrival << " " << x.m_Sender << " " << x.m_Content;
        return x.m_Enclosure ? (os << " + " << *x.m_Enclosure) : os;
    }

  private:
    CTimeStamp  m_Arrival;
    std::string m_Sender;
    CMailBody   m_Content;
    CAttach    *m_Enclosure;
};

struct ChronoCompareStrategy {
  bool operator() (const CMail& msg, const CTimeStamp& stamp) const { return msg.timeStamp() < stamp; }
  bool operator() (const CTimeStamp& stamp, const CMail& msg) const { return stamp < msg.timeStamp(); }
  bool operator() (const CMail& first, const CMail& second)   const { return first.timeStamp() < second.timeStamp(); }
};

class MailDirectory {
  public:
    using MsgIterator = std::vector<CMail>::const_iterator;
    
    void insertMessage (const CMail& incomingMsg) {
      auto pos = std::lower_bound(m_MessageList.begin(), m_MessageList.end(), incomingMsg, ChronoCompareStrategy());
      m_MessageList.insert(pos, incomingMsg);
    }

    void absorbContents (MailDirectory& sourceDir) {
      if(sourceDir.m_MessageList.empty()) return;
      
      if(m_MessageList.empty()) {
        m_MessageList = std::move(sourceDir.m_MessageList);
      } else {
        std::vector<CMail> combined;
        combined.reserve(m_MessageList.size() + sourceDir.m_MessageList.size());
        std::merge(std::make_move_iterator(m_MessageList.begin()), std::make_move_iterator(m_MessageList.end()),
                   std::make_move_iterator(sourceDir.m_MessageList.begin()), std::make_move_iterator(sourceDir.m_MessageList.end()),
                   std::back_inserter(combined), ChronoCompareStrategy());
        m_MessageList = std::move(combined);
      }
      sourceDir.m_MessageList.clear();
    }

    std::pair<MsgIterator, MsgIterator> fetchWindow (const CTimeStamp& startLimit, const CTimeStamp& endLimit) const {
      return std::make_pair(
            std::lower_bound(m_MessageList.begin(), m_MessageList.end(), startLimit, ChronoCompareStrategy()),
            std::upper_bound(m_MessageList.begin(), m_MessageList.end(), endLimit, ChronoCompareStrategy())
      );
    }
  
  private:
    std::vector<CMail> m_MessageList;
};

class CMailBox
{
  public:
    CMailBox ()
    {
        m_Mailboxes.emplace("inbox", MailDirectory());
    }

    bool delivery ( const CMail & mail )
    {
        m_Mailboxes["inbox"].insertMessage(mail);
        return true;
    }

    bool newFolder ( const std::string& folderName )
    {
        return m_Mailboxes.emplace(folderName, MailDirectory()).second;
    }

    bool moveMail ( const std::string& fromFolder, const std::string& toFolder )
    {
        if (fromFolder == toFolder) return true;

        auto srcIter = m_Mailboxes.find(fromFolder);
        auto dstIter = m_Mailboxes.find(toFolder);
        if (srcIter == m_Mailboxes.end() || dstIter == m_Mailboxes.end()) return false;
        
        dstIter->second.absorbContents(srcIter->second);
        return true;
    }

    std::list<CMail> listMail ( const std::string& folderName,
                                const CTimeStamp & from,
                                const CTimeStamp & to ) const
    {
        auto boxIter = m_Mailboxes.find(folderName);
        if (boxIter == m_Mailboxes.end()) return {};

        auto window = boxIter->second.fetchWindow(from, to);
        return std::list<CMail>(window.first, window.second);
    }

    std::set<std::string> listAddr ( const CTimeStamp & from,
                                     const CTimeStamp & to ) const
    {
        std::set<std::string> uniqueSenders;
        for (const auto & boxPair : m_Mailboxes) {
            auto window = boxPair.second.fetchWindow(from, to);
            std::transform(window.first, window.second, std::inserter(uniqueSenders, uniqueSenders.end()), 
                           [](const CMail & m) { return m.from(); });
        }
        return uniqueSenders;
    }

  private:
    std::unordered_map<std::string, MailDirectory> m_Mailboxes;
};

#ifndef __PROGTEST__
static std::string showMail ( const std::list<CMail> & l )
{
  std::ostringstream oss;
  for ( const auto & x : l )
    oss << x << std::endl;
  return oss . str ();
}

static std::string showUsers ( const std::set<std::string> & s )
{
  std::ostringstream oss;
  for ( const auto & x : s )
    oss << x << std::endl;
  return oss . str ();
}

int main ()
{
  CAttach * att;
  std::ostringstream oss;

  att = new CAttach ( 100 );
  CMail testMail ( CTimeStamp ( 2026, 1, 2, 12, 5, 0 ), "test@domain.cz", CMailBody ( 10, "test, test" ), att );
  att -> release ();
  assert ( testMail . timeStamp () == CTimeStamp ( 2026, 1, 2, 12, 5, 0 ) );
  assert ( testMail . from () == "test@domain.cz" );
  att = testMail . attachment ();
  oss << *att;
  att -> release ();
  assert ( oss . str () == "attachment: 100 B" );
  assert ( showMail ( { testMail } ) == "2026-01-02 12:05:00 test@domain.cz mail body: 10 B + attachment: 100 B\n" );

  CMailBox m0;
  assert ( m0 . delivery ( CMail ( CTimeStamp ( 2024, 3, 31, 15, 24, 13 ), "user1@fit.cvut.cz", CMailBody ( 14, "mail content 1" ), nullptr ) ) );
  assert ( m0 . delivery ( CMail ( CTimeStamp ( 2024, 3, 31, 15, 26, 23 ), "user2@fit.cvut.cz", CMailBody ( 22, "some different content" ), nullptr ) ) );
  att = new CAttach ( 200 );
  assert ( m0 . delivery ( CMail ( CTimeStamp ( 2024, 3, 31, 11, 23, 43 ), "boss1@fit.cvut.cz", CMailBody ( 14, "urgent message" ), att ) ) );
  assert ( m0 . delivery ( CMail ( CTimeStamp ( 2024, 3, 31, 18, 52, 27 ), "user1@fit.cvut.cz", CMailBody ( 14, "mail content 2" ), att ) ) );
  att -> release ();
  att = new CAttach ( 97 );
  assert ( m0 . delivery ( CMail ( CTimeStamp ( 2024, 3, 31, 16, 12, 48 ), "boss1@fit.cvut.cz", CMailBody ( 24, "even more urgent message" ), att ) ) );
  att -> release ();
  assert ( showMail ( m0 . listMail ( "inbox",
                      CTimeStamp ( 2000, 1, 1, 0, 0, 0 ),
                      CTimeStamp ( 2050, 12, 31, 23, 59, 59 ) ) ) == "2024-03-31 11:23:43 boss1@fit.cvut.cz mail body: 14 B + attachment: 200 B\n"
                        "2024-03-31 15:24:13 user1@fit.cvut.cz mail body: 14 B\n"
                        "2024-03-31 15:26:23 user2@fit.cvut.cz mail body: 22 B\n"
                        "2024-03-31 16:12:48 boss1@fit.cvut.cz mail body: 24 B + attachment: 97 B\n"
                        "2024-03-31 18:52:27 user1@fit.cvut.cz mail body: 14 B + attachment: 200 B\n" );
  assert ( showMail ( m0 . listMail ( "inbox",
                      CTimeStamp ( 2024, 3, 31, 15, 26, 23 ),
                      CTimeStamp ( 2024, 3, 31, 16, 12, 48 ) ) ) == "2024-03-31 15:26:23 user2@fit.cvut.cz mail body: 22 B\n"
                        "2024-03-31 16:12:48 boss1@fit.cvut.cz mail body: 24 B + attachment: 97 B\n" );
  assert ( showUsers ( m0 . listAddr ( CTimeStamp ( 2000, 1, 1, 0, 0, 0 ),
                       CTimeStamp ( 2050, 12, 31, 23, 59, 59 ) ) ) == "boss1@fit.cvut.cz\n"
                        "user1@fit.cvut.cz\n"
                        "user2@fit.cvut.cz\n" );
  assert ( showUsers ( m0 . listAddr ( CTimeStamp ( 2024, 3, 31, 15, 26, 23 ),
                       CTimeStamp ( 2024, 3, 31, 16, 12, 48 ) ) ) == "boss1@fit.cvut.cz\n"
                        "user2@fit.cvut.cz\n" );

  CMailBox m1;
  assert ( m1 . newFolder ( "work" ) );
  assert ( m1 . newFolder ( "spam" ) );
  assert ( !m1 . newFolder ( "spam" ) );
  assert ( m1 . delivery ( CMail ( CTimeStamp ( 2024, 3, 31, 15, 24, 13 ), "user1@fit.cvut.cz", CMailBody ( 14, "mail content 1" ), nullptr ) ) );
  att = new CAttach ( 500 );
  assert ( m1 . delivery ( CMail ( CTimeStamp ( 2024, 3, 31, 15, 26, 23 ), "user2@fit.cvut.cz", CMailBody ( 22, "some different content" ), att ) ) );
  att -> release ();
  assert ( m1 . delivery ( CMail ( CTimeStamp ( 2024, 3, 31, 11, 23, 43 ), "boss1@fit.cvut.cz", CMailBody ( 14, "urgent message" ), nullptr ) ) );
  att = new CAttach ( 468 );
  assert ( m1 . delivery ( CMail ( CTimeStamp ( 2024, 3, 31, 18, 52, 27 ), "user1@fit.cvut.cz", CMailBody ( 14, "mail content 2" ), att ) ) );
  att -> release ();
  assert ( m1 . delivery ( CMail ( CTimeStamp ( 2024, 3, 31, 16, 12, 48 ), "boss1@fit.cvut.cz", CMailBody ( 24, "even more urgent message" ), nullptr ) ) );
  assert ( showMail ( m1 . listMail ( "inbox",
                      CTimeStamp ( 2000, 1, 1, 0, 0, 0 ),
                      CTimeStamp ( 2050, 12, 31, 23, 59, 59 ) ) ) == "2024-03-31 11:23:43 boss1@fit.cvut.cz mail body: 14 B\n"
                        "2024-03-31 15:24:13 user1@fit.cvut.cz mail body: 14 B\n"
                        "2024-03-31 15:26:23 user2@fit.cvut.cz mail body: 22 B + attachment: 500 B\n"
                        "2024-03-31 16:12:48 boss1@fit.cvut.cz mail body: 24 B\n"
                        "2024-03-31 18:52:27 user1@fit.cvut.cz mail body: 14 B + attachment: 468 B\n" );
  assert ( showMail ( m1 . listMail ( "work",
                      CTimeStamp ( 2000, 1, 1, 0, 0, 0 ),
                      CTimeStamp ( 2050, 12, 31, 23, 59, 59 ) ) ) == "" );
  assert ( m1 . moveMail ( "inbox", "work" ) );
  assert ( showMail ( m1 . listMail ( "inbox",
                      CTimeStamp ( 2000, 1, 1, 0, 0, 0 ),
                      CTimeStamp ( 2050, 12, 31, 23, 59, 59 ) ) ) == "" );
  assert ( showMail ( m1 . listMail ( "work",
                      CTimeStamp ( 2000, 1, 1, 0, 0, 0 ),
                      CTimeStamp ( 2050, 12, 31, 23, 59, 59 ) ) ) == "2024-03-31 11:23:43 boss1@fit.cvut.cz mail body: 14 B\n"
                        "2024-03-31 15:24:13 user1@fit.cvut.cz mail body: 14 B\n"
                        "2024-03-31 15:26:23 user2@fit.cvut.cz mail body: 22 B + attachment: 500 B\n"
                        "2024-03-31 16:12:48 boss1@fit.cvut.cz mail body: 24 B\n"
                        "2024-03-31 18:52:27 user1@fit.cvut.cz mail body: 14 B + attachment: 468 B\n" );
  assert ( m1 . delivery ( CMail ( CTimeStamp ( 2024, 3, 31, 19, 24, 13 ), "user2@fit.cvut.cz", CMailBody ( 14, "mail content 4" ), nullptr ) ) );
  att = new CAttach ( 234 );
  assert ( m1 . delivery ( CMail ( CTimeStamp ( 2024, 3, 31, 13, 26, 23 ), "user3@fit.cvut.cz", CMailBody ( 9, "complains" ), att ) ) );
  att -> release ();
  assert ( showMail ( m1 . listMail ( "inbox",
                      CTimeStamp ( 2000, 1, 1, 0, 0, 0 ),
                      CTimeStamp ( 2050, 12, 31, 23, 59, 59 ) ) ) == "2024-03-31 13:26:23 user3@fit.cvut.cz mail body: 9 B + attachment: 234 B\n"
                        "2024-03-31 19:24:13 user2@fit.cvut.cz mail body: 14 B\n" );
  assert ( showMail ( m1 . listMail ( "work",
                      CTimeStamp ( 2000, 1, 1, 0, 0, 0 ),
                      CTimeStamp ( 2050, 12, 31, 23, 59, 59 ) ) ) == "2024-03-31 11:23:43 boss1@fit.cvut.cz mail body: 14 B\n"
                        "2024-03-31 15:24:13 user1@fit.cvut.cz mail body: 14 B\n"
                        "2024-03-31 15:26:23 user2@fit.cvut.cz mail body: 22 B + attachment: 500 B\n"
                        "2024-03-31 16:12:48 boss1@fit.cvut.cz mail body: 24 B\n"
                        "2024-03-31 18:52:27 user1@fit.cvut.cz mail body: 14 B + attachment: 468 B\n" );
  assert ( m1 . moveMail ( "inbox", "work" ) );
  assert ( showMail ( m1 . listMail ( "inbox",
                      CTimeStamp ( 2000, 1, 1, 0, 0, 0 ),
                      CTimeStamp ( 2050, 12, 31, 23, 59, 59 ) ) ) == "" );
  assert ( showMail ( m1 . listMail ( "work",
                      CTimeStamp ( 2000, 1, 1, 0, 0, 0 ),
                      CTimeStamp ( 2050, 12, 31, 23, 59, 59 ) ) ) == "2024-03-31 11:23:43 boss1@fit.cvut.cz mail body: 14 B\n"
                        "2024-03-31 13:26:23 user3@fit.cvut.cz mail body: 9 B + attachment: 234 B\n"
                        "2024-03-31 15:24:13 user1@fit.cvut.cz mail body: 14 B\n"
                        "2024-03-31 15:26:23 user2@fit.cvut.cz mail body: 22 B + attachment: 500 B\n"
                        "2024-03-31 16:12:48 boss1@fit.cvut.cz mail body: 24 B\n"
                        "2024-03-31 18:52:27 user1@fit.cvut.cz mail body: 14 B + attachment: 468 B\n"
                        "2024-03-31 19:24:13 user2@fit.cvut.cz mail body: 14 B\n" );

  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */