//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004,2011 OpenSim Ltd.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "src/udpapp/UdpBasicAppFog.h"

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/FragmentationTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"
//Extra Begin
#include "inet/common/packet/printer/PacketPrinter.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
//Extra End

namespace fogfn {
using namespace inet;


Define_Module(UdpBasicAppFog);

UdpBasicAppFog::~UdpBasicAppFog()
{
    cancelAndDelete(selfMsg);
}

void UdpBasicAppFog::initialize(int stage)
{
    ClockUserModuleMixin::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        numSent = 0;
        numReceived = 0;
        WATCH(numSent);
        WATCH(numReceived);

        localPort = par("localPort");
        destPort = par("destPort");
        startTime = par("startTime");
        stopTime = par("stopTime");
        packetName = par("packetName");
        dontFragment = par("dontFragment");
        if (stopTime >= CLOCKTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new ClockEvent("sendTimer");

        //Extra Begin
        //Fog node info
        processingCapacity.doubleValue = par("processingCapacity"); //processing capability of a fog node j

        linkCapacity.doubleValue = par("linkCapacity"); //bandwidth between nodes in Mbps

        powerIdle.doubleValue = par("powerIdle"); // power consumption of fog node in idle state in watts
        powerTransmission.doubleValue = par("powerTransmission"); // maximum power consumption during transmission in watts
        powerProcessing.doubleValue = par("powerProcessing"); // maximum power consumption during processing in watts

        //Service Info
        dataSize.doubleValue = par("dataSize"); // service size to calculate the processing time
        requestSize.doubleValue = par("requestSize"); // size of service request to calculate the communication time
        responseSize.doubleValue = par("responseSize"); // size of service response to calculate the communication time

        serviceDeadline.doubleValue = par("serviceDeadline");


    }else if(stage == INITSTAGE_TRANSPORT_LAYER){
        //L3Address myIpAddr;
        Ipv4Address myIpAddr;
        IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        int nInterfaces = ift->getNumInterfaces();
        if(nInterfaces > 2) //If host has more than 2 interfaces...
            error("The host has more than 2 interfaces!");
        for (int k=0; k < nInterfaces; k++){
            if (!ift->getInterface(k)->isLoopback())
            {
                myIpAddr = ift->getInterface(k)->getIpv4Address();
            }
        }
        optimizerType = par("optimizerType").stdstringValue();
        WATCH(optimizerType);
        if(optimizerType == "Genetic"){
            grayWolfOptimizer = nullptr;
            gaOptimizer = check_and_cast<GA *>(getSimulation()->getSystemModule()->getSubmodule("gaOptimizer"));
            gaOptimizer->registFogNodesInfo(getContainingNode(this), myIpAddr, getContainingNode(this)->getIndex());
        }else if(optimizerType == "GrayWolf"){
            gaOptimizer = nullptr;
            grayWolfOptimizer = check_and_cast<GrayWolf *>(getSimulation()->getSystemModule()->getSubmodule("grayWolfOptimizer"));
            grayWolfOptimizer->registFogNodesInfo(getContainingNode(this), myIpAddr, getContainingNode(this)->getIndex());
        }

        EV_INFO << "My index is " << getContainingNode(this)->getIndex() << ", my IP Address is " << myIpAddr << endl;

    }
    //Extra End
}

void UdpBasicAppFog::finish()
{
    recordScalar("packets sent", numSent);
    recordScalar("packets received", numReceived);
    ApplicationBase::finish();
}

void UdpBasicAppFog::setSocketOptions()
{
    int timeToLive = par("timeToLive");
    if (timeToLive != -1)
        socket.setTimeToLive(timeToLive);

    int dscp = par("dscp");
    if (dscp != -1)
        socket.setDscp(dscp);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    const char *multicastInterface = par("multicastInterface");
    if (multicastInterface[0]) {
        IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        NetworkInterface *ie = ift->findInterfaceByName(multicastInterface);
        if (!ie)
            throw cRuntimeError("Wrong multicastInterface setting: no interface named \"%s\"", multicastInterface);
        socket.setMulticastOutputInterface(ie->getInterfaceId());
    }

    bool receiveBroadcast = par("receiveBroadcast");
    if (receiveBroadcast)
        socket.setBroadcast(true);

    bool joinLocalMulticastGroups = par("joinLocalMulticastGroups");
    if (joinLocalMulticastGroups) {
        MulticastGroupList mgl = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this)->collectMulticastGroups();
        socket.joinLocalMulticastGroups(mgl);
    }
    socket.setCallback(this);
}

L3Address UdpBasicAppFog::chooseDestAddr()
{
    int k = intrand(destAddresses.size());
    if (destAddresses[k].isUnspecified() || destAddresses[k].isLinkLocal()) {
        L3AddressResolver().tryResolve(destAddressStr[k].c_str(), destAddresses[k]);
    }
    return destAddresses[k];
}

void UdpBasicAppFog::sendPacket()
{
    //Extra Begin
    std::vector<uint8_t> rawBytes; //std::vector<unsigned char> rawBytes;
    rawBytes.resize(sizeof(double) * 12);
    for (int i=0; i<sizeof(double); i++){
        rawBytes[i] = processingCapacity.rawBytes[i];
        //EV << "Basic app: processingCapacity.rawBytes[" << i << "] = " << processingCapacity.rawBytes[i] << endl;
        //EV << "Basic app: packet.rawBytes[" << i << "] = " << rawBytes[i] << endl;
        //EV_INFO << "Sent processingCapacity = " << processingCapacity.doubleValue<<endl;
        rawBytes[i+1*sizeof(double)] = linkCapacity.rawBytes[i];
        rawBytes[i+2*sizeof(double)] = powerIdle.rawBytes[i];
        rawBytes[i+3*sizeof(double)] = powerTransmission.rawBytes[i];
        rawBytes[i+4*sizeof(double)] = powerProcessing.rawBytes[i];
        rawBytes[i+5*sizeof(double)] = dataSize.rawBytes[i];
        rawBytes[i+6*sizeof(double)] = requestSize.rawBytes[i];
        rawBytes[i+7*sizeof(double)] = responseSize.rawBytes[i];
        rawBytes[i+8*sizeof(double)] = serviceDeadline.rawBytes[i];
    }
    EV_INFO << "UdpBasicAppFog::sendPacket(): Data presentation to be sent:" << " processingCapacity = " << processingCapacity.doubleValue << ", linkCapacity = " << linkCapacity.doubleValue << ", powerIdle = " << powerIdle.doubleValue << ", powerTransmission = " << powerTransmission.doubleValue << ", dataSize = " << dataSize.doubleValue << ", requestSize = " << requestSize.doubleValue << ", responseSize = " << responseSize.doubleValue << ", serviceDeadline = " << serviceDeadline.doubleValue << endl;

    //Extra End
    std::ostringstream str;
    str << packetName << "-" << numSent;
    Packet *packet = new Packet(str.str().c_str());
    //Extra Begin
/*  if (dontFragment)
        packet->addTag<FragmentationReq>()->setDontFragment(true);
    const auto& payload = makeShared<ApplicationPacket>();
    payload->setChunkLength(B(par("messageLength")));
    payload->setSequenceNumber(numSent);
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(payload);
*/
    //auto byteCountData = makeShared<ByteCountChunk>(B(sizeof(double) * 12), '?');
    auto rawBytesData = makeShared<BytesChunk>(); // sizeof(double) * 12 raw bytes
    rawBytesData->setBytes(rawBytes); //{processingCapacity.rawBytes[0], processingCapacity.rawBytes[1], processingCapacity.rawBytes[2], processingCapacity.rawBytes[3], processingCapacity.rawBytes[4], processingCapacity.rawBytes[5], processingCapacity.rawBytes[6], processingCapacity.rawBytes[7]});

    //packet->insertAtBack(byteCountData);
    packet->insertAtBack(rawBytesData);

    PacketPrinter printer; // turns packets into human readable strings
    printer.printPacket(std::cout, packet); // print to standard output

    /*ServiceRequestMessage *packet = new ServiceRequestMessage(str.str().c_str());
    packet->setName("ServiceRequestMessage");
    packet->setProcessingCapacity(processingCapacity);
    packet->setLinkCapacity(linkCapacity);
    packet->setPowerIdle(powerIdle);
    packet->setPowerTransmission(powerTransmission);
    packet->setPowerProcessing(powerProcessing);
    packet->setDataSize(dataSize);
    packet->setRequestSize(requestSize);
    packet->setResponseSize(responseSize);
    packet->setServiceDeadline(serviceDeadline);
    //packet->encapsulate(payload);
    //Extra End*/
    L3Address destAddr = chooseDestAddr();
    emit(packetSentSignal, packet);
    socket.sendTo((Packet *)packet, destAddr, destPort);
    numSent++;
}

void UdpBasicAppFog::processStart()
{
    socket.setOutputGate(gate("socketOut"));
    const char *localAddress = par("localAddress");
    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
    setSocketOptions();

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;

    while ((token = tokenizer.nextToken()) != nullptr) {
        destAddressStr.push_back(token);
        L3Address result;
        L3AddressResolver().tryResolve(token, result);
        if (result.isUnspecified())
            EV_ERROR << "cannot resolve destination address: " << token << endl;
        destAddresses.push_back(result);
    }

    if (!destAddresses.empty()) {
        selfMsg->setKind(SEND);
        processSend();
    }
    else {
        if (stopTime >= CLOCKTIME_ZERO) {
            selfMsg->setKind(STOP);
            scheduleClockEventAt(stopTime, selfMsg);
        }
    }
}

void UdpBasicAppFog::processSend()
{
    sendPacket();
    clocktime_t d = par("sendInterval");
    if (stopTime < CLOCKTIME_ZERO || getClockTime() + d < stopTime) {
        selfMsg->setKind(SEND);
        scheduleClockEventAfter(d, selfMsg);
    }
    else {
        selfMsg->setKind(STOP);
        scheduleClockEventAt(stopTime, selfMsg);
    }
}

void UdpBasicAppFog::processStop()
{
    socket.close();
}

void UdpBasicAppFog::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        ASSERT(msg == selfMsg);
        switch (selfMsg->getKind()) {
            case START:
                processStart();
                break;

            case SEND:
                processSend();
                break;

            case STOP:
                processStop();
                break;

            default:
                throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
        }
    }
    else
        socket.processMessage(msg);
}

void UdpBasicAppFog::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    // process incoming packet
    processPacket(packet);
}

void UdpBasicAppFog::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
    delete indication;
}

void UdpBasicAppFog::socketClosed(UdpSocket *socket)
{
    if (operationalState == State::STOPPING_OPERATION)
        startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
}

void UdpBasicAppFog::refreshDisplay() const
{
    ApplicationBase::refreshDisplay();

    char buf[100];
    sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
    getDisplayString().setTagArg("t", 0, buf);
}

void UdpBasicAppFog::processPacket(Packet *pk)
{
    emit(packetReceivedSignal, pk);
    EV_INFO << "Received packet: " << UdpSocket::getReceivedPacketInfo(pk) << endl;
    delete pk;
    numReceived++;
}

void UdpBasicAppFog::handleStartOperation(LifecycleOperation *operation)
{
    clocktime_t start = std::max(startTime, getClockTime());
    if ((stopTime < CLOCKTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime)) {
        selfMsg->setKind(START);
        scheduleClockEventAt(start, selfMsg);
    }
}

void UdpBasicAppFog::handleStopOperation(LifecycleOperation *operation)
{
    cancelEvent(selfMsg);
    socket.close();
    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void UdpBasicAppFog::handleCrashOperation(LifecycleOperation *operation)
{
    cancelClockEvent(selfMsg);
    socket.destroy(); // TODO  in real operating systems, program crash detected by OS and OS closes sockets of crashed programs.
}

} // namespace fogfn

