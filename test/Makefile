# This makefile recursively finds, compiles, and links .c and .cpp files.
#
# The following targets are supported by this makefile. Indentations represent
# dependencies.
#
# all
# clean
#
# Defining VERBOSE before running make (i.e., VERBOSE=1 make) will print all
# commands ran.

EXEC := LiveGrapherTest

CC := gcc
CFLAGS := -O3 -Wall -s -std=c11 -flto

CPP := g++
CPPFLAGS := -O3 -Wall -s -std=c++17 -flto

# Specify defines with -D directives here
DEFINES :=

LD := g++

# Specify include paths with -I directives here
IFLAGS := -Ihost/include

# Specify libs with -l directives here
LDFLAGS := -pthread

SRCDIR := src
OBJDIR := build

# Make does not offer a recursive wildcard function, so here's one:
rwildcard=$(wildcard $1$2) $(foreach dir,$(wildcard $1*),$(call rwildcard,$(dir)/,$2))

# Recursively find all C source files
SRC_C := $(call rwildcard,$(SRCDIR)/,*.c) $(call rwildcard,host/,*.c)

# Recursively find all C++ source files
SRC_CPP := $(call rwildcard,$(SRCDIR)/,*.cpp) $(call rwildcard,host/,*.cpp)

# Create raw list of object files
C_OBJ := $(SRC_C:.c=.o)
CPP_OBJ := $(SRC_CPP:.cpp=.o)

# Create list of object files for build
C_OBJ := $(addprefix $(OBJDIR)/,$(C_OBJ))
CPP_OBJ := $(addprefix $(OBJDIR)/,$(CPP_OBJ))

.PHONY: all
all: $(OBJDIR)/$(EXEC)

-include $(C_OBJ:.o=.d) $(CPP_OBJ:.o=.d)

$(OBJDIR)/$(EXEC): $(C_OBJ) $(CPP_OBJ)
	@mkdir -p $(@D)
	@echo Linking $@
ifdef VERBOSE
	$(LD) $+ $(LDFLAGS) -o $@
else
	@$(LD) $+ $(LDFLAGS) -o $@
endif

# Pattern rule for building object file from C source
# The -MMD flag generates .d files to track changes in header files included in
# the source.
$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	@echo Building C object $@
ifdef VERBOSE
	$(CC) $(CFLAGS) $(DEFINES) $(IFLAGS) -MMD -c -o $@ $<
else
	@$(CC) $(CFLAGS) $(DEFINES) $(IFLAGS) -MMD -c -o $@ $<
endif

# Pattern rule for building object file from C++ source
# The -MMD flag generates .d files to track changes in header files included in
# the source.
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@echo Building CPP object $@
ifdef VERBOSE
	$(CPP) $(CPPFLAGS) $(DEFINES) $(IFLAGS) -MMD -c -o $@ $<
else
	@$(CPP) $(CPPFLAGS) $(DEFINES) $(IFLAGS) -MMD -c -o $@ $<
endif

# Cleans the build directory
.PHONY: clean
clean:
	@echo Removing object files
ifdef VERBOSE
	-$(RM) -r $(OBJDIR)/$(SRCDIR) $(OBJDIR)/host
	-$(RM) $(OBJDIR)/$(EXEC)
else
	-@$(RM) -r $(OBJDIR)/$(SRCDIR) $(OBJDIR)/host
	-@$(RM) $(OBJDIR)/$(EXEC)
endif
