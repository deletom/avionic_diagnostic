RM = rm -f
MAKE = make
COPY = cp
CC = gcc
export CC
AR = ar
PS = c
LIBEXT = a

# -pipe -static
CFLAGS = -Wall -Werror
export CFLAGS

RELEASE_FLAGS = -O2 -D_PRINT_HANDLE_TIME_

DEBUG_FLAGS = -pg -ggdb -D_DEBUG -D_PRINT_HANDLE_TIME_

RELEASE	= diagnostic

DEBUG	= $(RELEASE)_debug

TARGET = $(RELEASE) $(DEBUG)

DEPFLAGS = -MMD

LIBFLAGS =  rcs

ALL_INCLUDE = . ./include ./src

ALL_LIBS = hiredis

PATHS = . ./include ./src

SOURCES = $(foreach dir, $(PATHS), $(wildcard $(dir)/*.$(PS)))

OBJECTS_R = $(patsubst %.$(PS),%.o,$(SOURCES))

OBJECTS_D = $(patsubst %.$(PS),%d.o,$(SOURCES))

OBJS = $(OBJECTS_R) $(OBJECTS_D)

LIB_LINK = $(addprefix -l, $(ALL_LIBS))

DEPS := $(patsubst %.o, %.d, $(OBJS))

MISSING_DEPS := $(filter-out $(wildcard $(DEPS)),$(DEPS))

MISSING_DEPS_SOURCES := $(wildcard $(patsubst %.d,%.$(PS),$(MISSING_DEPS)))

INSTALL = ../../bin

.PHONY : all install deps objs clean veryclean debug release rebuild subdirs

all :   debug

debug : $(DEBUG)

install : debug
	@echo '----- copy new version -----'

release : debug $(RELEASE)
	@echo '----- copy new version -----'

$(DEBUG) : $(OBJECTS_D) $(addprefix -l,$(LIBS_D))
	$(CC) $(DEBUG_FLAGS) $(CFLAGS)   -Wno-deprecated -o $@  $^ $(LIB_LINK)

$(RELEASE) : $(OBJECTS_R)  $(addprefix -l,$(LIBS_R))
	$(CC) $(RELEASE_FLAGS) $(CFLAGS)  -Wno-deprecated -o $@  $^ $(LIB_LINK)

deps : $(DEPS)

objs : $(OBJS)

clean:
	@echo '----- cleaning obj file -----'
	@$(RM) $(OBJS)
	@echo '----- cleaning dependency file -----'
	@$(RM) $(DEPS)
	@echo '----- cleaning binary file -----'
	@$(RM) -f $(RELEASE)
	@$(RM) -f $(DEBUG)
	@$(RM) -f *.log *.out *.bz2 core.* tags	cscope.*

veryclean : clean cleansub
		@echo '----- cleaning execute file -----'
		@$(RM) $(TARGET)

rebuild: veryclean all

ifneq ($(MISSING_DEPS),)
$(MISSING_DEPS) : subdirs 
	@$(RM) $(patsubst %.d,%.o,$@)
endif

-include $(DEPS)

$(OBJECTS_D) : %d.o : %.$(PS)
	$(CC) $(DEPFLAGS) $(DEBUG_FLAGS) $(CFLAGS) $(addprefix -I,$(ALL_INCLUDE)) -c $< -o $@

$(OBJECTS_R) : %.o : %.$(PS)
	$(CC) $(DEPFLAGS) $(RELEASE_FLAGS) $(CFLAGS) $(addprefix -I,$(ALL_INCLUDE)) -c $< -o $@
