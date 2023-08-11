
SRC=$(wildcard src/*.cpp src/modules/*.cpp  base/*.cpp ./*.cpp )
OBJ=$(patsubst %.cpp,%.o,${SRC})

hg:${OBJ}
	g++ -g -o hg $^ -pthread
%.o:%.cpp
	g++ -g -c $< -o $@

clean:
	rm ${OBJ} hg
