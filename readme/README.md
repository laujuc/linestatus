# LineStatus - C Wayland Implementation

This is a C implementation of a Wayland-based system status indicator. LineStatus draws an orange vertical line on the right side of the screen as a basic visual indicator.

## Current Status

This is a **complete implementation** of a Wayland-based system status indicator. The current implementation:

- ✅ **Working**: GTK Layer Shell version with Unix socket support
- ✅ **Working**: Draws a colored line on the right side of the screen
- ✅ **Working**: Uses proper layer shell for Niri compatibility
- ✅ **Working**: Uses efficient Cairo rendering with transparency
- ✅ **Working**: Dynamic volume updates via Unix socket
- ✅ **Working**: Custom line colors via CLI arguments
- ✅ **Working**: Custom positioning (X,Y coordinates)
- ✅ **Working**: Horizontal/vertical orientation support
- ✅ **Working**: Multiple instance support with different types
- ✅ **Working**: Transparent background with click-through functionality
- ✅ **Working**: OpenRC service script for automatic startup

## Building

### Prerequisites

**For Wayland version:**
- GCC compiler
- Wayland development libraries (`libwayland-dev`)

**For GTK Layer Shell version (recommended for Niri):**
- GTK4 development libraries (`libgtk-4-dev`)
- GTK Layer Shell (`libgtk-layer-shell-dev`)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/linestatus.git
cd linestatus

# Build the working version
make

# Run the GTK version (recommended for Niri)
make run

# Or run directly
./linestatus-static-volume
```

## Project Structure

```
linestatus/
├── Makefile               # Makefile for building the application
├── src/
│   ├── main.c             # Main GTK Layer Shell implementation
│   ├── layer-shell-protocol.h  # Header for future raw layer shell support
│   └── style.css          # CSS for GTK styling
├── archive/backup_files/ # Archived experimental versions (not used)
├── send-status            # Script to send status updates via socket
├── linestatus-openrc      # OpenRC init script for service management
├── OPENRC_INSTRUCTIONS.md # OpenRC setup instructions
├── NIRI_COMPATIBILITY.md  # Niri-specific documentation
├── IMPLEMENTATION_SUMMARY.md  # Technical implementation details
├── README.md              # This file
└── style.css              # CSS for GTK styling (root level)
```

## Current Implementation

The current implementation is a **simple Wayland client** that draws a vertical line on the right side of the screen. It:

1. Connects to the Wayland display
2. Creates a surface using the compositor
3. Uses shared memory (SHM) for efficient rendering
4. Draws a 1-pixel wide line (1080 pixels height) on the right edge
5. Runs as a persistent Wayland client
6. **Supports custom line colors via CLI arguments**

The line is drawn using ARGB8888 format with full opacity. By default, it uses orange color (RGB: 255, 165, 0), but this can be customized.

### Command Line Options

```bash
# Show help
./linestatus --help

# Use default orange color and type
./linestatus

# Use custom colors (hex format RRGGBB)
./linestatus --color FF0000    # Red line
./linestatus --color 00FF00    # Green line
./linestatus --color 0000FF    # Blue line
./linestatus --color FFA500    # Orange line
./linestatus --color "#FF5733"  # Orange-red with # prefix

# Set socket type for different indicators
./linestatus --type volume      # Volume indicator (default)
./linestatus --type brightness  # Brightness indicator
./linestatus --type battery     # Battery indicator

# Set orientation (vertical or horizontal)
./linestatus --orientation horizontal  # Horizontal line (grows right)
./linestatus --orient vertical        # Vertical line (grows up) - default

# Combine all features
./linestatus --color 00FF00 --type volume --orientation horizontal
./linestatus --color FF0000 --type brightness --orient vertical
./linestatus --color 0000FF --type battery --orientation horizontal

# Alternative option names
./linestatus --line-color FF00FF --type volume  # Purple volume line
./linestatus --pos 100,200 --orient horizontal  # Custom position (experimental)
```

### OpenRC Service

LineStatus includes an OpenRC init script for automatic startup and service management:

```bash
# Install the service
sudo cp linestatus-openrc /etc/init.d/linestatus
sudo chmod +x /etc/init.d/linestatus
sudo rc-update add linestatus default

# Manage the service
sudo rc-service linestatus start
sudo rc-service linestatus stop
sudo rc-service linestatus status
```

See `OPENRC_INSTRUCTIONS.md` for detailed setup instructions and multiple instance configuration.

### Unix Socket Communication

The application creates a Unix socket at `$XDG_RUNTIME_DIR/linestatus-TYPE.sock` for dynamic updates:

```bash
# Send updates to default socket (type: status)
echo 60 > $XDG_RUNTIME_DIR/linestatus-status.sock
./send-status 60

# Send updates to specific sockets
./send-status --type volume 60        # Send to volume socket
echo 80 > $XDG_RUNTIME_DIR/linestatus-brightness.sock  # Send to brightness socket
./send-status --type battery 45      # Send to battery socket

# Multiple instances example
# Terminal 1: Volume indicator (red line)
./linestatus --color FF0000 --type volume

# Terminal 2: Brightness indicator (blue line)  
./linestatus --color 0000FF --type brightness

# Terminal 3: Send updates
./send-status --type volume 75
./send-status --type brightness 60
```

## Future Development Plan

### Phase 1: Layer Shell Integration
1. Add layer-shell protocol support for proper overlay positioning
2. Implement proper anchoring to screen edges
3. Add configuration for line position and color
4. Implement proper cleanup and error handling

### Phase 2: Volume Monitoring
1. Add PipeWire bindings for volume monitoring
2. Implement volume change detection
3. Connect volume data to line visualization (height/color)
4. Add smooth animations for volume changes

### Phase 3: Full Feature Implementation
1. Add configuration file support (JSON/YAML)
2. Implement backlight control integration
3. Add multiple visual indicators (volume, brightness, etc.)
4. Implement signal handling for dynamic reconfiguration
5. Add proper error recovery and robustness

## Development Resources

For implementing the full version, consider these resources:

- **Wayland Bindings**: [zig-wayland](https://github.com/ifreund/zig-wayland)
- **PipeWire**: Official PipeWire documentation and Zig bindings
- **Zig Documentation**: [https://ziglang.org/documentation/master/](https://ziglang.org/documentation/master/)
- **DeepWiki Zig**: [https://deepwiki.com/ziglang/zig](https://deepwiki.com/ziglang/zig)

## License

This project is licensed under the MIT License.

## Contributing

Contributions are welcome! This is currently a basic starting point. You can help by:

- Implementing Wayland integration
- Adding PipeWire volume monitoring
- Creating proper graphics rendering
- Adding configuration management
- Improving error handling and robustness

## Acknowledgements

- Original Hare version by the LineStatus authors
- Zig community for the excellent language and tools
- Wayland and PipeWire developers for the underlying technologies

---

**Note**: This is currently a basic starting point. The README has been updated to accurately reflect the current state of the project. The build system has been fixed to work with Zig 0.15.2.