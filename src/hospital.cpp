#include "hospital.h"
#include "costs.h"
#include <iostream>
#include <pcosynchro/pcothread.h>
#include "utils.h"

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
    this->mutex.lock();
    if(this->stocks[what] >= qty){
        this->stocks[what] -= qty;
        cost = getCostPerUnit(what) * qty;
        this->money += cost;
    }
    this->mutex.unlock();
    return cost;
}

void Hospital::freeHealedPatient() {
    this->mutex.lock();
    if(this->stocks[ItemType::PatientHealed] > 0){
        this->stocks[ItemType::PatientHealed]--;
        this->nbFree++;
    }
    this->mutex.unlock();
}

void Hospital::transferPatientsFromClinic() {

    //  Check si les cliniques sont toujours actives
    bool clinicStillRunning = false;
    for(Seller* clinic : clinics){
        if(!clinic->isStopped())
            clinicStillRunning = true;
    }
    if(!clinicStillRunning){
        this->stopRoutine = true;
        return;
    }

    this->mutex.lock();
    if(this->stocks[ItemType::PatientSick] + this->stocks[ItemType::PatientHealed] + 1 <= this->maxBeds            // s'il a assez de lits
            and getCostPerUnit(ItemType::PatientHealed) + getEmployeeSalary(EmployeeType::Nurse) <= this->money ){ // et s'il a les fonds suffisants
      int cost = chooseRandomSeller(clinics)->request(ItemType::PatientHealed, 1);
      if(cost != 0){
          this->money -= (cost + getEmployeeSalary(EmployeeType::Nurse));
          this->stocks[ItemType::PatientHealed]++;
          this->nbHospitalised++;
      }
    }
    this->mutex.unlock();
}

int Hospital::send(ItemType it, int qty, int bill) { // it == patientSick (appelé que par ambulance)
    int cost = 0;
    this->mutex.lock();
    if(this->stocks[ItemType::PatientSick] + this->stocks[ItemType::PatientHealed] + qty <= this->maxBeds // s'il a assez de lits
            and (qty * bill) +  getEmployeeSalary(EmployeeType::Nurse) <= this->money                     // s'il a les fonds suffisants
            and !stopRoutine){                                                                            // s'il est toujours actif
        this->stocks[it] += qty;
        cost = (bill * qty);
        this->money -= (cost + getEmployeeSalary(EmployeeType::Nurse));
        this->nbHospitalised++;
    }
    this->mutex.unlock();
    return cost;
}

void Hospital::run() {
    if (clinics.empty()) {
        std::cerr << "You have to give clinics to a hospital before launching is routine" << std::endl;
        return;
    }

    interface->consoleAppendText(uniqueId, "[START] Hospital routine");
    int DayCounter = 0;

    while (this->money > 0 and !stopRoutine and !stopProgram) { // tant qu'il a l'argent et que les cliniques sont actives
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
