#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cctype>
#include <cmath>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <memory>
#include <functional>
#include <stdexcept>
#include <compare>
#include "ipaddress.h"
using namespace std::literals;
#endif /* __PROGTEST__ */

// (=^･^=)ฅ^•ﻌ•^ฅ(=^･^=)
//  ABSTRACT BASE CLASS - CRECORD
// (=^･^=)ฅ^•ﻌ•^ฅ(=^･^=)

class CRecord {
  protected: 
    std::string _name;
  public:
    CRecord (const std::string& name) : _name(name) {}

    virtual ~CRecord() = default;

    virtual CRecord* clone() const = 0;
    virtual std::string type() const = 0;
    virtual bool isEqual (const CRecord& other) const = 0;
    virtual void print (std::ostream& os) const = 0;
    std::string name() const { return _name; } //> getter

    friend std::ostream& operator << (std::ostream& os, const CRecord& record) {
      record.print(os);
      return os;
    }
} ;

// (=^･^=)ฅ^•ﻌ•^ฅ(=^･^=)
//  INHERITED RECORD CLASS
// (=^･^=)ฅ^•ﻌ•^ฅ(=^･^=)

class CRecA : public CRecord {
  CIPv4 _IP;
  public:
    // constructor
      CRecA(const std::string& name, const CIPv4& ip) : CRecord(name), _IP(ip) {};
    // name ()
      CRecord* clone() const override {return new CRecA(*this);};
    // type ()
      std::string type() const override {return "A";}
    // operator <<
      void print (std::ostream& os ) const override {
        os << _name << " A " << _IP;
      }
    // todo
      bool isEqual (const CRecord& other) const override {
        if(type() != other.type() || name() != other.name()) return false;
        const CRecA& otherA = dynamic_cast<const CRecA&> (other);
        return _IP == otherA._IP;
      }
};

class CRecAAAA : public CRecord {
  CIPv6 _IP;
  public:
    // constructor
      CRecAAAA(const std::string& name, const CIPv6& ip) : CRecord(name), _IP(ip) {};
    // name ()
      CRecord* clone() const override {return new CRecAAAA(*this);};
    // type ()
      std::string type() const override {return "AAAA";}
    // operator <<
      void print (std::ostream& os ) const override {
        os << _name << " AAAA " << _IP;
      }
    // todo
      bool isEqual (const CRecord& other) const override {
        if(type() != other.type() || name() != other.name()) return false;
        const CRecAAAA& otherAAAA = dynamic_cast<const CRecAAAA&> (other);
        return _IP == otherAAAA._IP;
      }
};

class CRecMX : public CRecord {
  std::string _mailServer;
  int _priority;
  public:
    // constructor
      CRecMX(const std::string& name,   const std::string& mailServer, int priority) : 
                        CRecord(name), _mailServer(mailServer), _priority(priority) {};
    // name ()
      CRecord* clone() const override {return new CRecMX(*this);};
    // type ()
      std::string type() const override {return "MX";}
    // operator <<
      void print (std::ostream& os ) const override {
        os << _name << " MX " << _priority << " " << _mailServer;
      }
    // todo
      bool isEqual (const CRecord& other) const override {
        if(type() != other.type() || name() != other.name()) return false;
        const CRecMX& otherMX = dynamic_cast<const CRecMX&> (other);
        return _mailServer == otherMX._mailServer &&
               _priority   == otherMX._priority;
      }
};


class CZoneSearch {
  std::vector <const CRecord*> _matches;
  public:
    size_t size() const { return _matches.size(); }
    void add (const CRecord* r) {_matches.push_back(r);}
    const CRecord& operator[] (size_t idx) const {
      if(idx >= _matches.size()) throw std::out_of_range(std::to_string(idx));
      return *_matches[idx];
    }
    friend std::ostream& operator << (std::ostream& os, const CZoneSearch& zs) {
      for (const auto* record: zs._matches) os << *record << "\n";
      return os; 
    }
};

// Rule of five
class CZone
{
  std::string _name;
  std::vector<std::unique_ptr<CRecord>> _records;
  std::unordered_map<std::string, std::vector<CRecord*>> _index;
  public:
    // constructor(s)
    CZone(const std::string& name): _name(name) {};
    CZone(const CZone& other) : _name(other._name) {
      _records.reserve(other._records.size());
      for (const auto &rec : other._records) {
        _records.emplace_back(rec->clone());
        _index[rec->name()].push_back(_records.back().get());
      }
    }
    // operator = (if needed)
    CZone& operator= (const CZone& other) {
      if(this == &other) return *this;
      _name = other._name;
      _records.clear();
      _index.clear();
      // By default, when a vector runs out of capacity, it reallocates (typically doubles) — copying all elements to a new buffer. 
      // Because I know the approximate size upfront, reserve() eliminates these (doubly) costly reallocations.
      _records.reserve(other._records.size()) ;
      // reserve() never shrinks the vector. Use shrink_to_fit() for that
      for (const auto &rec : other._records){
        //emplace_back: constructs directly inside the vector — no temporary!
        _records.emplace_back(rec->clone());
        _index[rec->name()].push_back(_records.back().get());
      }
      return *this;
    };
    // 3, 4, 5. Default Destructor and Move Semantics
    ~CZone() = default;
    CZone& operator=(CZone&& other) noexcept = default;
    
    // add ()
    bool add(const CRecord& record){
      auto it = _index.find(record.name());
      if(it != _index.end()) {
        for (const CRecord* exist: it->second) {
          if(exist->isEqual(record)) return false;
        }
      }

      _records.emplace_back(record.clone());
      _index[record.name()].push_back(_records.back().get());
      return true;
    }
    // del ()
    bool del (const CRecord& rec) {
      auto indexIt = _index.find(rec.name());
      if (indexIt == _index.end()) return false;
      
      auto& ptrList = indexIt->second;
      auto  listIt  = std::find_if(ptrList.begin(), ptrList.end(), 
            [&rec](const CRecord* ptr) {return ptr->isEqual(rec);}
      );
      if (listIt == ptrList.end()) return false;

      CRecord* targetPtr = *listIt;
      _records.erase(std::find_if(_records.begin(), _records.end(), 
        [targetPtr](const std::unique_ptr<CRecord>& u) { return u.get() == targetPtr; }));
      ptrList.erase(listIt);
        return true;
      }
      
    // search ()
    CZoneSearch search(const std::string& name) const {
      CZoneSearch result;
      auto it = _index.find(name);
      if (it != _index.end()) {
        for (const CRecord* ptr : it->second) {
          result.add(ptr);
        }
      }
      return result;
    }

    // operator <<
    friend std::ostream& operator<<(std::ostream& os, const CZone& zone) {
      os << zone._name << "\n";
      
      if (zone._records.empty()) { return os; }

      size_t last_idx = zone._records.size() - 1; 
      for (size_t i = 0; i <  last_idx; ++i) {
        os << " +- " << *zone._records[i] << "\n";
      }

      os << " \\- " << *zone._records[last_idx] << "\n";
      return os;
    }
};

#ifndef __PROGTEST__
int main ()
{
  std::ostringstream oss;

  CZone z0 ( "fit.cvut.cz" );
  assert ( z0 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  assert ( z0 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:0:1:2:3" ) ) ) == true );
  assert ( z0 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.158" ) ) ) == true );
  assert ( z0 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.160" ) ) ) == true );
  assert ( z0 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.159" ) ) ) == true );
  assert ( z0 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:1:2:3:4" ) ) ) == true );
  assert ( z0 . add ( CRecMX ( "courses", "relay.fit.cvut.cz.", 0 ) ) == true );
  assert ( z0 . add ( CRecMX ( "courses", "relay2.fit.cvut.cz.", 10 ) ) == true );
  oss . str ( "" );
  oss << z0;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- progtest A 147.32.232.142\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " +- courses A 147.32.232.159\n"
    " +- progtest AAAA 2001:718:2:2902:1:2:3:4\n"
    " +- courses MX 0 relay.fit.cvut.cz.\n"
    " \\- courses MX 10 relay2.fit.cvut.cz.\n" );
  assert ( z0 . search ( "progtest" ) . size () == 3 );
  oss . str ( "" );
  oss << z0 . search ( "progtest" );
  assert ( oss . str () == 
    "progtest A 147.32.232.142\n"
    "progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "progtest AAAA 2001:718:2:2902:1:2:3:4\n" );
  assert ( z0 . del ( CRecA ( "courses", CIPv4 ( "147.32.232.160" ) ) ) == true );
  assert ( z0 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.122" ) ) ) == true );
  oss . str ( "" );
  oss << z0;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- progtest A 147.32.232.142\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.159\n"
    " +- progtest AAAA 2001:718:2:2902:1:2:3:4\n"
    " +- courses MX 0 relay.fit.cvut.cz.\n"
    " +- courses MX 10 relay2.fit.cvut.cz.\n"
    " \\- courses A 147.32.232.122\n" );
  assert ( z0 . search ( "courses" ) . size () == 5 );
  oss . str ( "" );
  oss << z0 . search ( "courses" );
  assert ( oss . str () == 
    "courses A 147.32.232.158\n"
    "courses A 147.32.232.159\n"
    "courses MX 0 relay.fit.cvut.cz.\n"
    "courses MX 10 relay2.fit.cvut.cz.\n"
    "courses A 147.32.232.122\n" );
  oss . str ( "" );
  oss << z0 . search ( "courses" ) [ 0 ];
  assert ( oss . str () == "courses A 147.32.232.158" );
  assert ( z0 . search ( "courses" ) [ 0 ] . name () == "courses" );
  assert ( z0 . search ( "courses" ) [ 0 ] . type () == "A" );
  oss . str ( "" );
  oss << z0 . search ( "courses" ) [ 1 ];
  assert ( oss . str () == "courses A 147.32.232.159" );
  assert ( z0 . search ( "courses" ) [ 1 ] . name () == "courses" );
  assert ( z0 . search ( "courses" ) [ 1 ] . type () == "A" );
  oss . str ( "" );
  oss << z0 . search ( "courses" ) [ 2 ];
  assert ( oss . str () == "courses MX 0 relay.fit.cvut.cz." );
  assert ( z0 . search ( "courses" ) [ 2 ] . name () == "courses" );
  assert ( z0 . search ( "courses" ) [ 2 ] . type () == "MX" );
  try
  {
    oss . str ( "" );
    oss << z0 . search ( "courses" ) [ 10 ];
    assert ( "No exception thrown!" == nullptr );
  }
  catch ( const std::out_of_range & e )
  {
  }
  catch ( ... )
  {
    assert ( "Invalid exception thrown!" == nullptr );
  }
  dynamic_cast<const CRecAAAA &> ( z0 . search ( "progtest" ) [ 1 ] );
  CZone z1 ( "fit2.cvut.cz" );
  z1 . add ( z0 . search ( "progtest" ) [ 2 ] );
  z1 . add ( z0 . search ( "progtest" ) [ 0 ] );
  z1 . add ( z0 . search ( "progtest" ) [ 1 ] );
  z1 . add ( z0 . search ( "courses" ) [ 2 ] );
  oss . str ( "" );
  oss << z1;
  assert ( oss . str () == 
    "fit2.cvut.cz\n"
    " +- progtest AAAA 2001:718:2:2902:1:2:3:4\n"
    " +- progtest A 147.32.232.142\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " \\- courses MX 0 relay.fit.cvut.cz.\n" );
  dynamic_cast<const CRecA &> ( z1 . search ( "progtest" ) [ 1 ] );

  CZone z2 ( "fit.cvut.cz" );
  assert ( z2 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  assert ( z2 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:0:1:2:3" ) ) ) == true );
  assert ( z2 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.144" ) ) ) == true );
  assert ( z2 . add ( CRecMX ( "progtest", "relay.fit.cvut.cz.", 10 ) ) == true );
  assert ( z2 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == false );
  assert ( z2 . del ( CRecA ( "progtest", CIPv4 ( "147.32.232.140" ) ) ) == false );
  assert ( z2 . del ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  assert ( z2 . del ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == false );
  assert ( z2 . add ( CRecMX ( "progtest", "relay.fit.cvut.cz.", 20 ) ) == true );
  assert ( z2 . add ( CRecMX ( "progtest", "relay.fit.cvut.cz.", 10 ) ) == false );
  oss . str ( "" );
  oss << z2;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- progtest A 147.32.232.144\n"
    " +- progtest MX 10 relay.fit.cvut.cz.\n"
    " \\- progtest MX 20 relay.fit.cvut.cz.\n" );
  assert ( z2 . search ( "progtest" ) . size () == 4 );
  oss . str ( "" );
  oss << z2 . search ( "progtest" );
  assert ( oss . str () == 
    "progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "progtest A 147.32.232.144\n"
    "progtest MX 10 relay.fit.cvut.cz.\n"
    "progtest MX 20 relay.fit.cvut.cz.\n" );
  assert ( z2 . search ( "courses" ) . size () == 0 );
  oss . str ( "" );
  oss << z2 . search ( "courses" );
  assert ( oss . str () == "" );
  try
  {
    dynamic_cast<const CRecMX &> ( z2 . search ( "progtest" ) [ 0 ] );
    assert ( "Invalid type" == nullptr );
  }
  catch ( const std::bad_cast & e )
  {
  }

  CZone z4 ( "fit.cvut.cz" );
  assert ( z4 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  assert ( z4 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.158" ) ) ) == true );
  assert ( z4 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.160" ) ) ) == true );
  assert ( z4 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.159" ) ) ) == true );
  CZone z5 ( z4 );
  assert ( z4 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:0:1:2:3" ) ) ) == true );
  assert ( z4 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:1:2:3:4" ) ) ) == true );
  assert ( z5 . del ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  oss . str ( "" );
  oss << z4;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- progtest A 147.32.232.142\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " +- courses A 147.32.232.159\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " \\- progtest AAAA 2001:718:2:2902:1:2:3:4\n" );
  oss . str ( "" );
  oss << z5;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " \\- courses A 147.32.232.159\n" );
  z5 = z4;
  assert ( z4 . add ( CRecMX ( "courses", "relay.fit.cvut.cz.", 0 ) ) == true );
  assert ( z4 . add ( CRecMX ( "courses", "relay2.fit.cvut.cz.", 10 ) ) == true );
  oss . str ( "" );
  oss << z4;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- progtest A 147.32.232.142\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " +- courses A 147.32.232.159\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- progtest AAAA 2001:718:2:2902:1:2:3:4\n"
    " +- courses MX 0 relay.fit.cvut.cz.\n"
    " \\- courses MX 10 relay2.fit.cvut.cz.\n" );
  oss . str ( "" );
  oss << z5;
  assert ( oss . str () == 
    "fit.cvut.cz\n"
    " +- progtest A 147.32.232.142\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " +- courses A 147.32.232.159\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " \\- progtest AAAA 2001:718:2:2902:1:2:3:4\n" );

  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
