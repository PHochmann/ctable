TARGET_EXEC := test
BUILD_DIR   := ./bin
SRC_DIRS    := ./src ./tests

CFLAGS      := -MMD -MP -std=c99 -Wall -Wextra -Werror -pedantic -g3 -O0
LDFLAGS     := 

SRCS := $(shell find $(SRC_DIRS) -name *.c)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

all: $(BUILD_DIR)/$(TARGET_EXEC)
	./$(BUILD_DIR)/$(TARGET_EXEC)

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	@$(CC) $(OBJS) -o $@
	@echo Done. Placed executable at $(BUILD_DIR)/$(TARGET_EXEC)

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	echo Compiling $<
	$(CC) $(INC_FLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) -r ./bin

-include $(DEPS)
