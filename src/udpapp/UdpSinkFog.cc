//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "src/udpapp/UdpSinkFog.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpControlInfo_m.h"

//Extra Begin
#include "inet/common/packet/printer/PacketPrinter.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
//Extra End

namespace fogfn {
using namespace inet;


Define_Module(UdpSinkFog);

UdpSinkFog::~UdpSinkFog()
{
    cancelAndDelete(selfMsg);
}

void UdpSinkFog::initialize(int stage)
{
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        numReceived = 0;
        WATCH(numReceived);

        localPort = par("localPort");
        startTime = par("startTime");
        stopTime = par("stopTime");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        selfMsg = new cMessage("UDPSinkTimer");
        //Extra Begin
        gaOptimizer = check_and_cast<GA *>(getSimulation()->getSystemModule()->getSubmodule("gaOptimizer"));
        //Extra End
    }
}

void UdpSinkFog::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        ASSERT(msg == selfMsg);
        switch (selfMsg->getKind()) {
            case START:
                processStart();
                break;

            case STOP:
                processStop();
                break;

            default:
                throw cRuntimeError("Invalid kind %d in self message", (int)selfMsg->getKind());
        }
    }
    else if (msg->arrivedOn("socketIn"))
        socket.processMessage(msg);
    else
        throw cRuntimeError("Unknown incoming gate: '%s'", msg->getArrivalGate()->getFullName());
}

void UdpSinkFog::socketDataArrived(UdpSocket *socket, Packet *packet)
{
    // process incoming packet
    processPacket(packet);
}

void UdpSinkFog::socketErrorArrived(UdpSocket *socket, Indication *indication)
{
    EV_WARN << "Ignoring UDP error report " << indication->getName() << endl;
    delete indication;
}

void UdpSinkFog::socketClosed(UdpSocket *socket)
{
    if (operationalState == State::STOPPING_OPERATION)
        startActiveOperationExtraTimeOrFinish(par("stopOperationExtraTime"));
}

void UdpSinkFog::refreshDisplay() const
{
    ApplicationBase::refreshDisplay();

    char buf[50];
    sprintf(buf, "rcvd: %d pks", numReceived);
    getDisplayString().setTagArg("t", 0, buf);
}

void UdpSinkFog::finish()
{
    ApplicationBase::finish();
    EV_INFO << getFullPath() << ": received " << numReceived << " packets\n";
}

void UdpSinkFog::setSocketOptions()
{
    bool receiveBroadcast = par("receiveBroadcast");
    if (receiveBroadcast)
        socket.setBroadcast(true);

    MulticastGroupList mgl = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this)->collectMulticastGroups();
    socket.joinLocalMulticastGroups(mgl);

    // join multicastGroup
    const char *groupAddr = par("multicastGroup");
    multicastGroup = L3AddressResolver().resolve(groupAddr);
    if (!multicastGroup.isUnspecified()) {
        if (!multicastGroup.isMulticast())
            throw cRuntimeError("Wrong multicastGroup setting: not a multicast address: %s", groupAddr);
        socket.joinMulticastGroup(multicastGroup);
    }
    socket.setCallback(this);
}

void UdpSinkFog::processStart()
{
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);
    setSocketOptions();

    if (stopTime >= SIMTIME_ZERO) {
        selfMsg->setKind(STOP);
        scheduleAt(stopTime, selfMsg);
    }
}

void UdpSinkFog::processStop()
{
    if (!multicastGroup.isUnspecified())
        socket.leaveMulticastGroup(multicastGroup); // FIXME should be done by socket.close()
    socket.close();
}

void UdpSinkFog::processPacket(Packet *pk)
{
    EV_INFO << "Received packet: " << UdpSocket::getReceivedPacketInfo(pk) << endl;
    emit(packetReceivedSignal, pk);
    //Extra Begin
    PacketPrinter printer; // turns packets into human readable strings
    printer.printPacket(std::cout, pk); // print to standard output

    auto data = pk->peekDataAsBytes(); //peekAllAsBytes(); //peekData(); // peek remaining data in packet
    //std::vector<unsigned char> rawBytes;
    //rawBytes.resize(sizeof(double) * 12);
    for (int i=0; i<sizeof(double); i++){
        processingCapacity.rawBytes[i] = data->getByte(i); //data[i];
        //EV << "Sink: processingCapacity.rawBytes[" << i << "] = " << processingCapacity.rawBytes[i] << endl;
        linkCapacity.rawBytes[i] = data->getByte(i+1*sizeof(double)); //data[i+1*sizeof(double)];
        powerIdle.rawBytes[i] = data->getByte(i+2*sizeof(double)); //data[i+2*sizeof(double)];
        powerTransmission.rawBytes[i] = data->getByte(i+3*sizeof(double)); //data[i+3*sizeof(double)];
        powerProcessing.rawBytes[i] = data->getByte(i+4*sizeof(double)); //data[i+4*sizeof(double)];
        dataSize.rawBytes[i] = data->getByte(i+5*sizeof(double)); //data[i+8*sizeof(double)];
        requestSize.rawBytes[i] = data->getByte(i+6*sizeof(double)); //data[i+9*sizeof(double)];
        responseSize.rawBytes[i] = data->getByte(i+7*sizeof(double)); //data[i+10*sizeof(double)];
        serviceDeadline.rawBytes[i] = data->getByte(i+8*sizeof(double)); //data[i+11*sizeof(double)];
    }
    auto l3Addresses = pk->getTag<L3AddressInd>();
    //auto ports = pk->getTag<L4PortInd>();
    L3Address srcAddr = l3Addresses->getSrcAddress();
    //L3Address destAddr = l3Addresses->getDestAddress();
    gaOptimizer->registFogServiceInfo(srcAddr, processingCapacity.doubleValue, linkCapacity.doubleValue, powerIdle.doubleValue, powerTransmission.doubleValue, powerProcessing.doubleValue, dataSize.doubleValue, requestSize.doubleValue, responseSize.doubleValue, serviceDeadline.doubleValue);

    EV_INFO << "UdpSinkFog::processPacket(Packet): Received packet from " << srcAddr << ", processingCapacity = " << processingCapacity.doubleValue << ", linkCapacity = " << linkCapacity.doubleValue << ", powerIdle = " << powerIdle.doubleValue << ", powerTransmission = " << powerTransmission.doubleValue << ", dataSize = " << dataSize.doubleValue << ", requestSize = " << requestSize.doubleValue << ", responseSize = " << responseSize.doubleValue << ", serviceDeadline = " << serviceDeadline.doubleValue << endl;

    delete pk;

    numReceived++;
}

void UdpSinkFog::handleStartOperation(LifecycleOperation *operation)
{
    simtime_t start = std::max(startTime, simTime());
    if ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime)) {
        selfMsg->setKind(START);
        scheduleAt(start, selfMsg);
    }
}

void UdpSinkFog::handleStopOperation(LifecycleOperation *operation)
{
    cancelEvent(selfMsg);
    if (!multicastGroup.isUnspecified())
        socket.leaveMulticastGroup(multicastGroup); // FIXME should be done by socket.close()
    socket.close();
    delayActiveOperationFinish(par("stopOperationTimeout"));
}

void UdpSinkFog::handleCrashOperation(LifecycleOperation *operation)
{
    cancelEvent(selfMsg);
    if (operation->getRootModule() != getContainingNode(this)) { // closes socket when the application crashed only
        if (!multicastGroup.isUnspecified())
            socket.leaveMulticastGroup(multicastGroup); // FIXME should be done by socket.close()
        socket.destroy(); // TODO  in real operating systems, program crash detected by OS and OS closes sockets of crashed programs.
    }
}

} // namespace fogfn

