# ARCC - Current Architecture

## Overview

ARCC is a modern Windows C++ application that automatically sends "RESUME" text to a target application at a scheduled time. The application features a custom borderless window with a dark theme, DirectWrite text rendering, and Direct2D graphics for modern visual presentation.

## Architecture

### Application Structure
- **Single-class design**: `ARCCApp` class manages all functionality
- **Custom window**: Borderless `WS_POPUP` window with `WS_THICKFRAME` for resizing and custom title bar
- **Direct2D/DirectWrite rendering**: Hardware-accelerated graphics with high-quality text rendering
- **Dynamic layout**: Precise DIP-based layout calculation with DPI awareness
- **Modern Windows integration**: Rounded corners (Windows 11), immersive dark mode, per-monitor DPI awareness

### Key Components

#### Window Management
- Custom window class registration (`ARCCMainWindow`)
- Borderless window with draggable custom title bar and interactive title bar buttons
- Direct2D/DirectWrite rendering pipeline with device resource management
- DPI-aware layout with pixel-to-DIP conversion functions
- Dynamic window sizing based on calculated content height
- Modern Windows styling with rounded corners and immersive dark mode

#### Target Selection System
- Low-level mouse hook (`WH_MOUSE_LL`) for window capture
- Process enumeration using `PROCESSENTRY32` and `CreateToolhelp32Snapshot`
- Automatic filtering of system processes (explorer, etc.)
- ESC key cancellation support

#### Timer System
- High-precision timing using `std::chrono::system_clock`
- Two-timer approach: countdown check (1s) and UI updates (1s) using `SetTimer`
- Hour-offset based scheduling (next 5 hours displayed as buttons)
- Automatic day rollover for past times
- Real-time countdown display integrated into button text
- System sleep prevention during active timer (`SetThreadExecutionState`)

#### Message Automation
- Keyboard simulation using `keybd_event`
- Virtual key mapping with `VkKeyScanA`
- Shift key state handling for uppercase characters
- Target window foreground activation before message sending

## Visual Design

### Rendering Technology
- **Direct2D**: Hardware-accelerated 2D graphics with anti-aliasing
- **DirectWrite**: High-quality text rendering with ClearType anti-aliasing
- **DPI Awareness**: Per-monitor DPI awareness v2 with automatic scaling
- **Device Resources**: Proper COM resource management with recreation on device loss

### Color Scheme (Direct2D ColorF)
- **Background**: `#191922` (dark purple-gray)
- **Text**: `#DDD` (light gray) 
- **Title bar**: `#2A2A2A` (medium gray)
- **Buttons**: `#2D2D3A` (normal), `#3D3D4A` (hover)
- **Green state**: `#7FB58A` (ready to start), `#8FC59A` (hover)
- **Red state**: `#DC7C7C` (timer active), `#EC8C8C` (hover)
- **Amber state**: `#E5BB6E` (target button highlight)
- **Button text on colored backgrounds**: `#191922` (dark)

### Typography (DirectWrite)
- **Main font**: Segoe UI, 16pt (body text, buttons)
- **Title font**: Segoe UI, 15pt (title bar)
- **Icon font**: Segoe MDL2 Assets, 16pt (UI icons)
- **Font weights**: Normal and Bold variants
- **Text alignment**: Multiple formats (center, leading, with paragraph alignment)
- **Line spacing**: 1.5x for improved readability

### Application States
```
enum class AppState {
    Idle,    // No target selected - gray button
    Ready,   // Target selected - green button
    Waiting  // Timer running - red button with countdown
}
```

## Key Features

### Custom Title Bar
- Draggable title bar with application icon, name, and subtitle
- Interactive buttons: Help (opens GitHub), Minimize, Close
- Hover effects with opacity-based visual feedback
- MDL2 icons for modern appearance
- Mouse capture for window dragging

### Dynamic Layout Engine (DIP-based)
- `CalculateDirectWriteTextHeight()` for precise DirectWrite text measurement
- Progressive positioning system with `LayoutData` structure
- DIP-to-pixel conversion with DPI awareness
- Cached layout calculations for performance
- Layout invalidation and recalculation on DPI changes
- Hour selection buttons with dynamic time calculation

### Button Visual States
- **Target button**: Shows selected application with process name, window title, and handle
- **Start button**: Icon + text combination with three visual states:
  - Idle: Gray background with "Select target window"
  - Ready: Green background with play icon + "Click to start"  
  - Waiting: Red background with live countdown "Resuming in HH:MM:SS"
- **Hour buttons**: 5 buttons showing next hour times (12-hour format with am/pm)
  - Normal: Gray background with white text
  - Selected: Green background with dark text
  - Hover: Green overlay with opacity

### Target Display Format
```
<process_name>
"<window_title or [No Title]>" (0xHANDLE)
```
- Title truncation at 35 characters with ellipsis
- Multi-line display in target button

## Technical Implementation

### DirectX Resource Management
- **Direct2D Factory**: Single-threaded factory for 2D graphics
- **DirectWrite Factory**: Shared factory for text rendering
- **Render Target**: HWND-based render target with proper DPI settings
- **Brush Management**: Consolidated solid color brushes with RAII cleanup
- **Text Formats**: Multiple DirectWrite text formats for different UI elements
- **Device Loss Handling**: Automatic resource recreation on device loss

### Timer Precision
- Hour-based scheduling with 5 selectable hour offsets from next hour
- Target time set to 10 seconds after the hour (for limit reset timing)
- Automatic day rollover for times beyond 24 hours
- Immediate UI feedback when timer starts
- Real-time countdown updates with formatted time display

### Memory Management
- **COM Resource Management**: SafeRelease template for proper COM object cleanup
- **RAII Pattern**: Automatic resource cleanup in destructor
- **Hook Management**: Proper installation and removal of mouse hooks
- **Device Resources**: Centralized creation and cleanup with device loss recovery
- **Memory Allocation**: Stack-based allocation for temporary data structures

### Error Handling
- **COM HRESULT Validation**: Proper checking of Direct2D/DirectWrite operations
- **Hook Installation Validation**: Error messages for mouse hook failures
- **Target Window Validation**: Checking window availability before message sending  
- **Device Loss Recovery**: Automatic recreation of DirectX resources
- **DPI Change Handling**: Dynamic resource recreation on monitor DPI changes

## Current State

The application is functionally complete with:
- **Modern DirectX-based UI**: Hardware-accelerated Direct2D/DirectWrite rendering
- **Advanced DPI Awareness**: Per-monitor DPI v2 with automatic scaling and layout recalculation
- **Hour-based Timer System**: Visual hour selection with real-time countdown display
- **Professional Visual Design**: Dark theme with hover effects, icons, and state-based colors
- **Robust Window Management**: Custom title bar with interactive buttons and window dragging
- **Clean Architecture**: Single-class design with proper resource management and error handling

The codebase demonstrates modern Windows development practices combining DirectX graphics, COM resource management, and Win32 APIs to create a responsive, visually appealing desktop application with professional-grade user experience.