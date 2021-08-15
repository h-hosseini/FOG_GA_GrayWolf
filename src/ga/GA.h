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

#ifndef _FOGFN_SRC_GA_H_
#define _FOGFN_SRC_GA_H_

#define NUMBER_OF_TASKS 5
#define NUMBER_OF_JOBS 5

#include "inet/common/INETDefs.h"
#include "src/statistics/Statistics.h"
#include "inet/networklayer/common/L3AddressResolver.h"



namespace fogfn {
using namespace inet;

class GA : public cSimpleModule
{
    int numberOfFogNodes;
    int numberOfIterations;
    int numberOfServices;
    int numberOfPopulation;
    double dataSizePerMip;
    double unitCostProcessing; //unit cost in $ for processing resource in mips at fog node
    double unitCostStorage; //unit cost in $ for storage resource at fog node (per byte per second)
    double unitCostEnergy; //unit cost in $ for energy consumption per joule in fog environment

    double elitismRate;
    double crossoverProbability;
    double mutationRate;

    enum RouletteWheel{elitism = 0, crossover, mutation};

    Statistics *statistics;

    struct FogNode{
        cModule *host;
        L3Address ipAddress;
        //int nodeIndex;

        double processingCapacity; //processing capability of a fog node j
        double linkCapacity; //bandwidth between nodes in Mbps

        double powerIdle; // power consumption of fog node in idle state in watts
        double powerTransmission; // maximum power consumption during transmission in watts
        double powerProcessing; // maximum power consumption during processing in watts

        FogNode()
            : host(nullptr)
            , ipAddress (Ipv4Address::UNSPECIFIED_ADDRESS)
            , processingCapacity (0)
            , linkCapacity(0)
            , powerIdle(0)
            , powerTransmission (0)
            , powerProcessing (0)
            {};

    };
    friend std::ostream& operator<<(std::ostream& os, const struct FogNode& fogNode);

    typedef std::vector<struct FogNode> FogNodes;
    FogNodes fogNodes;


    struct Service{
        double cpuRequired;
        double ramRequired;
        double storageRequired;

        double dataSize; // service size to calculate the processing time
        double requestSize; // size of service request to calculate the communication time
        double responseSize; // size of service response to calculate the communication time

        double serviceDeadline;


        Service()
            : cpuRequired(0)
            , ramRequired(0)
            , storageRequired(0)
            , dataSize(0)
            , requestSize(0)
            , responseSize(0)
            , serviceDeadline(0)
            {};

    };
    friend std::ostream& operator<<(std::ostream& os, const struct Service& service);

    typedef std::vector<struct Service> Services;
    Services services;

    //A 2-D vector to maintain the assigned service to each fog node
    typedef std::vector<std::vector<unsigned int>> Xij; //Eq. (2), xij = 1 if service [i] is placed on the fog node [j] else, xij = 0.

    //A chromosome data structure
    typedef std::vector <int> Chromosome;
    //Chromosome chromosome;

    struct Individual{
        Chromosome chromosome;
        double fitnessValue;

        public:
            static bool compareIndividuals(Individual &a, Individual &b) {
                return a.fitnessValue > b.fitnessValue;
        }
    };
    typedef struct Individual Individualt;
    typedef std::vector <Individualt> Population;
    Population population;
    Population newPopulation;

public:
    GA()
        : numberOfFogNodes(0)
        , numberOfIterations(0)
        , numberOfServices(0)
        , numberOfPopulation(0)
        , dataSizePerMip(0)
        , unitCostProcessing(0)
        , unitCostStorage(0)
        , unitCostEnergy(0)
        , elitismRate(0)
        , crossoverProbability(0)
        , mutationRate(0)
        , statistics (nullptr)
        {};

    ~GA();

protected:

    virtual void initialize(int stage);
    virtual double serviceTime(Chromosome chromosome);
    virtual double energyConsumption(Chromosome chromosome);
    virtual double serviceCost(Chromosome chromosome);
    virtual double fitnessFunction(Chromosome chromosome);
    virtual void generationOfInitialPopulation();
    virtual void singlePointCrossOverOperation(Chromosome &chromosome1, Chromosome &chromosome2);
    virtual Chromosome mutationOperation(Chromosome chromosome1);
    virtual enum RouletteWheel rouletteWheel(double elitismRate, double crossoverProbability, double mutationRate);
    virtual void newGeneration(double elitismRate, double crossoverProbability, double mutationRate);

public:
    virtual void registFogNodesInfo(cModule *host, L3Address ipAddress, int fogIndex);
    virtual void registFogServiceInfo(L3Address srcAddr, double processingCapacity, double linkCapacity, double powerIdle, double powerTransmission, double powerProcessing, double cpuRequired, double ramRequired, double storageRequired, double dataSize, double requestSize, double responseSize, double serviceDeadline);
    virtual Individualt execute(double elitismRate, double crossoverProbability, double mutationRate);

};

} // namespace fogfn

#endif // ifndef _FOGFN_SRC_GA_H_
