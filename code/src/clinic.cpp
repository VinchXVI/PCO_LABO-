#include "clinic.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>
#include <iostream>

IWindowInterface* Clinic::interface = nullptr;

Clinic::Clinic(int uniqueId, int fund, std::vector<ItemType> resourcesNeeded)
    : Seller(fund, uniqueId), nbTreated(0), resourcesNeeded(resourcesNeeded)
{
    interface->updateFund(uniqueId, fund);
    interface->consoleAppendText(uniqueId, "Factory created");

    for(const auto& item : resourcesNeeded) {
        stocks[item] = 0;
    }
}

bool Clinic::verifyResources() {
    for (auto item : resourcesNeeded) {
        if (stocks[item] == 0) {
            return false;
        }
    }
    return true;
}

int Clinic::request(ItemType what, int qty){ // what == PatientHealed TODO
    int cost;
    if(this->stocks[what] != 0){
        cost = getCostPerUnit(what);
        this->money += cost;
        this->stocks[what]--;
    } else {
        cost = 0;
    }
    return cost;
}

void Clinic::treatPatient() {
    int employeeCost = getEmployeeSalary(getEmployeeThatProduces(ItemType::PatientSick));
    this->money -= employeeCost;

    //Temps simulant un traitement 
    interface->simulateWork();

    this->stocks[ItemType::PatientSick]--;
    this->stocks[ItemType::PatientHealed]++;
    this->nbTreated++;
    
    interface->consoleAppendText(uniqueId, "Clinic have healed a new patient");
}

void Clinic::orderResources() { // commande une ressource une par une (à changer ?) TODO
    int cost = 0;
    for(auto item : resourcesNeeded){
       if(this->stocks[item] == 0){
           if(item == ItemType::PatientSick){
               //chooseRandomSeller(hospitals);
               cost = chooseRandomSeller(hospitals)->request(item, 1);
           } else {
               for(Seller* supplier : suppliers){
                   cost = supplier->request(item, 1);
                   if (!cost)
                       break;
               }
           }
           if(cost != 0){
               this->money -= cost;
               this->stocks[item]++;
           }
       }
    }
}

void Clinic::run() {
    if (hospitals.empty() || suppliers.empty()) {
        std::cerr << "You have to give to hospitals and suppliers to run a clinic" << std::endl;
        return;
    }
    interface->consoleAppendText(uniqueId, "[START] Factory routine");

    while (this->money > 0) { // tant que la clinic a de l'argent (TODO)
        
        if (verifyResources()) {
            treatPatient();
        } else {
            orderResources();
        }
       
        interface->simulateWork();

        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);
    }
    interface->consoleAppendText(uniqueId, "[STOP] Factory routine");
}


void Clinic::setHospitalsAndSuppliers(std::vector<Seller*> hospitals, std::vector<Seller*> suppliers) {
    this->hospitals = hospitals;
    this->suppliers = suppliers;

    for (Seller* hospital : hospitals) {
        interface->setLink(uniqueId, hospital->getUniqueId());
    }
    for (Seller* supplier : suppliers) {
        interface->setLink(uniqueId, supplier->getUniqueId());
    }
}

int Clinic::getTreatmentCost() {
    return 0;
}

int Clinic::getWaitingPatients() {
    return stocks[ItemType::PatientSick];
}

int Clinic::getNumberPatients(){
    return stocks[ItemType::PatientSick] + stocks[ItemType::PatientHealed];
}

int Clinic::send(ItemType it, int qty, int bill){
    return 0;
}

int Clinic::getAmountPaidToWorkers() {
    return nbTreated * getEmployeeSalary(getEmployeeThatProduces(ItemType::PatientHealed));
}

void Clinic::setInterface(IWindowInterface *windowInterface) {
    interface = windowInterface;
}

std::map<ItemType, int> Clinic::getItemsForSale() {
    return stocks;
}


Pulmonology::Pulmonology(int uniqueId, int fund) :
    Clinic::Clinic(uniqueId, fund, {ItemType::PatientSick, ItemType::Pill, ItemType::Thermometer}) {}

Cardiology::Cardiology(int uniqueId, int fund) :
    Clinic::Clinic(uniqueId, fund, {ItemType::PatientSick, ItemType::Syringe, ItemType::Stethoscope}) {}

Neurology::Neurology(int uniqueId, int fund) :
    Clinic::Clinic(uniqueId, fund, {ItemType::PatientSick, ItemType::Pill, ItemType::Scalpel}) {}
