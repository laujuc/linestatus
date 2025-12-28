CC := zig cc
CFLAGS := -Wall -Wextra -std=c11
<<<<<<< HEAD
LDFLAGS := `pkg-config --cflags --libs gtk4 gtk4-layer-shell-0`
TARGET := linestatus
=======
LDFLAGS_WAYLAND := -lwayland-client
LDFLAGS_GTK := `pkg-config --cflags --libs gtk4 gtk4-layer-shell-0`
LDFLAGS_VOLUME := `pkg-config --cflags --libs gtk4 gtk4-layer-shell-0 libpipewire-0.3`
TARGET_WAYLAND := linestatus-wayland
TARGET_GTK := linestatus-gtk
TARGET_VOLUME_SIMPLE := linestatus-volume
TARGET_STATIC_VOLUME := linestatus
>>>>>>> dev/fix-click-through

SRC_DIR := src
SRC := $(SRC_DIR)/main.c

.PHONY: all clean run install

<<<<<<< HEAD
all: $(TARGET)
=======
all: $(TARGET_STATIC_VOLUME)
>>>>>>> dev/fix-click-through

$(TARGET): $(SRC) src/style.css
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
<<<<<<< HEAD
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
=======
	rm -f $(TARGET_STATIC_VOLUME)

run-wayland: $(TARGET_WAYLAND)
	./$(TARGET_WAYLAND)

run-gtk: $(TARGET_GTK)
	./$(TARGET_GTK)

run-volume: $(TARGET_VOLUME_SIMPLE)
	./$(TARGET_VOLUME_SIMPLE)

run:
	./$(TARGET_STATIC_VOLUME) &
>>>>>>> dev/fix-click-through

install:
	mkdir -p $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/linestatus