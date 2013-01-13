
CXX = g++
CXXFLAGS = -O2 -fomit-frame-pointer -funroll-loops -Wall -Wextra
#CXXFLAGS = -p -g -O0
LDFLAGS = -lSDL -lSDL_mixer -lSDL_net -lGL -lSDL_image
#LDFLAGS = -lSDL -lSDL_mixer -lSDL_net -lGL -lSDL_image -p -g -Wall -Wextra
CPPSOURCES = cutscene.cpp main.cpp options.cpp texops.cpp winlose.cpp \
	game.cpp multimenu.cpp title.cpp
CSOURCES = osc.c
OBJECTS = $(CPPSOURCES:.cpp=.o) $(CSOURCES:.c=.o)
NAME = euro1943-linux

all: $(NAME) server-linux

release: $(NAME) server-linux
	tar cvzf $(NAME).tar.gz --exclude=".svn" --transform "s,^,euro1943/," $(NAME) \
		bldg.txt changelog.txt obj.txt license.txt readme.txt \
		server-linux server.ini \
		audio/ \
		img/ \
		cutscene/ \
		maps/

server-linux:
	cd server && make

$(NAME): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)
	strip $(NAME)

clean:
	rm -f *.o $(NAME) $(NAME).tar.gz
	cd server && make clean

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $< -o $@

.c.o:
	$(CXX) -c $(CXXFLAGS) $< -o $@
