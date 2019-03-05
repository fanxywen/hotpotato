ringmaster: potato.h ringmaster.cpp
	g++ -std=gnu++11 -o ringmaster ringmaster.cpp
player: potato.h player.cpp
	g++ -std=gnu++11 -o player player.cpp
clean:
	rm -f *.c~ *.h~ *~
