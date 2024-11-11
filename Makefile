TARGET = main

CXX = g++
CXXFLAGS = -Wall -g
CXXFLAGS += -Ilib -Isrc -Isrc/shape

SRCS = main.cpp


$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)


clean:
	rm -f $(TARGET)