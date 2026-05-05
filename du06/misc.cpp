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
// ============================================================================
// 1. BASE CLASS & HELPERS
// ============================================================================
class CRecord {
protected:
    std::string m_Name;
    virtual bool cmp(const CRecord& other) const = 0; 

public:
    CRecord(const std::string& n) : m_Name(n) {}
    virtual ~CRecord() = default;
    
    // SAFETY FIX: Explicit Rule of Five to prevent compiler warnings
    CRecord(const CRecord&) = default;
    CRecord& operator=(const CRecord&) = default;

    virtual CRecord* clone() const = 0;
    virtual std::string type() const = 0;
    virtual void print(std::ostream& os) const = 0;
    virtual bool isZone() const { return false; }
    virtual bool isNL() const { return false; } 
    
    std::string name() const { return m_Name; }
    bool isEqual(const CRecord& o) const { return type() == o.type() && name() == o.name() && cmp(o); }

    virtual void printNode(std::ostream& os, const std::string& pfx, bool last) const {
        os << pfx << (last ? " \\- " : " +- "); print(os); os << "\n";
    }
    friend std::ostream& operator<<(std::ostream& os, const CRecord& r) { r.print(os); return os; }
};

class CZoneSearch {
    std::vector<CRecord*> m_M;
public:
    void add(CRecord* r) { m_M.push_back(r); }
    size_t size() const { return m_M.size(); }
    
    CRecord& operator[](size_t i) const { 
        // CRITICAL FIX: Added std::to_string(i) as required by the assignment
        if (i >= m_M.size()) throw std::out_of_range(std::to_string(i)); 
        return *m_M[i]; 
    }
    
    friend std::ostream& operator<<(std::ostream& os, const CZoneSearch& z) {
        for (auto* r : z.m_M) { os << *r; if (!r->isNL()) os << "\n"; }
        return os;
    }
};

// ============================================================================
// 2. DNS RECORDS (Ultra-Compact)
// ============================================================================
class CRecA : public CRecord {
    CIPv4 m_IP;
    bool cmp(const CRecord& o) const override { return m_IP == dynamic_cast<const CRecA&>(o).m_IP; }
public:
    CRecA(const std::string& n, const CIPv4& ip) : CRecord(n), m_IP(ip) {}
    CRecord* clone() const override { return new CRecA(*this); }
    std::string type() const override { return "A"; }
    void print(std::ostream& os) const override { os << m_Name << " A " << m_IP; }
};

class CRecAAAA : public CRecord {
    CIPv6 m_IP;
    bool cmp(const CRecord& o) const override { return m_IP == dynamic_cast<const CRecAAAA&>(o).m_IP; }
public:
    CRecAAAA(const std::string& n, const CIPv6& ip) : CRecord(n), m_IP(ip) {}
    CRecord* clone() const override { return new CRecAAAA(*this); }
    std::string type() const override { return "AAAA"; }
    void print(std::ostream& os) const override { os << m_Name << " AAAA " << m_IP; }
};

class CRecMX : public CRecord {
    std::string m_Mail; int m_Prio;
    bool cmp(const CRecord& o) const override { 
        const auto& x = dynamic_cast<const CRecMX&>(o); return m_Mail == x.m_Mail && m_Prio == x.m_Prio; 
    }
public:
    CRecMX(const std::string& n, const std::string& m, int p) : CRecord(n), m_Mail(m), m_Prio(p) {}
    CRecord* clone() const override { return new CRecMX(*this); }
    std::string type() const override { return "MX"; }
    void print(std::ostream& os) const override { os << m_Name << " MX " << m_Prio << " " << m_Mail; }
};

class CRecCNAME : public CRecord {
    std::string m_Alias;
    bool cmp(const CRecord& o) const override { return m_Alias == dynamic_cast<const CRecCNAME&>(o).m_Alias; }
public:
    CRecCNAME(const std::string& n, const std::string& a) : CRecord(n), m_Alias(a) {}
    CRecord* clone() const override { return new CRecCNAME(*this); }
    std::string type() const override { return "CNAME"; }
    void print(std::ostream& os) const override { os << m_Name << " CNAME " << m_Alias; }
};

class CRecSPF : public CRecord {
    std::vector<std::string> m_Addrs;
    bool cmp(const CRecord& o) const override { return m_Addrs == dynamic_cast<const CRecSPF&>(o).m_Addrs; }
public:
    CRecSPF(const std::string& n) : CRecord(n) {}
    CRecSPF& add(const std::string& a) { m_Addrs.push_back(a); return *this; }
    CRecord* clone() const override { return new CRecSPF(*this); }
    std::string type() const override { return "SPF"; }
    void print(std::ostream& os) const override {
        os << m_Name << " SPF ";
        for (size_t i = 0; i < m_Addrs.size(); ++i) os << (i > 0 ? ", " : "") << m_Addrs[i];
    }
};

// ============================================================================
// 3. THE CONTAINER (CZone)
// ============================================================================
class CZone : public CRecord {
    std::vector<std::unique_ptr<CRecord>> m_Recs;
    std::unordered_map<std::string, std::vector<CRecord*>> m_Idx;
    bool cmp(const CRecord&) const override { return true; } 

public:
    CZone(const std::string& n) : CRecord(n) {}
    
    CZone(const CZone& o) : CRecord(o.m_Name) { *this = o; }
    
    CZone& operator=(const CZone& o) {
        if (this == &o) return *this;
        m_Name = o.m_Name; m_Recs.clear(); m_Idx.clear();
        m_Recs.reserve(o.m_Recs.size());
        for (const auto& r : o.m_Recs) { 
            m_Recs.emplace_back(r->clone()); 
            m_Idx[r->name()].push_back(m_Recs.back().get()); 
        }
        return *this;
    }

    CRecord* clone() const override { return new CZone(*this); }
    std::string type() const override { return "ZONE"; }
    bool isZone() const override { return true; }
    bool isNL() const override { return true; }

    bool add(const CRecord& r) {
        if (m_Idx.count(r.name())) {
            bool isEx = (r.type() == "CNAME" || r.isZone());
            for (const auto* existing : m_Idx.at(r.name())) {
                if (isEx || existing->type() == "CNAME" || existing->isZone() || existing->isEqual(r)) 
                    return false; 
            }
        }
        m_Recs.emplace_back(r.clone());
        m_Idx[r.name()].push_back(m_Recs.back().get());
        return true;
    }

    bool del(const CRecord& r) {
        if (!m_Idx.count(r.name())) return false;
        auto& lst = m_Idx.at(r.name());
        
        auto it = std::find_if(lst.begin(), lst.end(), [&](CRecord* p) { return p->isEqual(r); });
        if (it == lst.end()) return false;
        
        CRecord* ptr = *it; 
        
        // SAFETY FIX: Explicit const reference instead of auto&
        m_Recs.erase(std::find_if(m_Recs.begin(), m_Recs.end(), [ptr](const std::unique_ptr<CRecord>& u) { return u.get() == ptr; }));
        lst.erase(it);
        return true;
    }

    CZoneSearch search(const std::string& q) const {
        size_t p = q.find_last_of('.');
        if (p == std::string::npos) {
            CZoneSearch res;
            if (m_Idx.count(q)) for (auto* ptr : m_Idx.at(q)) res.add(ptr);
            return res;
        } 
        
        std::string sub = q.substr(p + 1), rem = q.substr(0, p);
        if (m_Idx.count(sub)) {
            for (auto* ptr : m_Idx.at(sub)) {
                if (ptr->isZone()) return dynamic_cast<const CZone*>(ptr)->search(rem);
            }
        }
        return CZoneSearch();
    }

    void print(std::ostream& os) const override {
        os << m_Name << "\n";
        for (size_t i = 0; i < m_Recs.size(); ++i) m_Recs[i]->printNode(os, "", i == m_Recs.size() - 1);
    }

    void printNode(std::ostream& os, const std::string& pfx, bool last) const override {
        os << pfx << (last ? " \\- " : " +- ") << m_Name << "\n";
        std::string nPfx = pfx + (last ? "   " : "|  ");
        for (size_t i = 0; i < m_Recs.size(); ++i) m_Recs[i]->printNode(os, nPfx, i == m_Recs.size() - 1);
    }
};

#ifndef __PROGTEST__
int main ()
{
  std::ostringstream oss;

  CZone z0 ( "fit" );
  assert ( z0 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  assert ( z0 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:0:1:2:3" ) ) ) == true );
  assert ( z0 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.158" ) ) ) == true );
  assert ( z0 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.160" ) ) ) == true );
  assert ( z0 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.159" ) ) ) == true );
  assert ( z0 . add ( CRecCNAME ( "pririz", "sto.fit.cvut.cz." ) ) == true );
  assert ( z0 . add ( CRecSPF ( "courses" ) . add ( "ip4:147.32.232.128/25" ) . add ( "ip4:147.32.232.64/26" ) ) == true );
  assert ( z0 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:1:2:3:4" ) ) ) == true );
  assert ( z0 . add ( CRecMX ( "courses", "relay.fit.cvut.cz.", 0 ) ) == true );
  assert ( z0 . add ( CRecMX ( "courses", "relay2.fit.cvut.cz.", 10 ) ) == true );
  oss . str ( "" );
  oss << z0;
  assert ( oss . str () == 
    "fit\n"
    " +- progtest A 147.32.232.142\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " +- courses A 147.32.232.159\n"
    " +- pririz CNAME sto.fit.cvut.cz.\n"
    " +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
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
    "fit\n"
    " +- progtest A 147.32.232.142\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.159\n"
    " +- pririz CNAME sto.fit.cvut.cz.\n"
    " +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " +- progtest AAAA 2001:718:2:2902:1:2:3:4\n"
    " +- courses MX 0 relay.fit.cvut.cz.\n"
    " +- courses MX 10 relay2.fit.cvut.cz.\n"
    " \\- courses A 147.32.232.122\n" );
  assert ( z0 . search ( "courses" ) . size () == 6 );
  oss . str ( "" );
  oss << z0 . search ( "courses" );
  assert ( oss . str () == 
    "courses A 147.32.232.158\n"
    "courses A 147.32.232.159\n"
    "courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
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
  assert ( oss . str () == "courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26" );
  assert ( z0 . search ( "courses" ) [ 2 ] . name () == "courses" );
  assert ( z0 . search ( "courses" ) [ 2 ] . type () == "SPF" );
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
  CZone z1 ( "fit2" );
  z1 . add ( z0 . search ( "progtest" ) [ 2 ] );
  z1 . add ( z0 . search ( "progtest" ) [ 0 ] );
  z1 . add ( z0 . search ( "progtest" ) [ 1 ] );
  z1 . add ( z0 . search ( "courses" ) [ 2 ] );
  oss . str ( "" );
  oss << z1;
  assert ( oss . str () == 
    "fit2\n"
    " +- progtest AAAA 2001:718:2:2902:1:2:3:4\n"
    " +- progtest A 147.32.232.142\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " \\- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n" );
  dynamic_cast<const CRecA &> ( z1 . search ( "progtest" ) [ 1 ] );

  CZone z10 ( "fit" );
  assert ( z10 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  assert ( z10 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:0:1:2:3" ) ) ) == true );
  assert ( z10 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.144" ) ) ) == true );
  assert ( z10 . add ( CRecMX ( "progtest", "relay.fit.cvut.cz.", 10 ) ) == true );
  assert ( z10 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == false );
  assert ( z10 . del ( CRecA ( "progtest", CIPv4 ( "147.32.232.140" ) ) ) == false );
  assert ( z10 . del ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  assert ( z10 . del ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == false );
  assert ( z10 . add ( CRecMX ( "progtest", "relay.fit.cvut.cz.", 20 ) ) == true );
  assert ( z10 . add ( CRecMX ( "progtest", "relay.fit.cvut.cz.", 10 ) ) == false );
  assert ( z10 . add ( CRecCNAME ( "pririz", "sto.fit.cvut.cz." ) ) == true );
  assert ( z10 . add ( CRecCNAME ( "pririz", "stojedna.fit.cvut.cz." ) ) == false );
  assert ( z10 . add ( CRecA ( "pririz", CIPv4 ( "147.32.232.111" ) ) ) == false );
  assert ( z10 . add ( CRecCNAME ( "progtest", "progtestbak.fit.cvut.cz." ) ) == false );
  assert ( z10 . add ( CZone ( "test" ) ) == true );
  assert ( z10 . add ( CZone ( "pririz" ) ) == false );
  assert ( z10 . add ( CRecA ( "test", CIPv4 ( "147.32.232.232" ) ) ) == false );
  oss . str ( "" );
  oss << z10;
  assert ( oss . str () == 
    "fit\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- progtest A 147.32.232.144\n"
    " +- progtest MX 10 relay.fit.cvut.cz.\n"
    " +- progtest MX 20 relay.fit.cvut.cz.\n"
    " +- pririz CNAME sto.fit.cvut.cz.\n"
    " \\- test\n" );
  assert ( z10 . search ( "progtest" ) . size () == 4 );
  oss . str ( "" );
  oss << z10 . search ( "progtest" );
  assert ( oss . str () == 
    "progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "progtest A 147.32.232.144\n"
    "progtest MX 10 relay.fit.cvut.cz.\n"
    "progtest MX 20 relay.fit.cvut.cz.\n" );
  assert ( z10 . search ( "courses" ) . size () == 0 );
  oss . str ( "" );
  oss << z10 . search ( "courses" );
  assert ( oss . str () == "" );

  CZone z20 ( "<ROOT ZONE>" );
  CZone z21 ( "cz" );
  CZone z22 ( "cvut" );
  CZone z23 ( "fit" );
  assert ( z23 . add ( CRecA ( "progtest", CIPv4 ( "147.32.232.142" ) ) ) == true );
  assert ( z23 . add ( CRecAAAA ( "progtest", CIPv6 ( "2001:718:2:2902:0:1:2:3" ) ) ) == true );
  assert ( z23 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.158" ) ) ) == true );
  assert ( z23 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.160" ) ) ) == true );
  assert ( z23 . add ( CRecA ( "courses", CIPv4 ( "147.32.232.159" ) ) ) == true );
  assert ( z23 . add ( CRecCNAME ( "pririz", "sto.fit.cvut.cz." ) ) == true );
  assert ( z23 . add ( CRecSPF ( "courses" ) . add ( "ip4:147.32.232.128/25" ) . add ( "ip4:147.32.232.64/26" ) ) == true );
  CZone z24 ( "fel" );
  assert ( z24 . add ( CRecA ( "www", CIPv4 ( "147.32.80.2" ) ) ) == true );
  assert ( z24 . add ( CRecAAAA ( "www", CIPv6 ( "1:2:3:4:5:6:7:8" ) ) ) == true );
  assert ( z22 . add ( z23 ) == true );
  assert ( z22 . add ( z24 ) == true );
  assert ( z21 . add ( z22 ) == true );
  assert ( z20 . add ( z21 ) == true );
  assert ( z23 . add ( CRecA ( "www", CIPv4 ( "147.32.90.1" ) ) ) == true );
  oss . str ( "" );
  oss << z20;
  assert ( oss . str () == 
    "<ROOT ZONE>\n"
    " \\- cz\n"
    "    \\- cvut\n"
    "       +- fit\n"
    "       |  +- progtest A 147.32.232.142\n"
    "       |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "       |  +- courses A 147.32.232.158\n"
    "       |  +- courses A 147.32.232.160\n"
    "       |  +- courses A 147.32.232.159\n"
    "       |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "       |  \\- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "       \\- fel\n"
    "          +- www A 147.32.80.2\n"
    "          \\- www AAAA 1:2:3:4:5:6:7:8\n" );
  oss . str ( "" );
  oss << z21;
  assert ( oss . str () == 
    "cz\n"
    " \\- cvut\n"
    "    +- fit\n"
    "    |  +- progtest A 147.32.232.142\n"
    "    |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "    |  +- courses A 147.32.232.158\n"
    "    |  +- courses A 147.32.232.160\n"
    "    |  +- courses A 147.32.232.159\n"
    "    |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "    |  \\- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "    \\- fel\n"
    "       +- www A 147.32.80.2\n"
    "       \\- www AAAA 1:2:3:4:5:6:7:8\n" );
  oss . str ( "" );
  oss << z22;
  assert ( oss . str () == 
    "cvut\n"
    " +- fit\n"
    " |  +- progtest A 147.32.232.142\n"
    " |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |  +- courses A 147.32.232.158\n"
    " |  +- courses A 147.32.232.160\n"
    " |  +- courses A 147.32.232.159\n"
    " |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |  \\- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " \\- fel\n"
    "    +- www A 147.32.80.2\n"
    "    \\- www AAAA 1:2:3:4:5:6:7:8\n" );
  oss . str ( "" );
  oss << z23;
  assert ( oss . str () == 
    "fit\n"
    " +- progtest A 147.32.232.142\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " +- courses A 147.32.232.159\n"
    " +- pririz CNAME sto.fit.cvut.cz.\n"
    " +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " \\- www A 147.32.90.1\n" );
  oss . str ( "" );
  oss << z24;
  assert ( oss . str () == 
    "fel\n"
    " +- www A 147.32.80.2\n"
    " \\- www AAAA 1:2:3:4:5:6:7:8\n" );
  assert ( z20 . search ( "progtest.fit.cvut.cz" ) . size () == 2 );
  oss . str ( "" );
  oss << z20 . search ( "progtest.fit.cvut.cz" );
  assert ( oss . str () == 
    "progtest A 147.32.232.142\n"
    "progtest AAAA 2001:718:2:2902:0:1:2:3\n" );
  assert ( z20 . search ( "fit.cvut.cz" ) . size () == 1 );
  oss . str ( "" );
  oss << z20 . search ( "fit.cvut.cz" );
  assert ( oss . str () == 
    "fit\n"
    " +- progtest A 147.32.232.142\n"
    " +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " +- courses A 147.32.232.158\n"
    " +- courses A 147.32.232.160\n"
    " +- courses A 147.32.232.159\n"
    " +- pririz CNAME sto.fit.cvut.cz.\n"
    " \\- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n" );
  assert ( dynamic_cast<CZone &> ( z20 . search ( "fit.cvut.cz" ) [0] ) . add ( z20 . search ( "fel.cvut.cz" ) [0] ) == true );
  oss . str ( "" );
  oss << z20;
  assert ( oss . str () == 
    "<ROOT ZONE>\n"
    " \\- cz\n"
    "    \\- cvut\n"
    "       +- fit\n"
    "       |  +- progtest A 147.32.232.142\n"
    "       |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "       |  +- courses A 147.32.232.158\n"
    "       |  +- courses A 147.32.232.160\n"
    "       |  +- courses A 147.32.232.159\n"
    "       |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "       |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "       |  \\- fel\n"
    "       |     +- www A 147.32.80.2\n"
    "       |     \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       \\- fel\n"
    "          +- www A 147.32.80.2\n"
    "          \\- www AAAA 1:2:3:4:5:6:7:8\n" );
  assert ( dynamic_cast<CZone &> ( z20 . search ( "fit.cvut.cz" ) [0] ) . add ( z20 . search ( "cz" ) [0] ) == true );
  oss . str ( "" );
  oss << z20;
  assert ( oss . str () == 
    "<ROOT ZONE>\n"
    " \\- cz\n"
    "    \\- cvut\n"
    "       +- fit\n"
    "       |  +- progtest A 147.32.232.142\n"
    "       |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "       |  +- courses A 147.32.232.158\n"
    "       |  +- courses A 147.32.232.160\n"
    "       |  +- courses A 147.32.232.159\n"
    "       |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "       |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "       |  +- fel\n"
    "       |  |  +- www A 147.32.80.2\n"
    "       |  |  \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |  \\- cz\n"
    "       |     \\- cvut\n"
    "       |        +- fit\n"
    "       |        |  +- progtest A 147.32.232.142\n"
    "       |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "       |        |  +- courses A 147.32.232.158\n"
    "       |        |  +- courses A 147.32.232.160\n"
    "       |        |  +- courses A 147.32.232.159\n"
    "       |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "       |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "       |        |  \\- fel\n"
    "       |        |     +- www A 147.32.80.2\n"
    "       |        |     \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |        \\- fel\n"
    "       |           +- www A 147.32.80.2\n"
    "       |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       \\- fel\n"
    "          +- www A 147.32.80.2\n"
    "          \\- www AAAA 1:2:3:4:5:6:7:8\n" );
  assert ( dynamic_cast<CZone &> ( z20 . search ( "fit.cvut.cz.fit.cvut.cz" ) [0] ) . add ( z20 . search ( "cz" ) [0] ) == true );
  oss . str ( "" );
  oss << z20;
  assert ( oss . str () == 
    "<ROOT ZONE>\n"
    " \\- cz\n"
    "    \\- cvut\n"
    "       +- fit\n"
    "       |  +- progtest A 147.32.232.142\n"
    "       |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "       |  +- courses A 147.32.232.158\n"
    "       |  +- courses A 147.32.232.160\n"
    "       |  +- courses A 147.32.232.159\n"
    "       |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "       |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "       |  +- fel\n"
    "       |  |  +- www A 147.32.80.2\n"
    "       |  |  \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |  \\- cz\n"
    "       |     \\- cvut\n"
    "       |        +- fit\n"
    "       |        |  +- progtest A 147.32.232.142\n"
    "       |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "       |        |  +- courses A 147.32.232.158\n"
    "       |        |  +- courses A 147.32.232.160\n"
    "       |        |  +- courses A 147.32.232.159\n"
    "       |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "       |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "       |        |  +- fel\n"
    "       |        |  |  +- www A 147.32.80.2\n"
    "       |        |  |  \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |        |  \\- cz\n"
    "       |        |     \\- cvut\n"
    "       |        |        +- fit\n"
    "       |        |        |  +- progtest A 147.32.232.142\n"
    "       |        |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "       |        |        |  +- courses A 147.32.232.158\n"
    "       |        |        |  +- courses A 147.32.232.160\n"
    "       |        |        |  +- courses A 147.32.232.159\n"
    "       |        |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "       |        |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "       |        |        |  +- fel\n"
    "       |        |        |  |  +- www A 147.32.80.2\n"
    "       |        |        |  |  \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |        |        |  \\- cz\n"
    "       |        |        |     \\- cvut\n"
    "       |        |        |        +- fit\n"
    "       |        |        |        |  +- progtest A 147.32.232.142\n"
    "       |        |        |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "       |        |        |        |  +- courses A 147.32.232.158\n"
    "       |        |        |        |  +- courses A 147.32.232.160\n"
    "       |        |        |        |  +- courses A 147.32.232.159\n"
    "       |        |        |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "       |        |        |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "       |        |        |        |  \\- fel\n"
    "       |        |        |        |     +- www A 147.32.80.2\n"
    "       |        |        |        |     \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |        |        |        \\- fel\n"
    "       |        |        |           +- www A 147.32.80.2\n"
    "       |        |        |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |        |        \\- fel\n"
    "       |        |           +- www A 147.32.80.2\n"
    "       |        |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |        \\- fel\n"
    "       |           +- www A 147.32.80.2\n"
    "       |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       \\- fel\n"
    "          +- www A 147.32.80.2\n"
    "          \\- www AAAA 1:2:3:4:5:6:7:8\n" );
  assert ( dynamic_cast<CZone &> ( z20 . search ( "fit.cvut.cz.fit.cvut.cz" ) [0] ) . del ( CZone ( "fel" ) ) == true );
  oss . str ( "" );
  oss << z20;
  assert ( oss . str () == 
    "<ROOT ZONE>\n"
    " \\- cz\n"
    "    \\- cvut\n"
    "       +- fit\n"
    "       |  +- progtest A 147.32.232.142\n"
    "       |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "       |  +- courses A 147.32.232.158\n"
    "       |  +- courses A 147.32.232.160\n"
    "       |  +- courses A 147.32.232.159\n"
    "       |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "       |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "       |  +- fel\n"
    "       |  |  +- www A 147.32.80.2\n"
    "       |  |  \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |  \\- cz\n"
    "       |     \\- cvut\n"
    "       |        +- fit\n"
    "       |        |  +- progtest A 147.32.232.142\n"
    "       |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "       |        |  +- courses A 147.32.232.158\n"
    "       |        |  +- courses A 147.32.232.160\n"
    "       |        |  +- courses A 147.32.232.159\n"
    "       |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "       |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "       |        |  \\- cz\n"
    "       |        |     \\- cvut\n"
    "       |        |        +- fit\n"
    "       |        |        |  +- progtest A 147.32.232.142\n"
    "       |        |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "       |        |        |  +- courses A 147.32.232.158\n"
    "       |        |        |  +- courses A 147.32.232.160\n"
    "       |        |        |  +- courses A 147.32.232.159\n"
    "       |        |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "       |        |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "       |        |        |  +- fel\n"
    "       |        |        |  |  +- www A 147.32.80.2\n"
    "       |        |        |  |  \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |        |        |  \\- cz\n"
    "       |        |        |     \\- cvut\n"
    "       |        |        |        +- fit\n"
    "       |        |        |        |  +- progtest A 147.32.232.142\n"
    "       |        |        |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    "       |        |        |        |  +- courses A 147.32.232.158\n"
    "       |        |        |        |  +- courses A 147.32.232.160\n"
    "       |        |        |        |  +- courses A 147.32.232.159\n"
    "       |        |        |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    "       |        |        |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    "       |        |        |        |  \\- fel\n"
    "       |        |        |        |     +- www A 147.32.80.2\n"
    "       |        |        |        |     \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |        |        |        \\- fel\n"
    "       |        |        |           +- www A 147.32.80.2\n"
    "       |        |        |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |        |        \\- fel\n"
    "       |        |           +- www A 147.32.80.2\n"
    "       |        |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       |        \\- fel\n"
    "       |           +- www A 147.32.80.2\n"
    "       |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    "       \\- fel\n"
    "          +- www A 147.32.80.2\n"
    "          \\- www AAAA 1:2:3:4:5:6:7:8\n" );
  CZone z25 ( z20 );
z22 = z20;
  assert ( z20 . add ( CZone ( "sk" ) ) == true );
  assert ( z25 . add ( CZone ( "au" ) ) == true );
  assert ( z22 . add ( CZone ( "de" ) ) == true );
  oss . str ( "" );
  oss << z20;
  assert ( oss . str () == 
    "<ROOT ZONE>\n"
    " +- cz\n"
    " |  \\- cvut\n"
    " |     +- fit\n"
    " |     |  +- progtest A 147.32.232.142\n"
    " |     |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |     |  +- courses A 147.32.232.158\n"
    " |     |  +- courses A 147.32.232.160\n"
    " |     |  +- courses A 147.32.232.159\n"
    " |     |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |     |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " |     |  +- fel\n"
    " |     |  |  +- www A 147.32.80.2\n"
    " |     |  |  \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |  \\- cz\n"
    " |     |     \\- cvut\n"
    " |     |        +- fit\n"
    " |     |        |  +- progtest A 147.32.232.142\n"
    " |     |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |     |        |  +- courses A 147.32.232.158\n"
    " |     |        |  +- courses A 147.32.232.160\n"
    " |     |        |  +- courses A 147.32.232.159\n"
    " |     |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |     |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " |     |        |  \\- cz\n"
    " |     |        |     \\- cvut\n"
    " |     |        |        +- fit\n"
    " |     |        |        |  +- progtest A 147.32.232.142\n"
    " |     |        |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |     |        |        |  +- courses A 147.32.232.158\n"
    " |     |        |        |  +- courses A 147.32.232.160\n"
    " |     |        |        |  +- courses A 147.32.232.159\n"
    " |     |        |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |     |        |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " |     |        |        |  +- fel\n"
    " |     |        |        |  |  +- www A 147.32.80.2\n"
    " |     |        |        |  |  \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |        |        |  \\- cz\n"
    " |     |        |        |     \\- cvut\n"
    " |     |        |        |        +- fit\n"
    " |     |        |        |        |  +- progtest A 147.32.232.142\n"
    " |     |        |        |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |     |        |        |        |  +- courses A 147.32.232.158\n"
    " |     |        |        |        |  +- courses A 147.32.232.160\n"
    " |     |        |        |        |  +- courses A 147.32.232.159\n"
    " |     |        |        |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |     |        |        |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " |     |        |        |        |  \\- fel\n"
    " |     |        |        |        |     +- www A 147.32.80.2\n"
    " |     |        |        |        |     \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |        |        |        \\- fel\n"
    " |     |        |        |           +- www A 147.32.80.2\n"
    " |     |        |        |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |        |        \\- fel\n"
    " |     |        |           +- www A 147.32.80.2\n"
    " |     |        |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |        \\- fel\n"
    " |     |           +- www A 147.32.80.2\n"
    " |     |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     \\- fel\n"
    " |        +- www A 147.32.80.2\n"
    " |        \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " \\- sk\n" );
  oss . str ( "" );
  oss << z22;
  assert ( oss . str () == 
    "<ROOT ZONE>\n"
    " +- cz\n"
    " |  \\- cvut\n"
    " |     +- fit\n"
    " |     |  +- progtest A 147.32.232.142\n"
    " |     |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |     |  +- courses A 147.32.232.158\n"
    " |     |  +- courses A 147.32.232.160\n"
    " |     |  +- courses A 147.32.232.159\n"
    " |     |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |     |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " |     |  +- fel\n"
    " |     |  |  +- www A 147.32.80.2\n"
    " |     |  |  \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |  \\- cz\n"
    " |     |     \\- cvut\n"
    " |     |        +- fit\n"
    " |     |        |  +- progtest A 147.32.232.142\n"
    " |     |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |     |        |  +- courses A 147.32.232.158\n"
    " |     |        |  +- courses A 147.32.232.160\n"
    " |     |        |  +- courses A 147.32.232.159\n"
    " |     |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |     |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " |     |        |  \\- cz\n"
    " |     |        |     \\- cvut\n"
    " |     |        |        +- fit\n"
    " |     |        |        |  +- progtest A 147.32.232.142\n"
    " |     |        |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |     |        |        |  +- courses A 147.32.232.158\n"
    " |     |        |        |  +- courses A 147.32.232.160\n"
    " |     |        |        |  +- courses A 147.32.232.159\n"
    " |     |        |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |     |        |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " |     |        |        |  +- fel\n"
    " |     |        |        |  |  +- www A 147.32.80.2\n"
    " |     |        |        |  |  \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |        |        |  \\- cz\n"
    " |     |        |        |     \\- cvut\n"
    " |     |        |        |        +- fit\n"
    " |     |        |        |        |  +- progtest A 147.32.232.142\n"
    " |     |        |        |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |     |        |        |        |  +- courses A 147.32.232.158\n"
    " |     |        |        |        |  +- courses A 147.32.232.160\n"
    " |     |        |        |        |  +- courses A 147.32.232.159\n"
    " |     |        |        |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |     |        |        |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " |     |        |        |        |  \\- fel\n"
    " |     |        |        |        |     +- www A 147.32.80.2\n"
    " |     |        |        |        |     \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |        |        |        \\- fel\n"
    " |     |        |        |           +- www A 147.32.80.2\n"
    " |     |        |        |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |        |        \\- fel\n"
    " |     |        |           +- www A 147.32.80.2\n"
    " |     |        |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |        \\- fel\n"
    " |     |           +- www A 147.32.80.2\n"
    " |     |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     \\- fel\n"
    " |        +- www A 147.32.80.2\n"
    " |        \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " \\- de\n" );
  oss . str ( "" );
  oss << z25;
  assert ( oss . str () == 
    "<ROOT ZONE>\n"
    " +- cz\n"
    " |  \\- cvut\n"
    " |     +- fit\n"
    " |     |  +- progtest A 147.32.232.142\n"
    " |     |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |     |  +- courses A 147.32.232.158\n"
    " |     |  +- courses A 147.32.232.160\n"
    " |     |  +- courses A 147.32.232.159\n"
    " |     |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |     |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " |     |  +- fel\n"
    " |     |  |  +- www A 147.32.80.2\n"
    " |     |  |  \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |  \\- cz\n"
    " |     |     \\- cvut\n"
    " |     |        +- fit\n"
    " |     |        |  +- progtest A 147.32.232.142\n"
    " |     |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |     |        |  +- courses A 147.32.232.158\n"
    " |     |        |  +- courses A 147.32.232.160\n"
    " |     |        |  +- courses A 147.32.232.159\n"
    " |     |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |     |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " |     |        |  \\- cz\n"
    " |     |        |     \\- cvut\n"
    " |     |        |        +- fit\n"
    " |     |        |        |  +- progtest A 147.32.232.142\n"
    " |     |        |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |     |        |        |  +- courses A 147.32.232.158\n"
    " |     |        |        |  +- courses A 147.32.232.160\n"
    " |     |        |        |  +- courses A 147.32.232.159\n"
    " |     |        |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |     |        |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " |     |        |        |  +- fel\n"
    " |     |        |        |  |  +- www A 147.32.80.2\n"
    " |     |        |        |  |  \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |        |        |  \\- cz\n"
    " |     |        |        |     \\- cvut\n"
    " |     |        |        |        +- fit\n"
    " |     |        |        |        |  +- progtest A 147.32.232.142\n"
    " |     |        |        |        |  +- progtest AAAA 2001:718:2:2902:0:1:2:3\n"
    " |     |        |        |        |  +- courses A 147.32.232.158\n"
    " |     |        |        |        |  +- courses A 147.32.232.160\n"
    " |     |        |        |        |  +- courses A 147.32.232.159\n"
    " |     |        |        |        |  +- pririz CNAME sto.fit.cvut.cz.\n"
    " |     |        |        |        |  +- courses SPF ip4:147.32.232.128/25, ip4:147.32.232.64/26\n"
    " |     |        |        |        |  \\- fel\n"
    " |     |        |        |        |     +- www A 147.32.80.2\n"
    " |     |        |        |        |     \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |        |        |        \\- fel\n"
    " |     |        |        |           +- www A 147.32.80.2\n"
    " |     |        |        |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |        |        \\- fel\n"
    " |     |        |           +- www A 147.32.80.2\n"
    " |     |        |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     |        \\- fel\n"
    " |     |           +- www A 147.32.80.2\n"
    " |     |           \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " |     \\- fel\n"
    " |        +- www A 147.32.80.2\n"
    " |        \\- www AAAA 1:2:3:4:5:6:7:8\n"
    " \\- au\n" );

  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
