/**
 * @mainpage Domácí úloha 1: Kontrolní hlášení (BI-PA2)
 * @section zadani_sec Zadání 
 * Úkolem je realizovat třídu CVATRegister, která bude implementovat databázi kontrolních hlášení DPH.
 * Pro plánované důslednější potírání daňových úniků je zaveden systém kontrolních hlášení. V databázi jsou zavedené jednotlivé firmy, a do databáze jsou zaznamenávané jednotlivé vydané faktury, které daná firma vydala. Firmy lze do databáze zadávat a lze je rušit. Firma je identifikována svým jménem, adresou a daňovým identifikátorem (id). Daňový identifikátor je unikátní přes celou databázi. Jména a adresy se mohou opakovat, ale dvojice (jméno, adresa) je opět v databázi unikátní. Tedy v databázi může být mnoho firem ACME, mnoho firem může mít adresu Praha, ale firma ACME bydlící sídlící ve městě Praha může být v databázi pouze jedna. Při porovnávání daňových identifikátorů rozlišujeme malá a velká písmena, u jmen a adres naopak nerozlišujeme malá a velká písmena.
 * Veřejné rozhraní je uvedeno níže. Obsahuje následující:
 ** - Konstruktor bez parametrů. Tento konstruktor inicializuje instanci třídy tak, že vzniklá instance je zatím prázdná (neobsahuje žádné záznamy).
 ** - Destruktor. Uvolňuje prostředky, které instance alokovala.
 ** - Metoda newCompany(name, addr, id) přidá do existující databáze další záznam. Parametry name a addr reprezentují jméno a adresu, parametr id udává daňový identifikátor. Metoda vrací hodnotu true, pokud byl záznam přidán, nebo hodnotu false, pokud přidán nebyl (protože již v databázi existoval záznam se stejným jménem a adresou, nebo záznam se stejným id).
 ** - Metody cancelCompany (name, addr) / cancelCompany (id) odstraní záznam z databáze. Parametrem je jednoznačná identifikace pomocí jména a adresy (první varianta) nebo pomocí daňového identifikátoru (druhá varianta). Pokud byl záznam skutečně odstraněn, vrátí metoda hodnotu true. Pokud záznam neodstraní (protože neexistovala firma s touto identifikací), vrátí metoda hodnotu false.
 ** - Metody invoice (name, addr, amount) / invoice (id, amount) zaznamenají příjem ve výši amount. Varianty jsou dvě - firma je buď identifikována svým jménem a adresou, nebo daňovým identifikátorem. Pokud metoda uspěje, vrací true, pro neúspěch vrací false (neexistující firma).
 ** - Metoda auditCompany ( name, addr, sum ) / auditCompany ( id, sum ) vyhledá součet příjmů pro firmu se zadaným jménem a adresou nebo firmu zadanou daňovým identifikátorem. Nalezený součet uloží do výstupního parametru sum. Metoda vrátí true pro úspěch, false pro selhání (neexistující firma).
 ** - Metoda medianInvoice () vyhledá medián hodnoty faktury. Do vypočteného mediánu se započtou všechny úspěšně zpracované faktury zadané voláním invoice. Tedy nezapočítávají se faktury, které nešlo přiřadit (volání invoice selhalo), ale započítávají se všechny dosud registrované faktury, tedy při výmazu firmy se neodstraňují její faktury z výpočtu mediánu. Pokud je v systému zadaný sudý počet faktur, vezme se vyšší ze dvou prostředních hodnot. Pokud systém zatím nezpracoval žádnou fakturu, bude vrácena hodnota 0.
 ** - Metody firstCompany ( name, addr ) / nextCompany ( name, addr ) slouží k procházení existujícího seznamu firem v naší databázi. Firmy jsou procházené v abecedním pořadí podle jejich jména. Pokud mají dvě firmy stejná jména, rozhoduje o pořadí jejich adresa. Metoda firstCompany nalezne první firmu. Pokud je seznam firem prázdný, vrátí metoda hodnotu false. V opačném případě vrátí metoda hodnotu true a vyplní výstupní parametry name a addr. Metoda nextCompany funguje obdobně, nalezne další firmu, která v seznamu následuje za firmou určenou parametry. Pokud za name a addr již v seznamu není další firma, metoda vrací hodnotu false. V opačném případě metoda vrátí true a přepíše parametry name a addr jménem a adresou následující firmy.
 * @section note Poznámky
 * Odevzdávejte soubor, který obsahuje implementovanou třídu CVATRegister. Třída musí splňovat veřejné rozhraní podle ukázky - pokud Vámi odevzdané řešení nebude obsahovat popsané rozhraní, dojde k chybě při kompilaci. Do třídy si ale můžete doplnit další metody (veřejné nebo i privátní) a členské proměnné. Odevzdávaný soubor musí obsahovat jak deklaraci třídy (popis rozhraní) tak i definice metod, konstruktoru a destruktoru. Je jedno, zda jsou metody implementované inline nebo odděleně. Odevzdávaný soubor nesmí kromě implementace třídy CVATRegister obsahovat nic jiného, zejména ne vkládání hlavičkových souborů a funkci main (funkce main a vkládání hlavičkových souborů může zůstat, ale pouze obalené direktivami podmíněného překladu). Za základ implementace použijte přiložený zdrojový soubor. <br>
 * Třída je testovaná v omezeném prostředí, kde je limitovaná dostupná paměť (dostačuje k uložení seznamu) a je omezena dobou běhu. Implementovaná třída se nemusí zabývat kopírujícím konstruktorem ani přetěžováním operátoru =. V této úloze ProgTest neprovádí testy této funkčnosti. <br>
 * Implementace třídy musí být efektivní z hlediska nároků na čas i nároků na paměť. Jednoduché lineární řešení nestačí (pro testovací data vyžaduje čas přes 5 minut). Předpokládejte, že vytvoření a likvidace firmy jsou řádově méně časté než ostatní operace, tedy zde je lineární složitost akceptovatelná. Častá jsou volání invoice a auditCompany, jejich časová složitost musí být lepší než lineární (např. logaritmická nebo amortizovaná konstantní). Dále, v povinných testech se metoda medianInvoice volá málo často, tedy nemusí být příliš efektivní (pro úspěch v povinných testech stačí složitost lineární nebo n log n, pro bonusový test je potřeba složitost lepší než lineární). <br>
 * Pro uložení hodnot alokujte pole dynamicky případně použijte STL. Pozor Pokud budete pole alokovat ve vlastní režii, zvolte počáteční velikost malou (např. tisíc prvků) a velikost zvětšujte/zmenšujte podle potřeby. Při zaplnění pole není vhodné alokovat nové pole větší pouze o jednu hodnotu, takový postup má obrovskou režii na kopírování obsahu. Je rozumné pole rozšiřovat s krokem řádově tisíců prvků, nebo geometrickou řadou s kvocientem ~1.5 až 2. <br>
 * Pokud budete používat STL, nemusíte se starat o problémy s alokací. Pozor - k dispozici máte pouze část STL (viz hlavičkové soubory v přiložené ukázce). Tedy například kontejnery map / unordered_map / set / unordered_set / ... nejsou k dispozici. <br>
 * [aka] Knihovny: string, vector, list, algorithm, memory <br>
 * V přiloženém zdrojovém kódu jsou obsažené základní testy. Tyto testy zdaleka nepokrývají všechny situace, pro odladění třídy je budete muset rozšířit. Upozorňujeme, že testy obsažené v odevzdaných zdrojových kódech považujeme za nedílnou součást Vašeho řešení. Pokud v odevzdaném řešení necháte cizí testy, může být práce vyhodnocena jako opsaná. <br>

 */


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

/**
 * @brief Represents a single company record in the VAT register.
 * * Stores both the original casing for output purposes and the lowercased
 * versions to optimize case-insensitive binary searches.
 */
struct Company {
    std::string origName;     ///< Original company name with og case.
    std::string origAddr;     ///< Original company address with og case.
    std::string lowerName;    ///< Lowercased name for fast comparisons.
    std::string lowerAddr;    ///< Lowercased address for fast comparisons.
    std::string taxID;        ///< Unique tax identifier (case-sensitive).
    unsigned int totalIncome = 0; ///< Sum of all processed invoices for this company.
};

/**
 * @brief Interface for median calculation strategies.
 * * Allows for dependency injection and swapping of median calculation 
 * algorithms (e.g., linear array vs. two-heap approach).
 */
class IMedianStrategy {
public:
    virtual ~IMedianStrategy() = default;

    /**
     * @brief Adds a new invoice amount to the dataset.
     * @param amount The value of the processed invoice.
     */
    virtual void add(unsigned int amount) = 0;

    /**
     * @brief Retrieves the current median of all processed invoices.
     * @return The median value, or 0 if no invoices exist.
     */
    virtual unsigned int getMedian() const = 0;
};

/**
 * @brief High-performance median calculator using the Two-Heap algorithm.
 * * Maintains a max-heap for the lower half of data and a min-heap for the 
 * upper half. Provides O(log N) insertion and O(1) median retrieval.
 */
class TwoHeapMedianStrategy : public IMedianStrategy {
private:
    std::vector<unsigned int> lowerHalf; ///< Max-heap containing the smaller half of values.
    std::vector<unsigned int> upperHalf; ///< Min-heap containing the larger half of values.

public:
    void add(unsigned int amount) override {
        // Insert into the appropriate heap
        if (upperHalf.empty() || amount >= upperHalf.front()) {
            upperHalf.push_back(amount);
            std::push_heap(upperHalf.begin(), upperHalf.end(), std::greater<unsigned int>());
        } else {
            lowerHalf.push_back(amount);
            std::push_heap(lowerHalf.begin(), lowerHalf.end(), std::less<unsigned int>());
        }

        // Rebalance heaps: upperHalf can be at most 1 element larger than lowerHalf
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
        // If even total elements, upperHalf holds the higher of the two middle values
        return upperHalf.empty() ? 0 : upperHalf.front();
    }
};

/**
 * @brief Database engine for storing and querying Company records.
 * * Uses two sorted vectors of shared pointers to maintain O(log N) lookup 
 * times without duplicating the underlying totalIncome data.
 */
class CompanyRepository {
private:
    std::vector<std::shared_ptr<Company>> byName; ///< Sorted by lowerName, then lowerAddr.
    std::vector<std::shared_ptr<Company>> byTax;  ///< Sorted by taxID.

    /**
     * @brief Utility to convert a string to lowercase.
     * @param str The input string.
     * @return A new lowercased string.
     */
    static std::string toLower(const std::string& str) {
        std::string res = str;
        for (char& c : res) c = std::tolower(static_cast<unsigned char>(c));
        return res;
    }

    /**
     * @brief Comparator for sorting companies by Name and Address (case-insensitive).
     */
    static bool cmpName(const std::shared_ptr<Company>& a, const std::shared_ptr<Company>& b) {
        int cmp = a->lowerName.compare(b->lowerName);
        if (cmp != 0) return cmp < 0;
        return a->lowerAddr < b->lowerAddr;
    }

    /**
     * @brief Comparator for sorting companies by Tax ID (case-sensitive).
     */
    static bool cmpTax(const std::shared_ptr<Company>& a, const std::shared_ptr<Company>& b) {
        return a->taxID < b->taxID;
    }

public:
    /**
     * @brief Creates a dummy Company object used solely for binary search queries.
     */
    std::shared_ptr<Company> createQuery(const std::string& name, const std::string& addr, const std::string& taxID = "") const {
        auto c = std::make_shared<Company>();
        c->lowerName = toLower(name);
        c->lowerAddr = toLower(addr);
        c->taxID = taxID;
        return c;
    }

    /**
     * @brief Finds a company using its Name and Address via binary search.
     * @param query A dummy company object populated with search terms.
     * @param outIt Optional pointer to store the resulting vector iterator.
     * @return Shared pointer to the found Company, or nullptr if not found.
     */
    std::shared_ptr<Company> findByNameAndAddr(const std::shared_ptr<Company>& query, decltype(byName)::iterator* outIt = nullptr) {
        auto it = std::lower_bound(byName.begin(), byName.end(), query, cmpName);
        if (outIt) *outIt = it;
        if (it != byName.end() && (*it)->lowerName == query->lowerName && (*it)->lowerAddr == query->lowerAddr) {
            return *it;
        }
        return nullptr;
    }

    /**
     * @brief Finds a company using its Tax ID via binary search.
     */
    std::shared_ptr<Company> findByTaxID(const std::shared_ptr<Company>& query, decltype(byTax)::iterator* outIt = nullptr) {
        auto it = std::lower_bound(byTax.begin(), byTax.end(), query, cmpTax);
        if (outIt) *outIt = it;
        if (it != byTax.end() && (*it)->taxID == query->taxID) {
            return *it;
        }
        return nullptr;
    }

    /**
     * @brief Inserts a new company into both indexes.
     * @return True if successfully inserted, False if a duplicate exists.
     */
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

    /**
     * @brief Removes a company from both indexes.
     * @param comp Shared pointer to the company to remove.
     * @return True if successfully removed, False if invalid pointer.
     */
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

    /**
     * @brief Fetches the first company in alphabetical order.
     */
    bool getFirst(std::string& name, std::string& addr) const {
        if (byName.empty()) return false;
        name = byName.front()->origName;
        addr = byName.front()->origAddr;
        return true;
    }

    /**
     * @brief Fetches the next company in alphabetical order following the provided details.
     */
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

/**
 * @brief Main API for the VAT registration system.
 * * Satisfies the public interface requirements dictated by the assignment.
 */
class CVATRegister
{
  public:
    CVATRegister() : medianStrategy(std::make_unique<TwoHeapMedianStrategy>()) {}
    ~CVATRegister() = default;

    /**
     * @brief Registers a new company.
     * @return True on success, False if the company (by ID or Name/Addr) already exists.
     */
    bool newCompany(const std::string& name, const std::string& addr, const std::string& taxID) {
        return db.insert(name, addr, taxID);
    }

    /**
     * @brief Cancels a company using its Name and Address.
     * @return True if successfully removed, False if it does not exist.
     */
    bool cancelCompany(const std::string& name, const std::string& addr) {
        return db.erase(db.findByNameAndAddr(db.createQuery(name, addr)));
    }

    /**
     * @brief Cancels a company using its Tax ID.
     * @return True if successfully removed, False if it does not exist.
     */
    bool cancelCompany(const std::string& taxID) {
        return db.erase(db.findByTaxID(db.createQuery("", "", taxID)));
    }

    /**
     * @brief Processes an invoice for a company identified by Tax ID.
     * @param taxID The unique tax identifier.
     * @param amount The value of the invoice.
     * @return True if company exists and invoice processed, False otherwise.
     */
    bool invoice(const std::string& taxID, unsigned int amount) {
        auto comp = db.findByTaxID(db.createQuery("", "", taxID));
        if (!comp) return false;
        comp->totalIncome += amount;
        medianStrategy->add(amount);
        return true;
    }

    /**
     * @brief Processes an invoice for a company identified by Name and Address.
     */
    bool invoice(const std::string& name, const std::string& addr, unsigned int amount) {
        auto comp = db.findByNameAndAddr(db.createQuery(name, addr));
        if (!comp) return false;
        comp->totalIncome += amount;
        medianStrategy->add(amount);
        return true;
    }

    /**
     * @brief Queries the total income for a company (by Name and Address).
     * @param sumIncome Output parameter populated with the total income.
     * @return True on success, False if the company does not exist.
     */
    bool auditCompany(const std::string& name, const std::string& addr, unsigned int& sumIncome) const {
        auto comp = const_cast<CompanyRepository&>(db).findByNameAndAddr(db.createQuery(name, addr));
        if (!comp) return false;
        sumIncome = comp->totalIncome;
        return true;
    }

    /**
     * @brief Queries the total income for a company (by Tax ID).
     */
    bool auditCompany(const std::string& taxID, unsigned int& sumIncome) const {
        auto comp = const_cast<CompanyRepository&>(db).findByTaxID(db.createQuery("", "", taxID));
        if (!comp) return false;
        sumIncome = comp->totalIncome;
        return true;
    }

    /**
     * @brief Retrieves the first registered company in alphabetical order.
     */
    bool firstCompany(std::string& name, std::string& addr) const {
        return db.getFirst(name, addr);
    }

    /**
     * @brief Retrieves the alphabetically subsequent company based on the current name/address.
     */
    bool nextCompany(std::string& name, std::string& addr) const {
        return const_cast<CompanyRepository&>(db).getNext(name, addr, name, addr);
    }

    /**
     * @brief Returns the median value of all successfully processed invoices.
     */
    unsigned int medianInvoice() const {
        return medianStrategy->getMedian();
    }

  private:
    CompanyRepository db; ///< The database index manager.
    std::unique_ptr<IMedianStrategy> medianStrategy; ///< The strategy used to calculate medians.
};

#ifndef __PROGTEST__

#endif /* __PROGTEST__ */