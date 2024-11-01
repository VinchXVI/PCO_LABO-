#include "clinic.h"
#include "costs.h"
#include <pcosynchro/pcothread.h>
#include <iostream>
#include "utils.h"

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
    this->mutex.lock();
    if(this->stocks[what] > 0){
        this->stocks[what]--;
        cost = getCostPerUnit(what) * qty;
        this->money += cost;
    }
    this->mutex.unlock();
    return cost;
}

void Clinic::treatPatient() {
    int employeeCost = getEmployeeSalary(getEmployeeThatProduces(ItemType::PatientHealed));

    this->mutex.lock();
    if(this->money - employeeCost >= 0){
        this->money -= employeeCost;
    } else { // Si la clinique n'a plus les fonds pour traiter des patients, stop sa routine
        this->stopRoutine = true;
        this->mutex.unlock();
        return;
    }
    this->mutex.unlock();

    //Temps simulant un traitement 
    interface->simulateWork();

    mutex.lock();
    for(ItemType item : resourcesNeeded){
        this->stocks[item]--;
    }
    this->stocks[ItemType::PatientHealed]++;
    this->nbTreated++;
    mutex.unlock();
    
    interface->consoleAppendText(uniqueId, "Clinic have healed a new patient");
}

void Clinic::orderResources() { // commande une ressource une par une (à changer ?) TODO
    int cost, qty;
    bool transactionDone = false;

    //vérification si les fournisseur sont toujours actifs
    bool suppliersStillRunning = false;
    for(Seller* supplier : suppliers){
        if(!supplier->isStopped())
            suppliersStillRunning = true;
    }

    this->mutex.lock();
        for(ItemType item : this->resourcesNeeded) {
            qty = 0;
            cost = 0;

            if(this->stocks[item] == 0) {
                switch (item) {
                // Tansfert de patient depuis hôpitall
                case ItemType::PatientSick:
                    qty = 1;
                    if(this->money - qty * getCostPerUnit(item) >= 0) { // si fonds suffisants
                        cost = chooseRandomSeller(hospitals)->request(item, qty);
                    }
                break;

                // Commande de matériel médicall
                case ItemType::Pill:
                case ItemType::Scalpel:
                case ItemType::Stethoscope:
                case ItemType::Syringe:
                case ItemType::Thermometer:
                default:
                    qty = 5; // commande en lot de 5
                    if(this->money - qty * getCostPerUnit(item) >= 0 and suppliersStillRunning) { // si fonds suffisants
                        for(Seller* supplier : suppliers) { // cherche chaque fournisseur
                            cost = supplier->request(item, qty);
                            if(cost != 0) {
                                break; // Arrête de chercher dès qu'on a trouvé un fournisseur
                            }
                        }
                    }
                break;
                } // fin du switch

                // Mise à jour des stocks et des fonds si la transaction a réussi
                if(cost != 0) {
                    this->stocks[item] += qty;
                    this->money -= cost;
                    transactionDone = true;
                }
            } // fin de vérification des stocks d'un item
        } // fin de boucle for chaque item


    if(!transactionDone and !suppliersStillRunning){ // Si aucune transaction a pu être effectuée et les que les fournisseur ne sont plus actifs, stopper le programme
        stopRoutine = true;
    }
    this->mutex.unlock();
}


void Clinic::run() {
    if (hospitals.empty() || suppliers.empty()) {
        std::cerr << "You have to give to hospitals and suppliers to run a clinic" << std::endl;
        return;
    }
    interface->consoleAppendText(uniqueId, "[START] Factory routine");

    while (this->money > 0 and !stopRoutine) { // tant que la clinic a de l'argent pour s'approvisionner et traiter les patients
        
        if (verifyResources()) {
            treatPatient();
        } else {
            orderResources();
        }
       
        interface->simulateWork();

        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);

        if(stop == true) return;
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
