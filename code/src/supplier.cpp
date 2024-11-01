#include "supplier.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>
#include "utils.h"

IWindowInterface* Supplier::interface = nullptr;

Supplier::Supplier(int uniqueId, int fund, std::vector<ItemType> resourcesSupplied)
    : Seller(fund, uniqueId), resourcesSupplied(resourcesSupplied), nbSupplied(0) 
{
    for (const auto& item : resourcesSupplied) {    
        stocks[item] = 0;    
    }

    interface->consoleAppendText(uniqueId, QString("Supplier Created"));
    interface->updateFund(uniqueId, fund);
}


int Supplier::request(ItemType it, int qty) {
    int cost = 0;
    this->mutex.lock();
    if(qty <= this->stocks[it] and !this->stopRoutine){ // s'il a le matériel en stock et qu'il est toujours en service
        cost = qty * getCostPerUnit(it);
        this->stocks[it] -= qty;
        this->money += cost;
    }
    this->mutex.unlock();
    return cost;
}

void Supplier::run() {
    interface->consoleAppendText(uniqueId, "[START] Supplier routine");
    while (this->money > 0){
        ItemType resourceSupplied = getRandomItemFromStock();
        int supplierCost = getEmployeeSalary(getEmployeeThatProduces(resourceSupplied));

        this->mutex.lock();
        if(this->money - supplierCost >= 0){
            this->money -= supplierCost;
        } else {
            this->stopRoutine = true;
            this->mutex.unlock();
            break; // s'il ne peut plus payer son employé, arrête la routine
        }
        this->mutex.unlock();

        /* Temps aléatoire borné qui simule l'attente du travail fini*/
        interface->simulateWork();

        this->mutex.lock();
        this->stocks[resourceSupplied]++;
        this->mutex.unlock();

        nbSupplied++;

        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);

        if(stop == true) return;
    }
    interface->consoleAppendText(uniqueId, "[STOP] Supplier routine");
}


std::map<ItemType, int> Supplier::getItemsForSale() {
    return stocks;
}

int Supplier::getMaterialCost() {
    int totalCost = 0;
    for (const auto& item : resourcesSupplied) {
        totalCost += getCostPerUnit(item);
    }
    return totalCost;
}

int Supplier::getAmountPaidToWorkers() {
    return nbSupplied * getEmployeeSalary(EmployeeType::Supplier);
}

void Supplier::setInterface(IWindowInterface *windowInterface) {
    interface = windowInterface;
}

std::vector<ItemType> Supplier::getResourcesSupplied() const
{
    return resourcesSupplied;
}

int Supplier::send(ItemType it, int qty, int bill){
    return 0;
}
