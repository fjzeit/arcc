# Windows Measurement Units and Coordinate Systems Guide

## Overview

This lode documents the different measurement units and coordinate systems encountered in Windows C++ development with Direct2D/DirectWrite, particularly focusing on DPI-aware applications. Understanding these units is crucial for proper scaling and layout calculations.

## Core Measurement Units

### 1. Physical Pixels
**Definition**: Actual pixels on the display hardware.
**Usage**: Raw mouse coordinates, GetSystemMetrics results.
**DPI Dependency**: Changes meaning based on display DPI.

```cpp
// Mouse coordinates arrive as physical pixels
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    case WM_MOUSEMOVE:
        int physicalX = GET_X_LPARAM(lParam);  // Physical pixels
        int physicalY = GET_Y_LPARAM(lParam);  // Physical pixels
        break;
}

// Screen metrics are physical pixels
int screenWidth = GetSystemMetrics(SM_CXSCREEN);   // Physical pixels
int screenHeight = GetSystemMetrics(SM_CYSCREEN);  // Physical pixels
```

### 2. Device Independent Pixels (DIPs)
**Definition**: Logical pixels that remain consistent across DPI changes.
**Standard**: 96 DPI reference (1 DIP = 1/96 inch).
**Usage**: Direct2D rendering, layout calculations, UI element sizing.

```cpp
// Constants defined in DIPs
static constexpr float BUTTON_HEIGHT = 46.0f;      // 46 DIPs
static constexpr float WINDOW_MARGIN = 20.0f;      // 20 DIPs
static constexpr float TITLEBAR_HEIGHT = 40.0f;    // 40 DIPs

// Direct2D rectangles use DIPs
D2D1_RECT_F buttonRect = D2D1::RectF(
    margin,                    // DIPs
    currentY,                  // DIPs
    margin + buttonWidth,      // DIPs
    currentY + BUTTON_HEIGHT   // DIPs
);
```

### 3. Font Points
**Definition**: Traditional typography unit (1 point = 1/72 inch).
**Usage**: Font size specifications in DirectWrite.
**DPI Relationship**: DirectWrite automatically converts points to appropriate pixel sizes based on render target DPI.

```cpp
// Font sizes in points
static constexpr float MAIN_FONT_SIZE = 16.0f;   // 16 points
static constexpr float TITLE_FONT_SIZE = 13.0f;  // 13 points

// DirectWrite handles DPI scaling automatically
CreateTextFormat(
    FONT_SEGOE_UI, 
    nullptr, 
    DWRITE_FONT_WEIGHT_NORMAL,
    DWRITE_FONT_STYLE_NORMAL, 
    DWRITE_FONT_STRETCH_NORMAL, 
    MAIN_FONT_SIZE,  // Points - DirectWrite scales this
    L"", 
    &textFormat
);
```

## Unit Conversion Formulas

### Physical Pixels ↔ DIPs
```cpp
#define DPI_REFERENCE 96  // Standard reference DPI

// Physical pixels to DIPs
float PixelToDIP_X(int pixels) {
    return (float)pixels * DPI_REFERENCE / m_currentDpiX;
}

float PixelToDIP_Y(int pixels) {
    return (float)pixels * DPI_REFERENCE / m_currentDpiY;
}

// DIPs to physical pixels
int DIPToPixel_X(float dips) {
    return MulDiv((int)dips, (int)m_currentDpiX, DPI_REFERENCE);
}

int DIPToPixel_Y(float dips) {
    return MulDiv((int)dips, (int)m_currentDpiY, DPI_REFERENCE);
}
```

### Windows API Helper (MulDiv)
```cpp
// MulDiv handles integer overflow and rounding properly
int scaledValue = MulDiv(originalValue, newDPI, DPI_REFERENCE);

// Equivalent to: (originalValue * newDPI) / DPI_REFERENCE
// But handles overflow and provides proper rounding
```

## Coordinate System Contexts

### 1. Win32 Window Management (Physical Pixels)
```cpp
// Window positioning and sizing
RECT windowRect = { 100, 100, 900, 700 };  // Physical pixels
CreateWindow(..., 
    windowRect.left,                        // Physical pixels
    windowRect.top,                         // Physical pixels  
    windowRect.right - windowRect.left,     // Physical pixels
    windowRect.bottom - windowRect.top,     // Physical pixels
    ...);

// Client area measurements
RECT clientRect;
GetClientRect(hWnd, &clientRect);  // Returns physical pixels
```

### 2. Direct2D Rendering (DIPs)
```cpp
// All Direct2D operations use DIPs
m_pRenderTarget->Clear(BG_COLOR);

// Rectangle drawing
D2D1_RECT_F rect = D2D1::RectF(10.0f, 20.0f, 200.0f, 80.0f);  // DIPs
m_pRenderTarget->FillRectangle(&rect, m_pBrush);

// Text rendering
D2D1_RECT_F textRect = D2D1::RectF(margin, currentY, 
                                   margin + width, currentY + height);  // DIPs
m_pRenderTarget->DrawText(text, textLength, textFormat, &textRect, brush);
```

### 3. Mouse Input Processing
```cpp
void ProcessMouseInput(int x, int y) {
    // Input arrives as physical pixels
    // Convert to DIPs for UI logic
    float dipX = PixelToDIP_X(x);
    float dipY = PixelToDIP_Y(y);
    
    // Compare with DIP-based button rectangles
    bool isHovered = (dipX >= buttonRect.left && dipX <= buttonRect.right &&
                     dipY >= buttonRect.top && dipY <= buttonRect.bottom);
}
```

## Critical Unit Boundaries

### Boundary 1: Input → Logic
**Input**: Physical pixels from mouse events
**Logic**: DIPs for UI calculations
**Conversion**: At event entry point

```cpp
case WM_LBUTTONDOWN:
    int physicalX = GET_X_LPARAM(lParam);
    int physicalY = GET_Y_LPARAM(lParam);
    
    // Convert immediately to DIPs for all logic
    float dipX = PixelToDIP_X(physicalX);
    float dipY = PixelToDIP_Y(physicalY);
    
    HandleClick(dipX, dipY);  // All internal logic uses DIPs
    break;
```

### Boundary 2: Logic → Rendering
**Logic**: DIPs for layout calculations  
**Rendering**: DIPs for Direct2D (no conversion needed)
**Key**: Both use same unit system

```cpp
// Layout calculation (DIPs)
float buttonY = currentY;
float buttonHeight = BUTTON_HEIGHT;  // DIPs
D2D1_RECT_F buttonRect = D2D1::RectF(margin, buttonY, 
                                     margin + width, buttonY + buttonHeight);

// Rendering (DIPs) - no conversion needed
m_pRenderTarget->FillRectangle(&buttonRect, brush);
```

### Boundary 3: Logic → Window Management
**Logic**: DIPs for content sizing
**Window**: Physical pixels for Win32 APIs
**Conversion**: When setting window dimensions

```cpp
// Calculate content height in DIPs
int contentHeight = CalculateContentHeight();  // Returns DIPs

// Convert to physical pixels for window sizing
int clientHeight = MulDiv(contentHeight, (int)m_currentDpiY, DPI_REFERENCE);

// Create window rectangle (physical pixels)
RECT windowRect = { 0, 0, clientWidth, clientHeight };
AdjustWindowRectEx(&windowRect, windowStyle, FALSE, extendedStyle);
```

## Common Unit Mix-up Patterns

### Anti-Pattern 1: Mixing Units in Calculations
```cpp
// DON'T DO THIS - mixing physical pixels and DIPs
float dipX = PixelToDIP_X(mouseX);           // DIPs
RECT clientRect;
GetClientRect(hWnd, &clientRect);            // Physical pixels
bool inBounds = (dipX < clientRect.right);   // WRONG - comparing DIPs to pixels
```

### Correct Pattern
```cpp
// DO THIS - convert to common unit system
float dipX = PixelToDIP_X(mouseX);                    // DIPs
RECT clientRect;
GetClientRect(hWnd, &clientRect);                     // Physical pixels
float clientWidth = PixelToDIP_X(clientRect.right);   // Convert to DIPs
bool inBounds = (dipX < clientWidth);                 // Correct - both DIPs
```

### Anti-Pattern 2: Double Conversion
```cpp
// DON'T DO THIS - converting already converted values
float dipX = PixelToDIP_X(mouseX);        // First conversion
float dipXAgain = PixelToDIP_X(dipX);     // WRONG - dipX is already DIPs
```

## DPI Scaling Behavior by Unit

### Physical Pixels
- **Startup**: Quantity changes with system DPI
- **Runtime DPI Change**: Immediate change in meaning
- **Usage**: Window dimensions, mouse coordinates

### DIPs (Device Independent Pixels)
- **Startup**: Consistent logical size regardless of DPI
- **Runtime DPI Change**: Logical meaning stays same, physical representation changes
- **Usage**: UI layout, Direct2D rendering

### Font Points
- **Startup**: DirectWrite scales to appropriate pixel size based on DPI
- **Runtime DPI Change**: DirectWrite automatically rescales when render target recreated
- **Usage**: Font size specifications

## Best Practices for Unit Management

### 1. Establish Clear Boundaries
```cpp
class UIElement {
    // Internal state always in DIPs
    D2D1_RECT_F m_bounds;     // DIPs
    float m_width, m_height;  // DIPs
    
    // Conversion methods at boundaries
    void SetFromPixels(int x, int y, int w, int h) {
        m_bounds.left = PixelToDIP_X(x);
        m_bounds.top = PixelToDIP_Y(y);
        m_bounds.right = m_bounds.left + PixelToDIP_X(w);
        m_bounds.bottom = m_bounds.top + PixelToDIP_Y(h);
    }
};
```

### 2. Use Descriptive Variable Names
```cpp
// Clear unit indication in variable names
int mousePhysicalX, mousePhysicalY;      // Physical pixels
float mouseDipX, mouseDipY;              // DIPs
int windowPixelWidth, windowPixelHeight; // Physical pixels
float contentDipWidth, contentDipHeight; // DIPs
```

### 3. Convert at System Boundaries
```cpp
// Convert at input boundary
case WM_MOUSEMOVE:
    int physicalX = GET_X_LPARAM(lParam);
    int physicalY = GET_Y_LPARAM(lParam);
    
    // Convert once at boundary
    float dipX = PixelToDIP_X(physicalX);
    float dipY = PixelToDIP_Y(physicalY);
    
    // All internal processing uses DIPs
    ProcessMouseMove(dipX, dipY);
    break;
```

### 4. Document Unit Expectations
```cpp
// Clear documentation of expected units
class LayoutManager {
public:
    // All parameters and return values in DIPs unless noted
    float CalculateButtonHeight();                    // Returns DIPs
    void SetButtonPosition(float dipX, float dipY);   // Parameters in DIPs
    D2D1_RECT_F GetButtonBounds();                    // Returns DIP rectangle
    
    // Explicit when using other units
    void SetWindowSizePixels(int pixelWidth, int pixelHeight);  // Physical pixels
};
```

## Debugging Unit Issues

### Visual Debugging
```cpp
// Add unit information to debug output
void DebugPrintCoordinates(const char* context, int physX, int physY) {
    float dipX = PixelToDIP_X(physX);
    float dipY = PixelToDIP_Y(physY);
    
    printf("%s: Physical(%d,%d) -> DIP(%.1f,%.1f) @ DPI(%.0f,%.0f)\n", 
           context, physX, physY, dipX, dipY, m_currentDpiX, m_currentDpiY);
}
```

### Unit Conversion Validation
```cpp
// Verify round-trip conversions
void ValidateConversion() {
    int originalPixel = 100;
    float dip = PixelToDIP_X(originalPixel);
    int backToPixel = DIPToPixel_X(dip);
    
    assert(abs(originalPixel - backToPixel) <= 1);  // Allow 1 pixel rounding error
}
```

## Key Takeaways

1. **Physical Pixels**: Input coordinates, window dimensions, system metrics
2. **DIPs**: UI layout, Direct2D rendering, logical measurements  
3. **Font Points**: Font size specifications (DirectWrite handles DPI scaling)
4. **Convert at Boundaries**: Input→DIPs, DIPs→Window sizing
5. **Single Unit Systems**: Keep calculations within one unit system
6. **Clear Documentation**: Variable names and function signatures should indicate units
7. **MulDiv for Safety**: Use MulDiv for integer scaling to handle overflow and rounding

Understanding these units and their boundaries is essential for creating DPI-aware Windows applications that work correctly across different display configurations and DPI changes.