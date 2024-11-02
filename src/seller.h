#ifndef SELLER_H
#define SELLER_H

#include <QString>
#include <QStringBuilder>
#include <map>
#include <vector>
#include "costs.h"

#include <pcosynchro/pcomutex.h>

enum class ItemType {
    PatientSick, PatientHealed, Syringe, Pill, Scalpel, Thermometer, Stethoscope, Nothing
};

int getCostPerUnit(ItemType item);
QString getItemName(ItemType item);

enum class EmployeeType {Supplier, Nurse, Doctor};

EmployeeType getEmployeeThatProduces(ItemType item);
int getEmployeeSalary(EmployeeType employee);

class Seller {
public:
    /**
     * @brief Seller
     * @param money money money !
     */
    Seller(int money, int uniqueId) : money(money), uniqueId(uniqueId) {}

    /**
     * @brief getItemsForSale
     * @return The list of items for sale
     */
    virtual std::map<ItemType, int> getItemsForSale() = 0;

    /**
     * @brief Fonction permettant de proposer des ressources au vendeur
     * @param what Le type de resource
     * @param qty Nombre de ressources
     * @param bill Le coût de la transaction
     * @return La quantité acceptée ou la facture (peu dépendre de votre logique) et 0 si la transaction n'est pas acceptée.
     */
    virtual int send(ItemType what, int qty, int bill) = 0;

    /**
     * @brief Fonction permettant d'acheter des ressources au vendeur
     * @param what Le type de resource à acheter
     * @param qty Nombre de ressources voulant être achetées
     * @return La facture : côut de la resource * le nombre, 0 si indisponible
     */
    virtual int request(ItemType what, int qty) = 0;

    /**
     * @brief chooseRandomSeller
     * @param sellers
     * @return Returns a random seller from the sellers vector
     */
    static Seller* chooseRandomSeller(std::vector<Seller*>& sellers);

    /**
     * @brief getRandomItemFromStock
     * @return Returns a random ItemType from the stocks
     */
    ItemType getRandomItemFromStock() {
        if (stocks.empty()) {
            throw std::runtime_error("Stock is empty.");
        }

        auto it = stocks.begin();
        std::advance(it, rand() % stocks.size());

        return it->first;
    }

    /**
     * @brief Chooses a random item type from an items for sale map
     * @param itemsForSale
     * @return Returns the item type
     */
    static ItemType chooseRandomItem(std::map<ItemType, int>& itemsForSale);

    int getFund() { return money; }

    int getUniqueId() { return uniqueId; }

    bool isStopped() { return stopRoutine; } // Ajout supplémentaire^

protected:
    /**
     * @brief stocks : Type, Quantité
     */
    std::map<ItemType, int> stocks;
    int money;
    int uniqueId;
    PcoMutex mutex; // Ajout supplémentaire
    bool stopRoutine = false; // Ajout supplémentaire
};

#endif // SELLER_H
