main: 
	g++ -O2 -g FileManager.cpp -c -o FileManager.o
	g++ -O2 -g main.cpp -c -o main.o
	g++ -O2 -g BPlusTree.cpp -c -o BPlusTree.o
	g++ -O2 main.o FileManager.o BPlusTree.o -o run.o

run:
	./run.o
	
