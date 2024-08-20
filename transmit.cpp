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
#include "BloomFilter.h"
#include "Utils.h"

using namespace std;

string readFile(string filename);
void writeFile(string filename, vector<uint8_t> RxData);
vector<string> processInputFile(string input);


string readFile(string filename)
{
    ifstream fin;
    string data = "";
    fin.open(filename);
    if (!fin.is_open())
    {
        cout << "Input file failed to open.\n";
        return "Fail";
        exit(EXIT_FAILURE);
    }
    string line;
    while (getline(fin, line))
    {
        data += line;
    }
    fin.close();
    return data;
}

void writeFile(string filename, string TxData)
{
    ofstream fout;

    fout.open(filename);
    if (!fout.is_open())
    {
        cout << "Output file failed to open." << endl;
        exit(EXIT_FAILURE);
    }
    fout << TxData << endl;
    fout.close();
}


vector<string> processInputFile(string input)
{
    // Split input data by section
    string DDUID, TOPIC, DATA;
    DDUID = input.substr(input.find("DUID:"), input.find("TOPIC:")).substr(5);
    TOPIC = input.substr(input.find("TOPIC:"), input.find("DATA:") - input.find("TOPIC:")).substr(6);
    DATA = input.substr(input.find("DATA:"), input.find("DATE:") - input.find("DATA:")).substr(5);
    vector<string> processed = {DDUID, TOPIC, DATA};

    // Print Processed Data
    cout << "PROCESSED INPUT FILE:" << endl;
    cout << processed[0] << endl;
    cout << processed[1] << endl;
    cout << processed[2] << endl;
    cout << endl;
    return processed;
}

/*string modifystring (string cdp, int position) {
        char lower4;
        char upper4;
        if(((cdp[position] & 0x0F) + 48) > 57){
            lower4 = ((cdp[position] & 0x0F) + 55);
        }
        else{
            lower4 = ((cdp[position] & 0x0F) + 48);
        }
       
        if ((((cdp[position] & 0xF0) >> 4) + 48) > 57){
            upper4 = (((cdp[position] & 0xF0) >> 4) + 55);
        }
        else{
            upper4 = (((cdp[position] & 0xF0) >> 4) + 48);
        }

        cdp[position] = upper4;
        
        cdp.insert(position+1, 1, lower4);
        return cdp;
    }*/
string modifystring (string cdp, int position){
    /*if(cdp[position] > 9){
        cdp[position] = cdp[position] + 55;
    }
    else{
        cdp[position] = cdp[position] + 48;
    }*/
    //cdp[position] = 
    cdp[position] = cdp[position] + 32;
    return cdp;
}

int main()
{
    // Read from input file
    string inputData = readFile("infile.txt");
    vector<string> processed = processInputFile(inputData);
    vector<uint8_t> dduid = duckutils::convertStringToVector(processed[0]);
    uint8_t topic = Packet::stringToTopic(processed[1]);
    vector<uint8_t> data = duckutils::convertStringToVector(processed[2]);

    // Instantiate Packet Object
    Packet dp;

    // Set Duck ID
    dp.setDuckId(duckutils::convertStringToVector("DUCK0001"));

    //dp.setType(DuckType::LINK);

    // Create BloomFilter
    BloomFilter filter = BloomFilter(DEFAULT_NUM_SECTORS, DEFAULT_NUM_HASH_FUNCS, DEFAULT_BITS_PER_SECTOR, DEFAULT_MAX_MESSAGES);

    // Create formatted CDP packet in the buffer property of dp object
    dp.prepareForSending(&filter, dduid, topic, 1, 0x00, data);

    //gets payload generated
    vector<uint8_t> payload = dp.getBuffer();

    string CDP = duckutils::convertVectorToString(payload);

    cout << "CDP packet as a string: " << CDP << endl;

    //makes sure any unprintable characters in the payload come out as hex values
    for(int i = 0; i < 7; i=i+1)
    {
        CDP = modifystring(CDP, TOPIC_POS+i);
    }

    cout << "CDP packet as a string modified: " << CDP << endl;

    // Write formatted packet to output file in hex
    string cdpPacket = duckutils::convertToHex(dp.getBuffer().data(), dp.getBuffer().size()).c_str();
    cout << "CDP packet in hex: " << cdpPacket << endl;
    cout << "(SHOULD MATCH OUTFILE.TXT)" << endl;
    writeFile("forRadio.txt", CDP);


    return 0;
}