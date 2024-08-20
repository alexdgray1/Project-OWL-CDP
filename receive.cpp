#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <chrono>
#include <ctime>
#include "Packet.h"
#include "PapaDuck.h"
#include "MamaDuck.h"
//#include "BloomFilter.h"
#include "Utils.h"

using namespace std;






void writeFileWebServer (string filename, vector<string> TxData) {
    ofstream fout;
    fout.open(filename);
    if (!fout.is_open())
    {
        cout << "Output file failed to open." << endl;
        exit(EXIT_FAILURE);
    }
    fout<< "DDUID:" << TxData[0] << endl;
    fout << "Topic:" << TxData[1] << endl;
    fout<< "Data:" << TxData[2] <<endl;
    fout <<"Date:" << TxData[3] << endl;
    fout << "Time:" << TxData[4] << endl;
    fout.close();
}


/*string unmodifyString (string cdp, int position){
    char aByte;
    if(cdp[position] > 57 && cdp[position+1] > 57 ){
        aByte = (((cdp[position] -55) << 4) &0xF0) | ((cdp[position+1]-55) & 0x0F);
        //aByte = (cdp[position] - 55)  << 4;
    }
    else if (cdp[position] > 57 && cdp[position+1] <57)
    {
        aByte = (((cdp[position] -55) << 4) &0xF0) | ((cdp[position+1]-48) & 0x0F);
    }
    else if (cdp[position] < 57 && cdp[position+1] >57)
    {
        aByte = (((cdp[position] -48) << 4) &0xF0) | ((cdp[position+1]-55) & 0x0F);
    }
    else{
        aByte = (((cdp[position]-48) << 4) & 0xF0) | ((cdp[position+1]-48) &0x0F);
        //aByte = (cdp[position] -48) << 4;
    }
    cout << aByte << endl;
    cdp[position]= aByte;
    cdp.erase(position+1, 1);
    return cdp;

}*/
string unmodifyString (string cdp, int position){
    if(cdp[position] > 54){
        cdp[position] = cdp[position] - 55;
    }
    else{
        cdp[position] = cdp[position] - 48;
    }
    
    return cdp;
}



int main()
{

////set device ID
    string devID = "MAMA0003";

    // Instantiate Packet Object
    Packet dp;

    MamaDuck md;

    md.setDuckId(duckutils::convertStringToVector("MAMA0003"));

    vector<uint8_t> packetRecieved;
    //packetRecieved = {'P', 'A', 'P', 'A' , 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A' , 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A' , 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A', 'P', 'A'};
    string CDP = "DUCK0001MAMA0003Y4D5G50��mTest Data String";

    for(int i = 0; i < 7; i++)
    {
        CDP = unmodifyString(CDP, TOPIC_POS+i);
    }

    packetRecieved = duckutils::convertStringToVector(CDP);
    dp.setBuffer(packetRecieved);

    //gets payload generated
    vector<uint8_t> payload = dp.getBuffer();

    //Packet packet = Packet(dp.getBuffer());
    //duckutils::printVector(packet.getBuffer());

    

    md.handleReceivedPacket(dp);

    dp.reset();

    /*string destination = CDP.substr(DDUID_POS, devID.length());

    //check if this is the packet is going to this device
    if(destination == devID)
    {
        //TODO handle duck commands, acknowledgements, and pings


        //parses sections of received cdp payload
        vector<string> outputFileData;
        outputFileData = dp.decodePacket(payload);

        CDP = duckutils::convertVectorToString(payload);

        cout << "CDP packet as a string: " << CDP << endl;

        writeFileWebServer("outfile.txt", outputFileData);
    }
    else if (destination  == duckutils::convertVectorToString(BROADCAST_DUID))
    {

    }
    else
    {
        //Handle relaying data
        cout << "Not this duck" << endl;
    }*/
    //if not this do nothing

    return 0;
}