cd ~/workspace/MAI_OS_3sem2024/src

g++ client.cpp -o client -lrt -lpthread
g++ server.cpp -o server -lrt -lpthread

chmod +x client
chmod +x server