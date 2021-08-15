
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "src/ga/GA.h"

#include <vector>
#include <algorithm>    // std::sort
#include <math.h>       // ceil

namespace fogfn {
using namespace inet;

Define_Module(GA);

void GA::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL){
        //statistics = check_and_cast<statistics *>(getSimulation()->getSystemModule()->getSubmodule("statistics"));
        //selfMsg = new ClockEvent("sendTimer");
        selfMsg = new cMessage("sendTimer");

        numberOfFogNodes = par("numberOfFogNodes");
        numberOfIterations = par("numberOfIterations");
        numberOfServices = numberOfFogNodes; // Each service per each fog node //par("numberOfServices");
        numberOfPopulation = par("numberOfPopulation");

        dataSizePerMip = par("dataSizePerMip");
        unitCostProcessing = par("unitCostProcessing"); //unit cost in $ for processing resource in mips at fog node
        unitCostStorage = par("unitCostStorage"); //unit cost in $ for storage resource at fog node (per byte per second)
        unitCostEnergy = par("unitCostEnergy"); //unit cost in $ for energy consumption per joule in fog environment

        elitismRate = par("elitismRate");
        crossoverProbability = par("crossoverProbability");
        mutationRate = par("mutationRate");

        fogNodes.resize(numberOfFogNodes);
        services.resize(numberOfFogNodes); //Each Fog node one service
        WATCH(numberOfFogNodes);
        WATCH(numberOfIterations);
        WATCH(numberOfServices);
        WATCH(numberOfPopulation);

        WATCH(dataSizePerMip);
        WATCH(unitCostProcessing);
        WATCH(unitCostStorage);
        WATCH(unitCostEnergy);

        WATCH(elitismRate);
        WATCH(crossoverProbability);
        WATCH(mutationRate);

        WATCH_VECTOR(fogNodes);
        WATCH_VECTOR(services);

        startTime = par("startTime");
        //scheduleClockEventAt(startTime, selfMsg);
        scheduleAt(startTime, selfMsg);
    }

}

void GA::registFogNodesInfo(cModule *host, L3Address ipAddress, int fogIndex){
    fogNodes.at(fogIndex).host = host;
    fogNodes.at(fogIndex).ipAddress = ipAddress;
    EV_INFO << "GA::registFogNodesInfo(): Fog node name: " << host->getFullName() << ", Fog node #: " << fogIndex << ", Ip address: " << ipAddress << " is registered." << endl;
}

void GA::registFogServiceInfo(L3Address srcAddr, double processingCapacity, double linkCapacity, double powerIdle, double powerTransmission, double powerProcessing, double dataSize, double requestSize, double responseSize, double serviceDeadline){
    int fogIndex = -1;
    for (int i=0; i<numberOfFogNodes; i++){
        if (fogNodes.at(i).ipAddress == srcAddr){
            fogIndex = i;
        }
    }
    if (fogIndex == -1){
        error("GA::registFogServiceInfo(): The Fog node has not been registered before!");
        //throw cRuntimeError("GA::registFogServiceInfo(): The Fog node has not been registered before!");
    }
    fogNodes.at(fogIndex).processingCapacity = processingCapacity;
    fogNodes.at(fogIndex).linkCapacity = linkCapacity;
    fogNodes.at(fogIndex).powerIdle = powerIdle;
    fogNodes.at(fogIndex).powerTransmission = powerTransmission;
    fogNodes.at(fogIndex).powerProcessing = powerProcessing;
    services.at(fogIndex).dataSize = dataSize;
    services.at(fogIndex).requestSize = requestSize;
    services.at(fogIndex).responseSize = responseSize;
    services.at(fogIndex).serviceDeadline = serviceDeadline;

    EV_INFO << "GA::registFogServiceInfo(): service is registered. Fog info: " << fogNodes.at(fogIndex) << ", Service info: " << services.at(fogIndex) << endl;
}

/*
 * The service time is defined as the sum of processing, communication,
 * and service availability time for the services in the fog computing
 * environment given by Eq. (3).
 */
double GA::serviceTime(Chromosome chromosome){
    double processingTime = 0;  // The processing time tpro depends on the service size (data size) ssize i and the processing capacity of the fog node yj given by Eq. (4).
    double communicationTime = 0; // The communication time tcom is defined as the total time involved for transferring the service request to fog nodes and response back to the actuators. tcom depends on the service size (data size) and the link capacity Bf connected between the nodes as shown by Eq. (5).
    double serviceAvailabilityTime = 0; // The service availability time tav involves the time taken for selecting the fog node to deploy the services and data. Also, it depends on the total time of the service request in the waiting queue to get the selected fog nodes’ computational resources. Thus tav depends on the amount of time it takes for completing the current service request.

    std::vector<double> waitingQueueDelay;
    waitingQueueDelay.resize(numberOfFogNodes);
    for (int j=0; j<numberOfFogNodes ; j++){
        waitingQueueDelay.at(j) = 0;
    }

    for (int i=0; i<numberOfServices ; i++){
        //processingTime += services.at(i).dataSize / fogNodes.at(chromosome.at(i)).processingCapacity;
        double processingTimeServicei = services.at(i).dataSize / fogNodes.at(chromosome.at(i)).processingCapacity;
        processingTime += processingTimeServicei;
        communicationTime += (services.at(i).requestSize + services.at(i).responseSize) / fogNodes.at(chromosome.at(i)).linkCapacity;
        waitingQueueDelay.at(chromosome.at(i)) += processingTime;
        serviceAvailabilityTime += waitingQueueDelay.at(chromosome.at(i)) + processingTimeServicei;
    }

}

/*
 * This method calculates consumed energy for all nodes.
 * If a fog node does not participate in the optimization(or chromosome),
 * its idle energy is calculate for the cost function.
 */
double GA::energyConsumption(Chromosome chromosome){
    double t0 = 0; // start time
    double t1 = 0; // processing start time
    double tEndDelta = 0; // end time
    std::vector<double> processingTimeEndDelta;
    double energyProcessing = 0;
    double energyTransmission = 0;

    //calculate tEndDelta
    processingTimeEndDelta.resize(numberOfFogNodes);
    for(int i=0; i<numberOfFogNodes; i++){
        double tEndDeltai = 0;
        for(int j=0; j<numberOfServices; j++){
            if(chromosome.at(j) == i){
                tEndDeltai += services.at(j).dataSize / fogNodes.at(i).processingCapacity;
            }
        }
        processingTimeEndDelta.at(i) = tEndDelta;
        if(tEndDeltai > tEndDelta){
            tEndDelta = tEndDeltai;
        }
    }

    //calculate t1
    for(int i=0; i<numberOfFogNodes; i++){
        double transmissionTime = services.at(i).requestSize / fogNodes.at(i).linkCapacity;
        if(transmissionTime > t1){
            t1 = transmissionTime;
        }
    }

    //Energy calculation
    for(int i=0; i<numberOfFogNodes ; i++){
        double transmissionTimeEnd = services.at(i).requestSize / fogNodes.at(i).linkCapacity;
        double energyTransmissionIdle = (t1 - transmissionTimeEnd) * fogNodes.at(i).powerIdle;
        double energyTransmissionActive = (transmissionTimeEnd - t0) * fogNodes.at(i).powerTransmission;
        energyTransmission += energyTransmissionIdle + energyTransmissionActive;

        double energyProcessingIdle = (tEndDelta - processingTimeEndDelta.at(i)) * fogNodes.at(i).powerIdle;
        double energyProcessingActive = processingTimeEndDelta.at(i) * fogNodes.at(i).powerProcessing;
        energyProcessing += energyProcessingIdle + energyProcessingActive;

    }
    return energyProcessing + energyTransmission;

}

double GA::serviceCost(Chromosome chromosome){
    double processingCost = 0;
    double storageCost = 0;
    double energyCost = 0;

    for (int i=0; i<numberOfServices ; i++){
        processingCost += services.at(i).dataSize / dataSizePerMip * unitCostProcessing;
        storageCost += services.at(i).dataSize / 8 * unitCostStorage;
    }

    energyCost += energyConsumption(chromosome) * unitCostEnergy;

    return processingCost + storageCost + energyCost;

}

double GA::fitnessFunction(Chromosome chromosome){
    return 3 / (serviceTime(chromosome) + serviceCost(chromosome) + energyConsumption(chromosome));
}

void GA::generationOfInitialPopulation(){
    if(population.size() < numberOfPopulation)
                population.resize(numberOfPopulation);

    for(int n=0; n<numberOfPopulation; n++){
        //generateChromosoe(){
        for(int i=0; i<population.at(n).chromosome.size(); i++){
            population.at(n).chromosome.at(i) = intuniform(0, numberOfFogNodes - 1);
        }
        population.at(n).fitnessValue = 0;
    }
}

void GA::singlePointCrossOverOperation(Chromosome &chromosome1, Chromosome &chromosome2){
    //Chromosome chromosome1 = population.at(intuniform(0, population.size()));
    //Chromosome chromosome2 = population.at(intuniform(0, population.size()));
    int chromosomeSize = chromosome1.size();

    for(int i=ceil(chromosomeSize/2); i< chromosomeSize; i++){
        unsigned int gene = chromosome1.at(i);
        chromosome1.at(i) = chromosome2.at(i);
        chromosome2.at(i) = gene;
    }

}

GA::Chromosome GA::mutationOperation(Chromosome chromosome){
    Chromosome chromosome1 = chromosome;
    chromosome1.at(intuniform(0, chromosome.size())) = intuniform(0, numberOfFogNodes);
    return chromosome1;
}


enum GA::RouletteWheel GA::rouletteWheel(double elitismRate, double crossoverProbability, double mutationRate){
    double linerRouletteWheel = elitismRate + crossoverProbability + mutationRate;
    double randValue = uniform(0 , linerRouletteWheel);
    if(randValue < elitismRate)
        return elitism;
    else if(randValue < crossoverProbability)
        return crossover;
    else
        return mutation;
}

void GA::newGeneration(double elitismRate, double crossoverProbability, double mutationRate){
    int elitismIndex = 0;
    for(int i=0; i<population.size(); i++){
        enum RouletteWheel rouletteWheelValue = rouletteWheel(elitismRate, crossoverProbability, mutationRate);
        if( rouletteWheelValue == elitism){
            newPopulation.at(i) = population.at(elitismIndex);
            elitismIndex++;
        }else if(rouletteWheelValue == crossover){
            Chromosome chromosome1 = population.at(intuniform(0, population.size())).chromosome;
            Chromosome chromosome2 = population.at(intuniform(0, population.size())).chromosome;
            singlePointCrossOverOperation(chromosome1, chromosome2);
            newPopulation.at(i).chromosome = chromosome1;
            newPopulation.at(i).fitnessValue = 0;
            if(++i<population.size()){
                newPopulation.at(i).chromosome = chromosome2;
                newPopulation.at(i).fitnessValue = 0;
            }
        }else{
            newPopulation.at(i).chromosome = mutationOperation(population.at(intuniform(0, population.size())).chromosome);
            newPopulation.at(i).fitnessValue = 0;
        }
    }
    for(int i=0; i<population.size(); i++){
        population.at(i) = newPopulation.at(i);
    }
}

GA::Individualt GA::executeGa(double elitismRate, double crossoverProbability, double mutationRate){
    Individualt bestIndividual = population.at(0);
    generationOfInitialPopulation();
    for(int i=0; i<numberOfIterations; i++){
        for(int j=0; j<numberOfPopulation; j++){
            population.at(j).fitnessValue = fitnessFunction(population.at(j).chromosome);
        }
        sort(population.begin(), population.end(), Individual::compareIndividuals);
        bestIndividual = population.at(0);
        newGeneration(elitismRate, crossoverProbability, mutationRate);
    }
    return bestIndividual;
}

void GA::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        ASSERT(msg == selfMsg);
        executeGa(elitismRate, crossoverProbability, mutationRate);
    }
}

GA::~GA()
{
    cancelAndDelete(selfMsg);
}

std::ostream& operator<<(std::ostream& os, const struct GA::Service& service)
{
     os << "{dataSize: " << service.dataSize;
     os << ", requestSize: " << service.requestSize;
     os << ", responseSize: " << service.responseSize;
     os << ", serviceDeadline: " << service.serviceDeadline << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const struct GA::FogNode& fogNode)
{
     os << "{hostName: " << fogNode.host->getFullName();
     os << ", ipAddress: " << fogNode.ipAddress;
     os << ", processingCapacity: " << fogNode.processingCapacity;
     os << ", linkCapacity: " << fogNode.linkCapacity;
     os << ", powerIdle: " << fogNode.powerIdle;
     os << ", powerTransmission: " << fogNode.powerTransmission;
     os << ", powerProcessing: " << fogNode.powerProcessing << "}";
    return os;
}

} // namespace fogfn
