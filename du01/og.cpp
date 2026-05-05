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

class IMedianStrategy {
public:
    virtual ~IMedianStrategy() = default;
    virtual void add(unsigned int amount) = 0;
    virtual unsigned int getMedian() const = 0;
};

class TwoHeapMedianStrategy : public IMedianStrategy {
private:
    std::vector<unsigned int> lowerHalf;
    std::vector<unsigned int> upperHalf;

public:
    void add(unsigned int amount) override {
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
    }

    unsigned int getMedian() const override {
        return upperHalf.empty() ? 0 : upperHalf.front();
    }
};

class CompanyRepository {
private:
    std::vector<std::shared_ptr<Company>> byName;
    std::vector<std::shared_ptr<Company>> byTax;

    static std::string toLower(const std::string& str) {
        std::string res = str;
        for (char& c : res) c = std::tolower(static_cast<unsigned char>(c));
        return res;
    }

    static bool cmpName(const std::shared_ptr<Company>& a, const std::shared_ptr<Company>& b) {
        int cmp = a->lowerName.compare(b->lowerName);
        if (cmp != 0) return cmp < 0;
        return a->lowerAddr < b->lowerAddr;
    }

    static bool cmpTax(const std::shared_ptr<Company>& a, const std::shared_ptr<Company>& b) {
        return a->taxID < b->taxID;
    }

public:
    std::shared_ptr<Company> createQuery(const std::string& name, const std::string& addr, const std::string& taxID = "") const {
        auto c = std::make_shared<Company>();
        c->lowerName = toLower(name);
        c->lowerAddr = toLower(addr);
        c->taxID = taxID;
        return c;
    }

    std::shared_ptr<Company> findByNameAndAddr(const std::shared_ptr<Company>& query, decltype(byName)::iterator* outIt = nullptr) {
        auto it = std::lower_bound(byName.begin(), byName.end(), query, cmpName);
        if (outIt) *outIt = it;
        if (it != byName.end() && (*it)->lowerName == query->lowerName && (*it)->lowerAddr == query->lowerAddr) {
            return *it;
        }
        return nullptr;
    }

    std::shared_ptr<Company> findByTaxID(const std::shared_ptr<Company>& query, decltype(byTax)::iterator* outIt = nullptr) {
        auto it = std::lower_bound(byTax.begin(), byTax.end(), query, cmpTax);
        if (outIt) *outIt = it;
        if (it != byTax.end() && (*it)->taxID == query->taxID) {
            return *it;
        }
        return nullptr;
    }

    bool insert(const std::string& name, const std::string& addr, const std::string& taxID) {
        auto query = createQuery(name, addr, taxID);

        decltype(byName)::iterator itName;
        decltype(byTax)::iterator itTax;

        if (findByNameAndAddr(query, &itName) || findByTaxID(query, &itTax)) return false;

        query->origName = name;
        query->origAddr = addr;

        byName.insert(itName, query);
        byTax.insert(itTax, query);
        return true;
    }

    bool erase(std::shared_ptr<Company> comp) {
        if (!comp) return false;

        decltype(byName)::iterator itName;
        decltype(byTax)::iterator itTax;

        findByNameAndAddr(comp, &itName);
        findByTaxID(comp, &itTax);

        if (itName != byName.end()) byName.erase(itName);
        if (itTax != byTax.end()) byTax.erase(itTax);
        return true;
    }

    bool getFirst(std::string& name, std::string& addr) const {
        if (byName.empty()) return false;
        name = byName.front()->origName;
        addr = byName.front()->origAddr;
        return true;
    }

    bool getNext(const std::string& currentName, const std::string& currentAddr, std::string& nextName, std::string& nextAddr) {
        auto query = createQuery(currentName, currentAddr);
        decltype(byName)::iterator itName;

        if (!findByNameAndAddr(query, &itName)) return false;

        if (++itName == byName.end()) return false;

        nextName = (*itName)->origName;
        nextAddr = (*itName)->origAddr;
        return true;
    }
};

class CVATRegister
{
  public:
    CVATRegister() : medianStrategy(std::make_unique<TwoHeapMedianStrategy>()) {}
    ~CVATRegister() = default;

    bool newCompany(const std::string& name, const std::string& addr, const std::string& taxID) {
        return db.insert(name, addr, taxID);
    }

    bool cancelCompany(const std::string& name, const std::string& addr) {
        return db.erase(db.findByNameAndAddr(db.createQuery(name, addr)));
    }

    bool cancelCompany(const std::string& taxID) {
        return db.erase(db.findByTaxID(db.createQuery("", "", taxID)));
    }

    bool invoice(const std::string& taxID, unsigned int amount) {
        auto comp = db.findByTaxID(db.createQuery("", "", taxID));
        if (!comp) return false;
        comp->totalIncome += amount;
        medianStrategy->add(amount);
        return true;
    }

    bool invoice(const std::string& name, const std::string& addr, unsigned int amount) {
        auto comp = db.findByNameAndAddr(db.createQuery(name, addr));
        if (!comp) return false;
        comp->totalIncome += amount;
        medianStrategy->add(amount);
        return true;
    }

    bool auditCompany(const std::string& name, const std::string& addr, unsigned int& sumIncome) const {
        auto comp = const_cast<CompanyRepository&>(db).findByNameAndAddr(db.createQuery(name, addr));
        if (!comp) return false;
        sumIncome = comp->totalIncome;
        return true;
    }

    bool auditCompany(const std::string& taxID, unsigned int& sumIncome) const {
        auto comp = const_cast<CompanyRepository&>(db).findByTaxID(db.createQuery("", "", taxID));
        if (!comp) return false;
        sumIncome = comp->totalIncome;
        return true;
    }

    bool firstCompany(std::string& name, std::string& addr) const {
        return db.getFirst(name, addr);
    }

    bool nextCompany(std::string& name, std::string& addr) const {
        return const_cast<CompanyRepository&>(db).getNext(name, addr, name, addr);
    }

    unsigned int medianInvoice() const {
        return medianStrategy->getMedian();
    }

  private:
    CompanyRepository db;
    std::unique_ptr<IMedianStrategy> medianStrategy;
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
