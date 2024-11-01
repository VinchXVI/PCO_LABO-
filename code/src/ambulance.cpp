#include "ambulance.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>
#include "utils.h"

IWindowInterface* Ambulance::interface = nullptr;

Ambulance::Ambulance(int uniqueId, int fund, std::vector<ItemType> resourcesSupplied, std::map<ItemType, int> initialStocks)
    : Seller(fund, uniqueId), resourcesSupplied(resourcesSupplied), nbTransfer(0) 
{
    interface->consoleAppendText(uniqueId, QString("Ambulance Created"));

    for (const auto& item : resourcesSupplied) {
        if (initialStocks.find(item) != initialStocks.end()) {
            stocks[item] = initialStocks[item];
        } else {
            stocks[item] = 0;
        }
    }

    interface->updateFund(uniqueId, fund);
}

void Ambulance::sendPatient(){
    const int maxTry = 20; // nombre d'essais pour livrer un patient
    this->mutex.lock();
    for(int nbTry = 0; nbTry < maxTry; nbTry++){
        //choisi un hopital aléatoirement, puis lui propose "d'acheter" un patient
        int supplierCost = getEmployeeSalary(getEmployeeThatProduces(ItemType::PatientSick));
        int cost = this->chooseRandomSeller(hospitals)->send(ItemType::PatientSick, 1, getCostPerUnit(ItemType::PatientSick));

        if (cost > 0){
            this->stocks[ItemType::PatientSick]--;
            this->money += cost - supplierCost;
            this->nbTransfer++;
            this->mutex.unlock();
            return;
        }

       interface->simulateWork(); // attent un moment avant de réssayer
    }
    // Arrive à ce point s'il n'a pas réussi à transférer un patient plusieurs fois
    this->mutex.unlock();
    this->stopRoutine = true;
    return;
}

void Ambulance::run() {
    interface->consoleAppendText(uniqueId, "[START] Ambulance routine");

    while (this->getNumberPatients() > 0 and !this->stopRoutine) { //Tant que son "stock" de patients n'est pas vide et qu'il continue à faire des tranferts
        sendPatient();
        
        interface->simulateWork();

        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);

        if(stop == true) return;
    }

    interface->consoleAppendText(uniqueId, "[STOP] Ambulance routine");
}

std::map<ItemType, int> Ambulance::getItemsForSale() {
    return stocks;
}

int Ambulance::getMaterialCost() {
    int totalCost = 0;
    for (const auto& item : resourcesSupplied) {
        totalCost += getCostPerUnit(item);
    }
    return totalCost;
}

int Ambulance::getAmountPaidToWorkers() {
    return nbTransfer * getEmployeeSalary(EmployeeType::Supplier);
}

int Ambulance::getNumberPatients(){
    return stocks[ItemType::PatientSick];
}

void Ambulance::setInterface(IWindowInterface *windowInterface) {
    interface = windowInterface;
}


void Ambulance::setHospitals(std::vector<Seller*> hospitals){
    this->hospitals = hospitals;

    for (Seller* hospital : hospitals) {
        interface->setLink(uniqueId, hospital->getUniqueId());
    }
}

int Ambulance::send(ItemType it, int qty, int bill) {
    return 0;
}


int Ambulance::request(ItemType what, int qty){
    return 0;
}

std::vector<ItemType> Ambulance::getResourcesSupplied() const
{
    return resourcesSupplied;
}
