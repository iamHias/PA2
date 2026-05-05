#ifndef __PROGTEST__
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <memory>
#endif /* __PROGTEST__ */

struct Company {
    std::string origName;
    std::string origAddr;
    std::string lowerName;
    std::string lowerAddr;
    std::string taxID;
    unsigned int totalIncome = 0;
};

class CVATRegister {
private:
    std::vector<std::shared_ptr<Company>> byName;
    std::vector<std::shared_ptr<Company>> byTax;

    std::vector<unsigned int> lowerHalf;
    std::vector<unsigned int> upperHalf;

    static std::string toLower(std::string str) {
        for (char& c : str) c = std::tolower(static_cast<unsigned char>(c));
        return str;
    }

    static bool cmpName(const std::shared_ptr<Company>& a, const std::shared_ptr<Company>& b) {
        if (a->lowerName != b->lowerName) return a->lowerName < b->lowerName;
        return a->lowerAddr < b->lowerAddr;
    }

    static bool cmpTax(const std::shared_ptr<Company>& a, const std::shared_ptr<Company>& b) {
        return a->taxID < b->taxID;
    }

    std::shared_ptr<Company> createQuery(const std::string& name, const std::string& addr, const std::string& taxID = "") const {
        auto c = std::make_shared<Company>();
        c->lowerName = toLower(name);
        c->lowerAddr = toLower(addr);
        c->taxID = taxID;
        return c;
    }

    std::shared_ptr<Company> findByName(const std::shared_ptr<Company>& q) const {
        auto it = std::lower_bound(byName.begin(), byName.end(), q, cmpName);
        return (it != byName.end() && (*it)->lowerName == q->lowerName && (*it)->lowerAddr == q->lowerAddr) ? *it : nullptr;
    }

    std::shared_ptr<Company> findByTax(const std::shared_ptr<Company>& q) const {
        auto it = std::lower_bound(byTax.begin(), byTax.end(), q, cmpTax);
        return (it != byTax.end() && (*it)->taxID == q->taxID) ? *it : nullptr;
    }

    bool eraseCompany(const std::shared_ptr<Company>& c) {
        if (!c) return false;
        byName.erase(std::lower_bound(byName.begin(), byName.end(), c, cmpName));
        byTax.erase(std::lower_bound(byTax.begin(), byTax.end(), c, cmpTax));
        return true;
    }

    bool processInvoice(const std::shared_ptr<Company>& c, unsigned int amount) {
        if (!c) return false;
        c->totalIncome += amount;
        
        if (upperHalf.empty() || amount >= upperHalf.front()) {
            upperHalf.push_back(amount);
            std::push_heap(upperHalf.begin(), upperHalf.end(), std::greater<unsigned int>());
        } else {
            lowerHalf.push_back(amount);
            std::push_heap(lowerHalf.begin(), lowerHalf.end(), std::less<unsigned int>());
        }

        if (upperHalf.size() > lowerHalf.size() + 1) {
            lowerHalf.push_back(upperHalf.front());
            std::push_heap(lowerHalf.begin(), lowerHalf.end(), std::less<unsigned int>());
            std::pop_heap(upperHalf.begin(), upperHalf.end(), std::greater<unsigned int>());
            upperHalf.pop_back();
        } else if (lowerHalf.size() > upperHalf.size()) {
            upperHalf.push_back(lowerHalf.front());
            std::push_heap(upperHalf.begin(), upperHalf.end(), std::greater<unsigned int>());
            std::pop_heap(lowerHalf.begin(), lowerHalf.end(), std::less<unsigned int>());
            lowerHalf.pop_back();
        }
        return true;
    }

    bool processAudit(const std::shared_ptr<Company>& c, unsigned int& sum) const {
        if (!c) return false;
        sum = c->totalIncome;
        return true;
    }

public:
    CVATRegister() = default;
    ~CVATRegister() = default;

    bool newCompany(const std::string& name, const std::string& addr, const std::string& taxID) {
        auto q = createQuery(name, addr, taxID);
        auto itName = std::lower_bound(byName.begin(), byName.end(), q, cmpName);
        auto itTax = std::lower_bound(byTax.begin(), byTax.end(), q, cmpTax);

        if ((itName != byName.end() && (*itName)->lowerName == q->lowerName && (*itName)->lowerAddr == q->lowerAddr) ||
            (itTax != byTax.end() && (*itTax)->taxID == q->taxID)) {
            return false;
        }

        q->origName = name;
        q->origAddr = addr;
        byName.insert(itName, q);
        byTax.insert(itTax, q);
        return true;
    }

    bool cancelCompany(const std::string& name, const std::string& addr) {
        return eraseCompany(findByName(createQuery(name, addr)));
    }

    bool cancelCompany(const std::string& taxID) {
        return eraseCompany(findByTax(createQuery("", "", taxID)));
    }

    bool invoice(const std::string& taxID, unsigned int amount) {
        return processInvoice(findByTax(createQuery("", "", taxID)), amount);
    }

    bool invoice(const std::string& name, const std::string& addr, unsigned int amount) {
        return processInvoice(findByName(createQuery(name, addr)), amount);
    }

    bool auditCompany(const std::string& name, const std::string& addr, unsigned int& sumIncome) const {
        return processAudit(findByName(createQuery(name, addr)), sumIncome);
    }

    bool auditCompany(const std::string& taxID, unsigned int& sumIncome) const {
        return processAudit(findByTax(createQuery("", "", taxID)), sumIncome);
    }

    bool firstCompany(std::string& name, std::string& addr) const {
        if (byName.empty()) return false;
        name = byName.front()->origName;
        addr = byName.front()->origAddr;
        return true;
    }

    bool nextCompany(std::string& name, std::string& addr) const {
        auto q = createQuery(name, addr);
        auto it = std::lower_bound(byName.begin(), byName.end(), q, cmpName);
        
        if (it == byName.end() || (*it)->lowerName != q->lowerName || (*it)->lowerAddr != q->lowerAddr || ++it == byName.end()) {
            return false;
        }
        
        name = (*it)->origName;
        addr = (*it)->origAddr;
        return true;
    }

    unsigned int medianInvoice() const {
        return upperHalf.empty() ? 0 : upperHalf.front();
    }
};


#ifndef __PROGTEST__
int               main           ()
{
  std::string name, addr;
  unsigned int sumIncome;

  CVATRegister b1;
  assert ( b1 . newCompany ( "ACME", "Thakurova", "666/666" ) );
  assert ( b1 . newCompany ( "ACME", "Kolejni", "666/666/666" ) );
  assert ( b1 . newCompany ( "Dummy", "Thakurova", "123456" ) );
  assert ( b1 . invoice ( "666/666", 2000 ) );
  assert ( b1 . medianInvoice () == 2000 );
  assert ( b1 . invoice ( "666/666/666", 3000 ) );
  assert ( b1 . medianInvoice () == 3000 );
  assert ( b1 . invoice ( "123456", 4000 ) );
  assert ( b1 . medianInvoice () == 3000 );
  assert ( b1 . invoice ( "aCmE", "Kolejni", 5000 ) );
  assert ( b1 . medianInvoice () == 4000 );
  assert ( b1 . auditCompany ( "ACME", "Kolejni", sumIncome ) && sumIncome == 8000 );
  assert ( b1 . auditCompany ( "123456", sumIncome ) && sumIncome == 4000 );
  assert ( b1 . firstCompany ( name, addr ) && name == "ACME" && addr == "Kolejni" );
  assert ( b1 . nextCompany ( name, addr ) && name == "ACME" && addr == "Thakurova" );
  assert ( b1 . nextCompany ( name, addr ) && name == "Dummy" && addr == "Thakurova" );
  assert ( ! b1 . nextCompany ( name, addr ) );
  assert ( b1 . cancelCompany ( "ACME", "KoLeJnI" ) );
  assert ( b1 . medianInvoice () == 4000 );
  assert ( b1 . cancelCompany ( "666/666" ) );
  assert ( b1 . medianInvoice () == 4000 );
  assert ( b1 . invoice ( "123456", 100 ) );
  assert ( b1 . medianInvoice () == 3000 );
  assert ( b1 . invoice ( "123456", 300 ) );
  assert ( b1 . medianInvoice () == 3000 );
  assert ( b1 . invoice ( "123456", 200 ) );
  assert ( b1 . medianInvoice () == 2000 );
  assert ( b1 . invoice ( "123456", 230 ) );
  assert ( b1 . medianInvoice () == 2000 );
  assert ( b1 . invoice ( "123456", 830 ) );
  assert ( b1 . medianInvoice () == 830 );
  assert ( b1 . invoice ( "123456", 1830 ) );
  assert ( b1 . medianInvoice () == 1830 );
  assert ( b1 . invoice ( "123456", 2830 ) );
  assert ( b1 . medianInvoice () == 1830 );
  assert ( b1 . invoice ( "123456", 2830 ) );
  assert ( b1 . medianInvoice () == 2000 );
  assert ( b1 . invoice ( "123456", 3200 ) );
  assert ( b1 . medianInvoice () == 2000 );
  assert ( b1 . firstCompany ( name, addr ) && name == "Dummy" && addr == "Thakurova" );
  assert ( ! b1 . nextCompany ( name, addr ) );
  assert ( b1 . cancelCompany ( "123456" ) );
  assert ( ! b1 . firstCompany ( name, addr ) );

  CVATRegister b2;
  assert ( b2 . newCompany ( "ACME", "Kolejni", "abcdef" ) );
  assert ( b2 . newCompany ( "Dummy", "Kolejni", "123456" ) );
  assert ( ! b2 . newCompany ( "AcMe", "kOlEjNi", "1234" ) );
  assert ( b2 . newCompany ( "Dummy", "Thakurova", "ABCDEF" ) );
  assert ( b2 . medianInvoice () == 0 );
  assert ( b2 . invoice ( "ABCDEF", 1000 ) );
  assert ( b2 . medianInvoice () == 1000 );
  assert ( b2 . invoice ( "abcdef", 2000 ) );
  assert ( b2 . medianInvoice () == 2000 );
  assert ( b2 . invoice ( "aCMe", "kOlEjNi", 3000 ) );
  assert ( b2 . medianInvoice () == 2000 );
  assert ( ! b2 . invoice ( "1234567", 100 ) );
  assert ( ! b2 . invoice ( "ACE", "Kolejni", 100 ) );
  assert ( ! b2 . invoice ( "ACME", "Thakurova", 100 ) );
  assert ( ! b2 . auditCompany ( "1234567", sumIncome ) );
  assert ( ! b2 . auditCompany ( "ACE", "Kolejni", sumIncome ) );
  assert ( ! b2 . auditCompany ( "ACME", "Thakurova", sumIncome ) );
  assert ( ! b2 . cancelCompany ( "1234567" ) );
  assert ( ! b2 . cancelCompany ( "ACE", "Kolejni" ) );
  assert ( ! b2 . cancelCompany ( "ACME", "Thakurova" ) );
  assert ( b2 . cancelCompany ( "abcdef" ) );
  assert ( b2 . medianInvoice () == 2000 );
  assert ( ! b2 . cancelCompany ( "abcdef" ) );
  assert ( b2 . newCompany ( "ACME", "Kolejni", "abcdef" ) );
  assert ( b2 . cancelCompany ( "ACME", "Kolejni" ) );
  assert ( ! b2 . cancelCompany ( "ACME", "Kolejni" ) );

  return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
