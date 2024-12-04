all : mygit
mygit : main.cpp
		g++ -w main.cpp -o mygit -lssl -lcrypto -lz 
clean :
		rm -f mygit