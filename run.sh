cd src/

g++ server.cpp -o server -lzmq
g++ client.cpp -o client -lzmq

chmod +x client

./server

rm server
rm client

cd ../
