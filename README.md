To compile(assumes redis is already installed):

1) g++ main.cpp Utils.cpp Packet.cpp BloomFilter.cpp PapaDuck.cpp redis.cpp MamaDuck.cpp -o testBasicTransmitandReceive -lhiredis
2) ./testBasicTransmitandReceive
