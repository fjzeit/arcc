# Windows DPI Scaling Patterns and Best Practices

## Overview

This lode documents critical patterns and best practices for handling DPI scaling in Windows C++ applications using Direct2D/DirectWrite, based on real-world debugging of runtime DPI change issues.

## Core Problem Pattern

**Symptom**: Application works perfectly when started at any DPI scale, but fails to adapt properly when DPI changes while running.

**Root Cause**: Inconsistent DPI value sources throughout the application. Different parts of the code query DPI at different times, leading to stale or mismatched values during DPI transitions.

## Critical Insight: Single Source of Truth

The fundamental solution is maintaining a **single source of truth** for DPI values throughout the application lifecycle.

### Bad Pattern: Multiple DPI Sources
```cpp
// DON'T DO THIS - Multiple inconsistent DPI queries
void SomeFunction() {
    HDC hdc = GetDC(hWnd);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);  // Query 1
    ReleaseDC(hWnd, hdc);
    // ... use dpiX for calculations
}

void AnotherFunction() {
    HDC hdc = GetDC(hWnd);
    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);  // Query 2 - different timing!
    ReleaseDC(hWnd, hdc);
    // ... use dpiY for calculations
}
```

### Good Pattern: Centralized DPI Storage
```cpp
class MyApp {
private:
    float m_currentDpiX, m_currentDpiY;  // Single source of truth

public:
    void SetDPI(float dpiX, float dpiY) {
        m_currentDpiX = dpiX;
        m_currentDpiY = dpiY;
    }
    
    // All coordinate conversion uses stored values
    float PixelToDIP_X(int pixels) {
        return (float)pixels * DPI_REFERENCE / m_currentDpiX;
    }
};
```

## DPI Change Handling Sequence

When handling `WM_DPICHANGED`, the sequence must mirror the startup process:

```cpp
case WM_DPICHANGED:
{
    int newDpiX = LOWORD(wParam);
    int newDpiY = HIWORD(wParam);
    
    // 1. Window resizing (accept suggested width, calculate height)
    // 2. Store new DPI values FIRST
    m_currentDpiX = (float)newDpiX;
    m_currentDpiY = (float)newDpiY;
    
    // 3. Recreate text formats (DirectWrite handles DPI automatically)
    CreateTextFormats();
    
    // 4. Recreate render target with explicit DPI
    DiscardDeviceResources();
    CreateDeviceResources(hWnd, (float)newDpiX, (float)newDpiY);
    
    // 5. Invalidate cached measurements
    m_cachedMeasurements = {};
    
    // 6. Recalculate layout with new DPI
    int contentHeight = CalculateContentHeight();
    
    // 7. Final window resize to proper dimensions
    ResizeWindowToContent(contentHeight, newDpiY);
}
```

## DirectWrite DPI Scaling Truth

**Key Insight**: DirectWrite automatically handles DPI scaling when render targets are created with proper DPI values. Do NOT manually scale fonts by DPI ratio.

### Wrong Approach
```cpp
// DON'T DO THIS - Manual font scaling causes double-scaling
float fontSizeScaled = baseFontSize * (newDpi / oldDpi);
CreateTextFormat(..., fontSizeScaled, ...);
```

### Correct Approach
```cpp
// DO THIS - Let DirectWrite handle scaling automatically
CreateTextFormat(..., baseFontSize, ...);  // Same size always
CreateRenderTarget(..., dpiX, dpiY);       // DPI specified here
```

## Coordinate Conversion Patterns

### Consistent Conversion Functions
```cpp
// Always use stored DPI values, never query GetDeviceCaps
float PixelToDIP_X(int pixels) {
    return (float)pixels * DPI_REFERENCE / m_currentDpiX;
}

float PixelToDIP_Y(int pixels) {
    return (float)pixels * DPI_REFERENCE / m_currentDpiY;
}

int DIPToPixel_X(float dips) {
    return MulDiv((int)dips, (int)m_currentDpiX, DPI_REFERENCE);
}
```

### Mouse Hit Testing
```cpp
void OnMouseMove(int x, int y) {
    // Convert to DIP using stored DPI
    float dipX = PixelToDIP_X(x);
    float dipY = PixelToDIP_Y(y);
    
    // All button rectangles are in DIP coordinates
    bool isHovered = (dipX >= buttonRect.left && dipX <= buttonRect.right &&
                     dipY >= buttonRect.top && dipY <= buttonRect.bottom);
}
```

## Text Measurement and Layout

### Invalidation on DPI Changes
```cpp
struct CachedMeasurements {
    float iconWidth = 0;
    float textWidth = 0;
    float totalWidth = 0;
};

// Invalidate during DPI changes
case WM_DPICHANGED:
    m_cachedMeasurements = {};  // Force recalculation
    break;
```

### Text Bounding Rectangles
```cpp
// LESSON LEARNED: Give text more space than exact measurements
// DirectWrite can be finicky about exact-width rectangles
D2D1_RECT_F textRect = D2D1::RectF(
    startX, 
    textTop, 
    buttonRect.right,  // Full remaining width, not exact measurement
    textBottom
);

// Use left-aligned text format for proper positioning
textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
```

## Common Pitfalls and Solutions

### Pitfall 1: GetDeviceCaps at Different Times
**Problem**: Different functions call GetDeviceCaps at different moments during DPI transitions.
**Solution**: Store DPI once, use everywhere.

### Pitfall 2: Manual Font Scaling
**Problem**: Manually scaling font sizes by DPI ratio.
**Solution**: Let DirectWrite handle scaling via render target DPI.

### Pitfall 3: Mixing DPI Sources
**Problem**: Some calculations use render target DPI, others use GetDeviceCaps DPI.
**Solution**: Single DPI storage updated only during controlled moments.

### Pitfall 4: Text Wrapping in Buttons
**Problem**: Text rectangles sized exactly to measured width cause wrapping.
**Solution**: Give text the full available width with appropriate alignment.

### Pitfall 5: Stale Cached Values
**Problem**: Cached measurements not invalidated during DPI changes.
**Solution**: Systematic invalidation of all cached UI measurements.

## Window Height Calculation

```cpp
// CRITICAL: Calculate height AFTER text formats are recreated
case WM_DPICHANGED:
    // 1. Set window to suggested width first
    // 2. Recreate text formats with new DPI
    CreateTextFormats();
    CreateDeviceResources(hWnd, newDpiX, newDpiY);
    
    // 3. NOW calculate proper height with new font metrics
    int contentHeight = CalculateContentHeight();
    int clientHeight = MulDiv(contentHeight, newDpiY, DPI_REFERENCE);
    
    // 4. Resize to proper dimensions
```

## Best Practices Summary

1. **Single DPI Source**: Store DPI in class members, update only during controlled transitions
2. **Mirror Startup Process**: DPI change handling should follow same sequence as initialization
3. **Trust DirectWrite**: Don't manually scale fonts, let DirectWrite handle DPI via render target
4. **Invalidate Caches**: Systematically reset all cached measurements during DPI changes
5. **Generous Text Bounds**: Give text more space than exact measurements to prevent wrapping
6. **Consistent Conversion**: Use same DPI values for all pixel↔DIP conversions
7. **Proper Sequencing**: Recreate resources before recalculating layouts

## Testing Strategy

1. **Startup Test**: Verify app works correctly when started at various DPI scales (100%, 125%, 150%, 200%)
2. **Runtime Test**: Change DPI while app is running and verify all elements adapt correctly
3. **Rapid Changes**: Test multiple rapid DPI changes to catch timing issues
4. **Element Verification**: Check fonts, button sizes, hover zones, window height, and text positioning

This pattern successfully resolved complex DPI adaptation issues where the application worked perfectly at startup but failed during runtime DPI changes.