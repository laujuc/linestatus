# LineStatus Makefile
# Build system for the LineStatus GTK application

CC := zig cc
CFLAGS := -Wall -Wextra -std=c11

# Main build configuration
LDFLAGS := `pkg-config --cflags --libs gtk4 gtk4-layer-shell-0`
SRC_DIR := src

# Target definitions
TARGET_MAIN := linestatus
TARGET_STATIC := linestatus-static

# Source files
SRC_MAIN := $(SRC_DIR)/main.c
SRC_STATIC := $(SRC_DIR)/main_static_volume.c

.PHONY: all clean run install

# Default target builds the main application
all: $(TARGET_MAIN)

# Main application target (interactive volume control)
$(TARGET_MAIN): $(SRC_MAIN) src/style.css
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Static volume application target
$(TARGET_STATIC): $(SRC_STATIC) src/style.css
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Clean both targets
clean:
	rm -f $(TARGET_MAIN) $(TARGET_STATIC)

# Run targets
run: $(TARGET_MAIN)
	./$(TARGET_MAIN)

run-static: $(TARGET_STATIC)
	./$(TARGET_STATIC)

# Install the main application
install:
	mkdir -p $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET_MAIN) $(DESTDIR)/usr/local/bin/linestatus