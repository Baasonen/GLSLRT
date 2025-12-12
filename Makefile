CC = gcc

SRC_DIR = src
INC_DIR = include
GLFW_INC = C:/libs/glfw/include/GLFW
GLFW_LIB = C:/libs/glfw/lib

SRC = $(wildcard $(SRC_DIR)/*.c)
OUT = a.exe

CFLAGS = -I$(INC_DIR) -I$(GLFW_INC)
LDFLAGS = -L$(GLFW_LIB)
LIBS = -lglfw3 -lopengl32 -lgdi32

$(OUT): $(SRC)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(OUT)

.PHONY: clean
clean:
	rm -f $(OUT)