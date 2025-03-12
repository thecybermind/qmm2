# QMM2 Makefile

include Config.mak

CC=g++

BASE_CFLAGS=-pipe -m32 -I ./include/

BROOT=linux
BR=$(BROOT)/release
BD=$(BROOT)/debug

OBJR=$(SRC_FILES:./src/%.cpp=$(BR)/%.o)
OBJD=$(SRC_FILES:./src/%.cpp=$(BD)/%.o)

DEBUG_CFLAGS=$(BASE_CFLAGS) -g -pg 
RELEASE_CFLAGS=$(BASE_CFLAGS) -O2 -fPIC -fomit-frame-pointer -ffast-math -falign-loops=2 -falign-jumps=2 -falign-functions=2 -fno-strict-aliasing -fstrength-reduce 

SHLIBCFLAGS=
#-fPIC
SHLIBLDFLAGS=-shared -m32

help:
	@echo QMM supports the following make rules:
	@echo release - builds release version
	@echo debug - builds debug version
	@echo clean - cleans all output files
	
release:
	@echo ---
	@echo --- building qmm2 \(release\)
	@echo ---
	$(MAKE) $(BR)/$(BINARY).so

	@echo ---
	@echo --- Release build complete.
	@echo ---
	@echo --- Binaries are in linux/release
	@echo ---

debug: $(BD)/$(BINARY).so

$(BR)/$(BINARY).so: $(BR) $(OBJR)
	$(CC) $(RELEASE_CFLAGS) $(SHLIBLDFLAGS) -o $@ $(OBJR)
  
$(BD)/$(BINARY).so: $(BD) $(OBJD)
	$(CC) $(DEBUG_CFLAGS) $(SHLIBLDFLAGS) -o $@ $(OBJD)

$(BR)/%.o: ./src/%.cpp $(HDR_FILES)
	$(CC) $(RELEASE_CFLAGS) $(FLAGS) $(SHLIBCFLAGS) -o $@ -c $<
  
$(BD)/%.o: ./src/%.cpp $(HDR_FILES)
	$(CC) $(DEBUG_CFLAGS) $(FLAGS) $(SHLIBCFLAGS) -o $@ -c $<

$(BR):
	# @if [ ! -d $(BROOT) ];then mkdir $(BROOT);fi
	@if [ ! -d $(@) ];then mkdir -p $@;fi

$(BD):
	# @if [ ! -d $(BROOT) ];then mkdir $(BROOT);fi
	@if [ ! -d $(@) ];then mkdir -p $@;fi
	
clean:
	@rm -rf $(BD) $(BR) $(BROOT)
