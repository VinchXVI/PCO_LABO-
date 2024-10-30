#include "hospital.h"
#include "costs.h"
#include <iostream>
#include <pcosynchro/pcothread.h>

IWindowInterface* Hospital::interface = nullptr;

Hospital::Hospital(int uniqueId, int fund, int maxBeds)
    : Seller(fund, uniqueId), maxBeds(maxBeds), currentBeds(0), nbHospitalised(0), nbFree(0)
{
    interface->updateFund(uniqueId, fund);
    interface->consoleAppendText(uniqueId, "Hospital Created with " + QString::number(maxBeds) + " beds");
    
    std::vector<ItemType> initialStocks = { ItemType::PatientHealed, ItemType::PatientSick };

    for(const auto& item : initialStocks) {
        stocks[item] = 0;
    }
}

int Hospital::request(ItemType what, int qty){ // what == patientSick (appelé que par clinic)
    int cost = 0;
    mutex.lock();
    if(this->stocks[what] >= qty){
        this->stocks[what] -= qty;
        cost = getCostPerUnit(what) * qty;
        // paiement infirmers à ajouter
        this->money += cost;
    }
    mutex.unlock();
    return cost;
}

void Hospital::freeHealedPatient() { //TODO
    mutex.lock();
    if(this->stocks[ItemType::PatientHealed] > 0){
        this->stocks[ItemType::PatientHealed]--;
        this->money += getCostPerUnit(ItemType::PatientHealed);
        // paiement infirmiers à ajouter (?)
        this->nbFree++;
    }
    mutex.unlock();
}

void Hospital::transferPatientsFromClinic() { //TODO
    int cost;
    mutex.lock();
    if(this->stocks[ItemType::PatientSick] + this->stocks[ItemType::PatientHealed] + 1 <= this->maxBeds){
      cost = chooseRandomSeller(clinics)->request(ItemType::PatientHealed, 1);
      if(cost != 0){
          // paiement infirmiers à ajouter
          this->money -= cost;
          this->stocks[ItemType::PatientHealed]++;
          this->nbHospitalised++;
      }
    }
    mutex.unlock();
}

int Hospital::send(ItemType it, int qty, int bill) { // it == patientSick (appelé que par ambulance)
    int cost = 0;
    mutex.lock();
    if(this->stocks[ItemType::PatientSick] + this->stocks[ItemType::PatientHealed] + qty <= this->maxBeds / 2 and qty * bill <= this->money){ // divisé parr 2 pour éviter de surpeupler de patient malade
        this->stocks[it] += qty;
        cost = bill * qty;
        // paiement infirmiers à ajouter
        this->money -= cost;
        this->nbHospitalised++;
    }
    mutex.unlock();
    return cost;
}

void Hospital::run()
{
    if (clinics.empty()) {
        std::cerr << "You have to give clinics to a hospital before launching is routine" << std::endl;
        return;
    }

    interface->consoleAppendText(uniqueId, "[START] Hospital routine");
    int DayCounter = 0;

    while (this->money > 0) { // Tant qu'il y a de l'argent (TODO)
        transferPatientsFromClinic();

        if(DayCounter % 5 == 0)
            freeHealedPatient();

        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);
        interface->simulateWork(); // Temps d'attente
        ++DayCounter;
    }

    interface->consoleAppendText(uniqueId, "[STOP] Hospital routine");
}

int Hospital::getAmountPaidToWorkers() {
    return nbHospitalised * getEmployeeSalary(EmployeeType::Nurse);
}

int Hospital::getNumberPatients(){
    return stocks[ItemType::PatientSick] + stocks[ItemType::PatientHealed] + nbFree;
}

std::map<ItemType, int> Hospital::getItemsForSale()
{
    return stocks;
}

void Hospital::setClinics(std::vector<Seller*> clinics){
    this->clinics = clinics;

    for (Seller* clinic : clinics) {
        interface->setLink(uniqueId, clinic->getUniqueId());
    }
}

void Hospital::setInterface(IWindowInterface* windowInterface){
    interface = windowInterface;
}
