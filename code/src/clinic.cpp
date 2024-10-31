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

int Clinic::request(ItemType what, int qty){ // what == PatientHealed (appelé que par hopital) TODO
    int cost = 0;
    if(this->stocks[what] != 0){
        mutex.lock();
        this->stocks[what]--;
        cost = getCostPerUnit(what) * qty;
        this->money += cost;
        mutex.unlock();
    }
    return cost;
}

void Clinic::treatPatient() {
    int employeeCost = getEmployeeSalary(getEmployeeThatProduces(ItemType::PatientHealed));

    if(this->money - employeeCost >= 0){
        mutex.lock();
        this->money -= employeeCost;
        mutex.unlock();
    } else {
        keepRoutine = false;
        return;
    }

    //Temps simulant un traitement 
    interface->simulateWork();

    mutex.lock();
    this->stocks[ItemType::PatientSick]--;
    this->stocks[ItemType::PatientHealed]++;
    this->nbTreated++;
    mutex.unlock();
    
    interface->consoleAppendText(uniqueId, "Clinic have healed a new patient");
}

void Clinic::orderResources() { // commande une ressource une par une (à changer ?) TODO
    int cost, qty;

    for(ItemType item : this->resourcesNeeded) {
        qty = 0;
        cost = 0;

        if(this->stocks[item] == 0) {
            switch (item) {
            case ItemType::PatientSick:
                qty = 1;
                if(this->money - qty * getCostPerUnit(item) >= 0) { // si fonds suffisants
                    mutex.lock();
                    cost = chooseRandomSeller(hospitals)->request(item, qty);
                    mutex.unlock();
                }
                break;

            case ItemType::Pill:
            case ItemType::Scalpel:
            case ItemType::Stethoscope:
            case ItemType::Syringe:
            case ItemType::Thermometer:
                qty = 5; // commande en lot de 5
                if(this->money - qty * getCostPerUnit(item) >= 0) { // si fonds suffisants
                    for(Seller* supplier : suppliers) { // cherche chaque fournisseur
                        mutex.lock();
                        cost = supplier->request(item, qty);
                        mutex.unlock();
                        if(cost) {
                            break; // sort dès qu'on a trouvé un fournisseur
                        }
                    }
                }
                break;

            default:
                break;
            } // fin du switch

            // Mise à jour des stocks et des fonds si la transaction a réussi
            if(cost) {
                mutex.lock();
                this->stocks[item] += qty;
                this->money -= cost;
                mutex.unlock();
            }
        } // fin de vérification des stocks
    } // fin de boucle for
}


void Clinic::run() {
    if (hospitals.empty() || suppliers.empty()) {
        std::cerr << "You have to give to hospitals and suppliers to run a clinic" << std::endl;
        return;
    }
    interface->consoleAppendText(uniqueId, "[START] Factory routine");

    while (this->money > 0 && keepRoutine) { // tant que la clinic a de l'argent pour s'approvisionner et traiter les patients
        
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
