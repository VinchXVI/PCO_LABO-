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
    int maxTry = 10;
    for(int nbTry = 0; nbTry < maxTry; nbTry++){ //Essaie 10x d'envoyer des patients sinon arrête sa routine
        //choisi un hopital aléatoirement, puis lui propose "d'acheter" un patient
        int cost = this->chooseRandomSeller(hospitals)->send(ItemType::PatientSick, 1, getCostPerUnit(ItemType::PatientSick));
        int supplierCost = getEmployeeSalary(getEmployeeThatProduces(ItemType::PatientSick));
        if (cost){
            mutex.lock();
            stocks[ItemType::PatientSick]--;
            this->money += cost - supplierCost;
            this->nbTransfer++;
            mutex.unlock();
            return;
        }
    }
    keepRoutine = false;
}

void Ambulance::run() {
    interface->consoleAppendText(uniqueId, "[START] Ambulance routine");

    while (this->getNumberPatients() > 0 && keepRoutine) { //Tant que son "stock" de patients n'est pas vide et qu'il continue à faire des tranferts
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
