# project name
TARGET = myshell

CC       = gcc
# compiling flags here
CFLAGS   = -I. -Wall #-std=c99

LINKER   = gcc -o
# linking flags here
LFLAGS   = -Wall -I. -lm

# change these to set the proper directories where each files shoould be
SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
RM       = rm 


$(BINDIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(BINDIR)
	@$(LINKER) $@ $(LFLAGS) $(OBJECTS)
	@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c $(INCLUDES)
	@mkdir -p $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONEY: clean
clean:
	@$(RM) -f $(OBJECTS)
	@$(RM) -rf $(OBJDIR)
	@echo "Cleanup complete!"

.PHONEY: remove
remove: clean
	@$(RM) -f  $(BINDIR)/$(TARGET)
	@$(RM) -rf $(BINDIR)
	@echo "Executable removed!"

