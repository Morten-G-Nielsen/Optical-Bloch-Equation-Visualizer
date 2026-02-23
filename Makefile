CXXFLAGS = -std=c++23 -Wall -Iinclude
LDFLAGS = -lglfw -lGL -ldl -lpthread

main: main.o glad.o
	$(CXX) main.o glad.o -o main $(LDFLAGS)

main.o: main.cc
	$(CXX) $(CXXFLAGS) -c main.cc -o main.o

glad.o: src/glad.c
	$(CXX) $(CXXFLAGS) -c src/glad.c -o glad.o

clean:
	$(RM) main *.o

run:
	./main
