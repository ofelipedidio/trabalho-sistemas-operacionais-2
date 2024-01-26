#include "../include/election.h"
#include <iterator> // Add this include directive
#include <algorithm>
#include <iostream>

void initiateElection();

bool comp(const server_t& a, const server_t& b){
    if( a.ip > b.ip ) return true;
    if( a.ip < b.ip) return false;
    return (a.port > b.port);
}

server_t getNextServer(const metadata_t &metadata, const server_t &currentServer)
{
    if (metadata.servers.size() == 1)
        return currentServer;

    auto it = std::find(metadata.servers.begin(), metadata.servers.end(), currentServer);
    it = it == metadata.servers.end() ? metadata.servers.begin() : ++it;
    if(it->server_type == primary)return getNextServer(metadata, *(it));
    return *(it);
    
}

void receiveElectionMessage(metadata_t& metadata, server_t& currentServer, server_t& senderServer)
{
    server_t nextServer = getNextServer(metadata, currentServer);

    if(currentServer.ip == senderServer.ip && currentServer.port == senderServer.port)
        return sendElectedMessage(nextServer, currentServer);
    
    if(comp(currentServer, senderServer))
        return sendElectionMessage(nextServer, currentServer);
    
    sendElectionMessage(nextServer, senderServer);
}


void receiveElectedMessage(metadata_t& metadata, server_t& currentServer, server_t& electedServer){
    if(currentServer.ip == electedServer.ip && currentServer.port == electedServer.port){
        std::cerr<<"election finished ";
        printServer(electedServer);
        return;
    }
    updateElected(currentServer, electedServer);
    server_t nextServer = getNextServer(metadata, currentServer);
    sendElectedMessage(nextServer, electedServer);
}

void setElected(metadata_t& metadata, server_t& electedServer){

    for(int i = metadata.servers.size(); i >=0; ){
        if(metadata.servers[i].server_type == primary){
            metadata.servers.pop_back();
            i-=2;
            continue;
        }
        i--;
    }

    // TODO 
    // ver qual erro que dá
    electedServer.server_type = primary;

    printServer(electedServer);    
}

void printServer(const server_t& server){
    std::cerr<<server.ip<<' '<<server.port<<' '<<server.server_type<<'\n';
}

void initiateElection(){
    metadata_t metadata = GetMetadata();
    //how to find myself?
    //TODO
    server_t currentServer;
    server_t targetServer = getNextServer(metadata, currentServer);
    sendElectionMessage(targetServer, currentServer);




}
