
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

#include <vector>
#include <algorithm>    // std::sort
#include "src/statistics/Statistics.h"


namespace fogfn {
using namespace inet;

Define_Module(Statistics);

Statistics::Statistics(){

}

void Statistics::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL){

    }

}


void Statistics::handleMessage(cMessage* msg)
{
/*    if (msg->getKind() == Global_REPAIR_TIMER){
        numberOfGlogalRepaires++;
        scheduleNextGlobalRepair();
    }else{
        EV << "Unknown self message is deleted." << endl;
        delete msg;
    }

    return;*/
}


void Statistics::saveStatistics()
{
/*
    //Time statistics
    FILE *convergenceTimeStart, *convergenceTimeEndUpward, *convergenceTimeEndDownward,  *convergenceTimeDownward, *convergenceTimeUpward, *joiningTimeUpward, *joiningTimeDownward;

    convergenceTimeStart = fopen("convergenceTimeStart.txt", "a");
    fprintf(convergenceTimeStart, "%f\n", this->convergenceTimeStart.dbl());
    fclose(convergenceTimeStart);

    convergenceTimeEndUpward = fopen("convergenceTimeEndUpward.txt", "a");
    fprintf(convergenceTimeEndUpward, "%f\n", this->convergenceTimeEndUpward.dbl());
    fclose(convergenceTimeEndUpward);

    joiningTimeUpward = fopen("joiningTimeUpward.txt", "a");
    for (unsigned int i=0; i<nodeStateList.size(); i++){
        fprintf(joiningTimeUpward, "%f\n", nodeStateList.at(i).joiningTimeUpward.dbl());
    }
    fclose(joiningTimeUpward);

    convergenceTimeUpward = fopen("01_1_convergenceTimeUpward.txt", "a");
    fprintf(convergenceTimeUpward, "%f\n", this->convergenceTimeEndUpward.dbl() - this->convergenceTimeStart.dbl());
    fclose(convergenceTimeUpward);

    if ((mop == Storing_Mode_of_Operation_with_no_multicast_support) || (mop == Non_Storing_Mode_of_Operation)){
        convergenceTimeEndDownward = fopen("convergenceTimeEndDownward.txt", "a");
        fprintf(convergenceTimeEndDownward, "%f\n", this->convergenceTimeEndDownward.dbl());
        fclose(convergenceTimeEndDownward);

        joiningTimeDownward = fopen("joiningTimeDownward.txt", "a");
        for (unsigned int i=0; i<nodeStateList.size(); i++){
            fprintf(joiningTimeDownward, "%f\n", nodeStateList.at(i).joiningTimeDownward.dbl());
        }
        fclose(joiningTimeDownward);

        convergenceTimeDownward = fopen("01_2_convergenceTimeDownward.txt", "a");
        fprintf(convergenceTimeDownward, "%f\n", this->convergenceTimeEndDownward.dbl() - this->convergenceTimeStart.dbl());
        fclose(convergenceTimeDownward);
    }

    //Message statistics
    FILE *averageSentDIO, *averageSentDIS, *averageSentDAO, *averageNumberOfMessages;
    double averageDIO = 0, averageDIS = 0, averageDAO = 0;

    averageSentDIO = fopen("averageSentDIO.txt", "a");
    averageDIO = (double)(numSentDIO) / nodeStateList.size();
    fprintf(averageSentDIO, "%f\n", averageDIO);
    fclose(averageSentDIO);

    averageSentDIS = fopen("averageSentDIS.txt", "a");
    averageDIS = (double)(numSentDIS) / nodeStateList.size();
    fprintf(averageSentDIS, "%f\n", averageDIS);
    fclose(averageSentDIS);

    if ((mop == Storing_Mode_of_Operation_with_no_multicast_support) || (mop == Non_Storing_Mode_of_Operation)){
        averageSentDAO = fopen("averageSentDAO.txt", "a");
        averageDAO = (double)(numSentDAO) / nodeStateList.size();
        fprintf(averageSentDAO, "%f\n", averageDAO);
        fclose(averageSentDAO);
    }

    averageNumberOfMessages = fopen("02_averageNumberOfMessages.txt", "a");
    fprintf(averageNumberOfMessages, "%f\n", averageDIO + averageDIS + averageDAO);
    fclose(averageNumberOfMessages);

    //Table statistics
    FILE *averageNumberOfNeighborsFP, *averageNumberOfParentsFP, *averageNumberOfRoutesFP, *averageNumberOfSRRoutesFP, *averageNumberofTableEntriesFP;
    double averageNumberOfNeighbors, averageNumberOfParents, averageNumberOfRoutes, averageNumberOfSRRoutes;

    averageNumberOfNeighbors = (double) (numberOfNeighbors) / nodeStateList.size();
    averageNumberOfNeighborsFP = fopen("averageNumberOfNeighbors.txt", "a");
    fprintf(averageNumberOfNeighborsFP, "%f\n", averageNumberOfNeighbors);
    fclose(averageNumberOfNeighborsFP);

    averageNumberOfParents = (double) (numberOfParents) / nodeStateList.size();
    averageNumberOfParentsFP = fopen("averageNumberOfParents.txt", "a");
    fprintf(averageNumberOfParentsFP, "%f\n", averageNumberOfParents);
    fclose(averageNumberOfParentsFP);

    averageNumberOfRoutes = (double)(numberOfRoutes) / nodeStateList.size();
    averageNumberOfRoutesFP = fopen("averageNumberOfRoutes.txt", "a");
    fprintf(averageNumberOfRoutesFP, "%f\n", averageNumberOfRoutes);
    fclose(averageNumberOfRoutesFP);

    if (mop == Non_Storing_Mode_of_Operation){
        averageNumberOfSRRoutes = (double)(numberOfSRRoutes) / nodeStateList.size();
        averageNumberOfSRRoutesFP = fopen("averageNumberOfSRRoutes.txt", "a");
        fprintf(averageNumberOfSRRoutesFP, "%f\n", averageNumberOfSRRoutes);
        fclose(averageNumberOfSRRoutesFP);
    }

    averageNumberofTableEntriesFP = fopen("03_averageNumberofTableEntries.txt", "a");
    //fprintf(averageNumberofTableEntriesFP, "%f\n", averageNumberOfNeighbors + averageNumberOfParents + averageNumberOfDefaultRoutes + averageNumberOfRoutes);
    fprintf(averageNumberofTableEntriesFP, "%f\n", averageNumberOfNeighbors + averageNumberOfParents + averageNumberOfRoutes);
    fclose(averageNumberofTableEntriesFP);

    //Hop count statistics
    if ((mop == Storing_Mode_of_Operation_with_no_multicast_support) || (mop == Non_Storing_Mode_of_Operation) || (mop == No_Downward_Routes_maintained_by_RPL)){
        calculateHopCount();

        FILE *numberofHopCountFP, *averageNumberofHopCountFP;
        numberofHopCountFP = fopen("numberofHopCount.txt", "a");
        averageNumberofHopCount = 0;
        int numflows = 0;
        for (unsigned int i = 0; i < nodeStateList.size(); i++){
            for (unsigned int j = 0; j < nodeStateList.size(); j++){
                //fprintf(numberofHopCountFP, "%3d\t", hopCountMat.at(i).at(j));  //sorted hopCountMat
                fprintf(numberofHopCountFP, "%3d\t", hopCountMat.at(nodeIndexToOrderedIndex(i)).at(nodeIndexToOrderedIndex(j)));  //hopCountMat must be converted from ordered index to non-ordered/original one.
                if ((i != j) && (hopCountMat.at(nodeIndexToOrderedIndex(i)).at(nodeIndexToOrderedIndex(j)) != -1)){
                    averageNumberofHopCount += hopCountMat.at(nodeIndexToOrderedIndex(i)).at(nodeIndexToOrderedIndex(j));
                    numflows++; // Finally, numflows will be nodeStateList.size() ^ 2 - nodeStateList.size()
                }
            }
            fprintf(numberofHopCountFP, "\n");
        }
        fprintf(numberofHopCountFP, "\n -------------------------------------------------------------\n");
        fclose(numberofHopCountFP);
        averageNumberofHopCount /= numflows;

        averageNumberofHopCountFP = fopen("04_averageNumberofHopCount.txt", "a");
        fprintf(averageNumberofHopCountFP, "%f\n", averageNumberofHopCount);
        fclose(averageNumberofHopCountFP);
    }

    //Other statistics
    //FILE *preferedParent, *nodeRank;

     */
}
Statistics::~Statistics()
{
    //cancelAndDelete(globalRepairTimer);


}

} // namespace fogfn
