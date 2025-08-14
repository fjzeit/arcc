#include <windows.h>
#include <tlhelp32.h>
#include <windowsx.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>
#include "resource.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

class ARCCApp {
private:
	enum class AppState {
		Idle,       // No target selected
		Ready,      // Target selected, ready to start
		Waiting     // Timer is running
	};
	HWND m_hTargetWindow;
	HHOOK m_hInputHook;
	bool m_bCapturing;
	bool m_bTimerActive;
	std::chrono::system_clock::time_point m_targetTime;
	std::string m_targetWindowTitle;
	std::string m_targetProcessName;
	int m_selectedHourOffset = 0;

	// Theme colors
	static const D2D1_COLOR_F BG_COLOR;
	static const D2D1_COLOR_F TEXT_COLOR;
	static const D2D1_COLOR_F BUTTON_COLOR;
	static const D2D1_COLOR_F BUTTON_HOVER_COLOR;
	static const D2D1_COLOR_F BUTTON_GREEN_COLOR;
	static const D2D1_COLOR_F BUTTON_GREEN_HOVER_COLOR;
	static const D2D1_COLOR_F BUTTON_RED_COLOR;
	static const D2D1_COLOR_F BUTTON_RED_HOVER_COLOR;
	static const D2D1_COLOR_F START_BUTTON_TEXT_COLOR;
	static const D2D1_COLOR_F TARGET_BUTTON_COLOR;
	static const D2D1_COLOR_F TITLEBAR_COLOR;
	static constexpr float TITLEBAR_HEIGHT = 40.0f;
	static constexpr float LINE_SPACING = 1.5f;

	// Layout constants
	static constexpr float WINDOW_MARGIN = 25.0f;
	static constexpr float ELEMENT_SPACING = 16.0f;
	static constexpr float WINDOW_WIDTH = 500.0f;
	static constexpr float CONTENT_WIDTH = 450.0f;

	// Button dimensions
	static constexpr float TARGET_BUTTON_HEIGHT = 54.0f;
	static constexpr float START_BUTTON_HEIGHT = 46.0f;
	static constexpr float HOUR_BUTTON_WIDTH = 80.0f;
	static constexpr float HOUR_BUTTON_HEIGHT = 32.0f;
	static constexpr float HOUR_BUTTON_SPACING = 10.0f;

	// Font sizes
	static constexpr float MAIN_FONT_SIZE = 16.0f;
	static constexpr float TITLE_FONT_SIZE = 15.0f;

	// UI constants
	static constexpr float BUTTON_TEXT_PADDING = 8.0f;
	static constexpr float BUTTON_TEXT_PADDING_V = 4.0f;
	static constexpr float BORDER_WIDTH = 1.0f;
	static constexpr int DPI_REFERENCE = 96;
	static constexpr int HOUR_COUNT = 5;
	static constexpr int TITLE_CHAR_LIMIT = 35;

	// String constants
	static constexpr const wchar_t* FONT_SEGOE_UI = L"Segoe UI";
	static constexpr const wchar_t* FONT_SEGOE_MDL2 = L"Segoe MDL2 Assets";
	static constexpr const char* APP_CLASS_NAME = "ARCCMainWindow";
	static constexpr const char* APP_WINDOW_TITLE = "ARCC";
	static constexpr const wchar_t* APP_TITLE_MAIN = L"ARCC";
	static constexpr const wchar_t* APP_TITLE_SUB = L"Auto Resume CC";
	static constexpr const wchar_t* INSTRUCTION_TEXT = L"Resume message will be sent to the application identified below. Click the target button and then click on the target application, or press ESC to cancel.";
	static constexpr const wchar_t* TAB_INFO_TEXT = L"The resume message will be sent to the application. If your application has a tabbed interface then make sure the correct tab is active.";
	static constexpr const wchar_t* START_INFO_TEXT = L"Click start button below to activate the resumer. When the limit reset time occurs the resume message will be sent to the selected application. If you close the target application before the timer expires the timer will be stopped.";
	static constexpr const wchar_t* ICON_MINIMIZE = L"\uE949";
	static constexpr const wchar_t* ICON_CLOSE = L"\uE8BB";
	static constexpr const wchar_t* ICON_PLAY = L"\uE768";
	static constexpr const wchar_t* ICON_HELP = L"\uE946";
	static constexpr const char* HELP_URL = "https://github.com/fjzeit/arcc";
	static constexpr const char* RESUME_MESSAGE = "RESUME";
	static constexpr const char* FILE_EXT_EXE = ".exe";
	static constexpr const char* PROCESS_EXPLORER = "explorer";
	static constexpr const char* PROCESS_ARCC = "arcc";

	// Button text constants
	static constexpr const wchar_t* BTN_TARGET_CAPTURE = L"Click on target window or ESC to cancel";
	static constexpr const wchar_t* BTN_TARGET_SELECT = L"Click to select target window";
	static constexpr const wchar_t* BTN_START_CLICK = L" Click to start";
	static constexpr const wchar_t* BTN_START_SELECT = L"Select target window";
	static constexpr const wchar_t* TITLE_NO_TITLE = L"[No Title]";
	static constexpr const wchar_t* TITLE_ELLIPSIS = L"...";

	// Error messages
	static constexpr const char* ERR_HOOK_FAILED = "Failed to install mouse hook";
	static constexpr const char* ERR_NO_TARGET = "Please select a target window first";
	static constexpr const char* ERR_TARGET_GONE = "Target window is no longer available";
	static constexpr const char* ERR_TITLE = "Error";
	static constexpr const char* WARN_TITLE = "Warning";

	// Direct2D resources
	ID2D1Factory* m_pD2DFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;

	// Consolidated brushes
	ID2D1SolidColorBrush* m_pBgBrush = nullptr;
	ID2D1SolidColorBrush* m_pTextBrush = nullptr;
	ID2D1SolidColorBrush* m_pButtonBrush = nullptr;
	ID2D1SolidColorBrush* m_pButtonHoverBrush = nullptr;
	ID2D1SolidColorBrush* m_pGreenBrush = nullptr;
	ID2D1SolidColorBrush* m_pGreenHoverBrush = nullptr;
	ID2D1SolidColorBrush* m_pRedBrush = nullptr;
	ID2D1SolidColorBrush* m_pRedHoverBrush = nullptr;
	ID2D1SolidColorBrush* m_pAmberBrush = nullptr;
	ID2D1SolidColorBrush* m_pTitleBarBrush = nullptr;
	ID2D1SolidColorBrush* m_pWhiteBrush = nullptr;

	// DirectWrite resources
	IDWriteFactory* m_pDWriteFactory = nullptr;
	IDWriteTextFormat* m_pTextFormat = nullptr;
	IDWriteTextFormat* m_pTitleTextFormat = nullptr;
	IDWriteTextFormat* m_pButtonTextFormat = nullptr;
	IDWriteTextFormat* m_pBoldTextFormat = nullptr;
	IDWriteTextFormat* m_pIconTextFormat = nullptr;
	IDWriteTextFormat* m_pBoldIconTextFormat = nullptr;
	IDWriteTextFormat* m_pBoldLeftTextFormat = nullptr;

	// Custom window members
	HWND m_hMainWindow;
	bool m_bDragging;
	POINT m_dragOffset{};
	HBRUSH m_hBackgroundBrush;
	bool m_bWindowActive;
	float m_currentDpiX, m_currentDpiY;

	// Mouse tracking
	POINT m_mousePos;
	bool m_bMouseTracking;

	// Title bar button hover tracking
	enum class TitleBarHover {
		None,
		Help,
		Minimize,
		Close
	} m_titleBarHover = TitleBarHover::None;

	static ARCCApp* s_pInstance;

	// Helper function for safe COM release
	template<class Interface>
	inline void SafeRelease(Interface** ppInterfaceToRelease) {
		if (*ppInterfaceToRelease != nullptr) {
			(*ppInterfaceToRelease)->Release();
			(*ppInterfaceToRelease) = nullptr;
		}
	}

	HRESULT CreateSingleTextFormat(const wchar_t* fontFamily, DWRITE_FONT_WEIGHT weight, float fontSize, IDWriteTextFormat** textFormat) {
		return m_pDWriteFactory->CreateTextFormat(
			fontFamily, nullptr, weight,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			fontSize, L"", textFormat);
	}

	void ConfigureTextFormat(IDWriteTextFormat* textFormat, DWRITE_TEXT_ALIGNMENT textAlign, DWRITE_PARAGRAPH_ALIGNMENT paraAlign) {
		if (textFormat) {
			textFormat->SetTextAlignment(textAlign);
			textFormat->SetParagraphAlignment(paraAlign);
			textFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_DEFAULT, LINE_SPACING, 0.8f);
		}
	}

	float PixelToDIP_X(int pixels) const {
		return (float)pixels * DPI_REFERENCE / m_currentDpiX;
	}

	float PixelToDIP_Y(int pixels) const {
		return (float)pixels * DPI_REFERENCE / m_currentDpiY;
	}

	int DIPToPixel_X(float dips) const {
		return MulDiv(static_cast<int>(dips), static_cast<int>(m_currentDpiX), DPI_REFERENCE);
	}

	int DIPToPixel_X(int dips) const { return DIPToPixel_X(static_cast<float>(dips)); }

	int DIPToPixel_Y(float dips) const {
		return MulDiv(static_cast<int>(dips), static_cast<int>(m_currentDpiY), DPI_REFERENCE);
	}

	int DIPToPixel_Y(int dips) const { return DIPToPixel_Y(static_cast<float>(dips)); }

	// Timer management helper function
	void StopTimer() {
		if (m_bTimerActive) {
			KillTimer(m_hMainWindow, TIMER_COUNTDOWN);
			KillTimer(m_hMainWindow, TIMER_STATUS_UPDATE);
			m_bTimerActive = false;

			// Allow system sleep again
			SetThreadExecutionState(ES_CONTINUOUS);
		}
	}

	// Title bar layout
	struct TitleBarButtonPositions {
		float helpButtonX;
		float minimizeButtonX;
		float closeButtonX;
		float buttonY;
		float buttonWidth;
		float buttonHeight;
	};

	// Start button layout
	struct StartButtonMeasurements {
		float iconWidth;
		float textWidth;
		float totalWidth;
		float startX;
	};

	// All the layout
	struct LayoutData {
		// Content positioning
		float contentTop = 0;
		float textWidth = 0;
		float spacing = 0;

		// Text blocks with positions and heights
		struct TextBlock {
			D2D1_RECT_F rect;
			float height;
		};

		TextBlock instructionText{};
		TextBlock tabInfoText{};
		TextBlock startInfoText{};

		// Button rectangles
		D2D1_RECT_F targetButtonRect{};
		D2D1_RECT_F startButtonRect{};
		D2D1_RECT_F hourButtonRects[HOUR_COUNT]{};

		// Start button detailed measurements
		StartButtonMeasurements startButtonMeasurements{};

		// Total calculated height
		float totalContentHeight = 0;

		// Validity flag
		bool isValid;

		LayoutData() : isValid(false) {}
	};

	TitleBarButtonPositions m_titleBarButtonPositions{};
	LayoutData m_layoutData = {};

	TitleBarButtonPositions CalculateTitleBarButtonPositions(HWND hWnd) {
		TitleBarButtonPositions pos = {};
		pos.buttonWidth = TITLEBAR_HEIGHT;
		pos.buttonHeight = TITLEBAR_HEIGHT;
		pos.buttonY = 0.0f;

		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		float windowWidth = PixelToDIP_X(clientRect.right);

		pos.closeButtonX = windowWidth - pos.buttonWidth;
		pos.minimizeButtonX = pos.closeButtonX - pos.buttonWidth;
		pos.helpButtonX = pos.minimizeButtonX - pos.buttonWidth;

		return pos;
	}

	void UpdateTitleBarButtonPositions(HWND hWnd) {
		m_titleBarButtonPositions = CalculateTitleBarButtonPositions(hWnd);
	}

	StartButtonMeasurements CalculateStartButtonMeasurements(const D2D1_RECT_F& textRect) {
		StartButtonMeasurements measurements = {};

		std::wstring iconText = ICON_PLAY;
		std::wstring mainText = BTN_START_CLICK;

		IDWriteTextLayout* pIconLayout = nullptr;
		IDWriteTextLayout* pTextLayout = nullptr;

		if (m_pDWriteFactory && m_pIconTextFormat) {
			HRESULT hr = m_pDWriteFactory->CreateTextLayout(
				iconText.c_str(), static_cast<UINT32>(iconText.length()),
				m_pIconTextFormat, 1000.0f, textRect.bottom - textRect.top, &pIconLayout);
			if (SUCCEEDED(hr) && pIconLayout) {
				DWRITE_TEXT_METRICS iconMetrics;
				pIconLayout->GetMetrics(&iconMetrics);
				measurements.iconWidth = iconMetrics.width;
				pIconLayout->Release();
			}
		}

		if (m_pDWriteFactory && m_pBoldTextFormat) {
			HRESULT hr = m_pDWriteFactory->CreateTextLayout(
				mainText.c_str(), static_cast<UINT32>(mainText.length()),
				m_pBoldTextFormat, 1000.0f, textRect.bottom - textRect.top, &pTextLayout);
			if (SUCCEEDED(hr) && pTextLayout) {
				DWRITE_TEXT_METRICS textMetrics;
				pTextLayout->GetMetrics(&textMetrics);
				measurements.textWidth = textMetrics.width;
				pTextLayout->Release();
			}
		}

		measurements.totalWidth = measurements.iconWidth + measurements.textWidth;
		float buttonWidth = textRect.right - textRect.left;
		measurements.startX = textRect.left + (buttonWidth - measurements.totalWidth) / 2.0f;

		return measurements;
	}


	void CreateTextFormats() {
		if (!m_pDWriteFactory) return;

		// Create text formats
		CreateSingleTextFormat(FONT_SEGOE_UI, DWRITE_FONT_WEIGHT_NORMAL, MAIN_FONT_SIZE, &m_pTextFormat);
		CreateSingleTextFormat(FONT_SEGOE_UI, DWRITE_FONT_WEIGHT_NORMAL, TITLE_FONT_SIZE, &m_pTitleTextFormat);
		CreateSingleTextFormat(FONT_SEGOE_UI, DWRITE_FONT_WEIGHT_NORMAL, MAIN_FONT_SIZE, &m_pButtonTextFormat);
		CreateSingleTextFormat(FONT_SEGOE_UI, DWRITE_FONT_WEIGHT_BOLD, MAIN_FONT_SIZE, &m_pBoldTextFormat);
		CreateSingleTextFormat(FONT_SEGOE_UI, DWRITE_FONT_WEIGHT_BOLD, MAIN_FONT_SIZE, &m_pBoldLeftTextFormat);
		CreateSingleTextFormat(FONT_SEGOE_MDL2, DWRITE_FONT_WEIGHT_NORMAL, MAIN_FONT_SIZE, &m_pIconTextFormat);
		CreateSingleTextFormat(FONT_SEGOE_MDL2, DWRITE_FONT_WEIGHT_BOLD, MAIN_FONT_SIZE, &m_pBoldIconTextFormat);

		// Configure text formats
		ConfigureTextFormat(m_pTextFormat, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		ConfigureTextFormat(m_pButtonTextFormat, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		ConfigureTextFormat(m_pBoldTextFormat, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		ConfigureTextFormat(m_pBoldLeftTextFormat, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		ConfigureTextFormat(m_pTitleTextFormat, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		ConfigureTextFormat(m_pIconTextFormat, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		ConfigureTextFormat(m_pBoldIconTextFormat, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	HRESULT CreateDeviceResources(HWND hWnd, float dpiX = 0, float dpiY = 0) {
		if (m_pRenderTarget) return S_OK; // Already created

		if (!m_pD2DFactory) return E_FAIL;

		RECT rc;
		GetClientRect(hWnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top
		);

		// During startup will yield 0 dpi values, so default to what we already know
		if (dpiX == 0 || dpiY == 0) {
			dpiX = m_currentDpiX;
			dpiY = m_currentDpiY;
		}

		// Store DPI values as our trusted source
		m_currentDpiX = dpiX;
		m_currentDpiY = dpiY;

		// Create render target
		HRESULT hr = m_pD2DFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_DEFAULT,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
				dpiX,
				dpiY,
				D2D1_RENDER_TARGET_USAGE_NONE,
				D2D1_FEATURE_LEVEL_DEFAULT
			),
			D2D1::HwndRenderTargetProperties(hWnd, size),
			&m_pRenderTarget
		);

		if (FAILED(hr)) return hr;

		// Set anti-aliasing modes
		m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Crisp lines and shapes
		m_pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE); // Good text

		// Create brushes
		hr = m_pRenderTarget->CreateSolidColorBrush(BG_COLOR, &m_pBgBrush);
		if (FAILED(hr)) return hr;

		hr = m_pRenderTarget->CreateSolidColorBrush(TEXT_COLOR, &m_pTextBrush);
		if (FAILED(hr)) return hr;

		hr = m_pRenderTarget->CreateSolidColorBrush(BUTTON_COLOR, &m_pButtonBrush);
		if (FAILED(hr)) return hr;

		hr = m_pRenderTarget->CreateSolidColorBrush(BUTTON_HOVER_COLOR, &m_pButtonHoverBrush);
		if (FAILED(hr)) return hr;

		hr = m_pRenderTarget->CreateSolidColorBrush(BUTTON_GREEN_COLOR, &m_pGreenBrush);
		if (FAILED(hr)) return hr;

		hr = m_pRenderTarget->CreateSolidColorBrush(BUTTON_GREEN_HOVER_COLOR, &m_pGreenHoverBrush);
		if (FAILED(hr)) return hr;

		hr = m_pRenderTarget->CreateSolidColorBrush(BUTTON_RED_COLOR, &m_pRedBrush);
		if (FAILED(hr)) return hr;

		hr = m_pRenderTarget->CreateSolidColorBrush(BUTTON_RED_HOVER_COLOR, &m_pRedHoverBrush);
		if (FAILED(hr)) return hr;

		hr = m_pRenderTarget->CreateSolidColorBrush(TARGET_BUTTON_COLOR, &m_pAmberBrush);
		if (FAILED(hr)) return hr;

		hr = m_pRenderTarget->CreateSolidColorBrush(TITLEBAR_COLOR, &m_pTitleBarBrush);
		if (FAILED(hr)) return hr;

		hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f), &m_pWhiteBrush);
		if (FAILED(hr)) return hr;

		return S_OK;
	}

	void DiscardDeviceResources() {
		// Release consolidated brushes
		SafeRelease(&m_pBgBrush);
		SafeRelease(&m_pTextBrush);
		SafeRelease(&m_pButtonBrush);
		SafeRelease(&m_pButtonHoverBrush);
		SafeRelease(&m_pGreenBrush);
		SafeRelease(&m_pGreenHoverBrush);
		SafeRelease(&m_pRedBrush);
		SafeRelease(&m_pRedHoverBrush);
		SafeRelease(&m_pAmberBrush);
		SafeRelease(&m_pTitleBarBrush);
		SafeRelease(&m_pWhiteBrush);
		SafeRelease(&m_pRenderTarget);
	}

	void ApplyModernWindowStyling() const {
		if (!m_hMainWindow) return;

		// Enable Windows 11 rounded corners
		DWM_WINDOW_CORNER_PREFERENCE cornerPref = DWMWCP_ROUND;
		DwmSetWindowAttribute(m_hMainWindow, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));

		// Enable DWM border rendering for active/inactive state
		BOOL useImmersiveDarkMode = TRUE;
		DwmSetWindowAttribute(m_hMainWindow, DWMWA_USE_IMMERSIVE_DARK_MODE, &useImmersiveDarkMode, sizeof(useImmersiveDarkMode));
	}


	// Calculate position of ui elements
	void CalculateLayout(float overrideClientWidthDIP = 0.0f) {
		// Reset validity
		m_layoutData.isValid = false;

		// Calculate content width - either use override or convert from pixels to DIP
		float clientWidthDIP;
		if (overrideClientWidthDIP > 0.0f) {
			clientWidthDIP = overrideClientWidthDIP;
		}
		else {
			RECT clientRect;
			GetClientRect(m_hMainWindow, &clientRect);
			clientWidthDIP = PixelToDIP_X(clientRect.right - clientRect.left);
		}

		// Setup basic layout parameters
		m_layoutData.contentTop = WINDOW_MARGIN + TITLEBAR_HEIGHT;
		m_layoutData.textWidth = clientWidthDIP - (2 * WINDOW_MARGIN);
		m_layoutData.spacing = ELEMENT_SPACING;

		float currentY = m_layoutData.contentTop;
		float margin = WINDOW_MARGIN;
		float textWidth = m_layoutData.textWidth;
		float spacing = m_layoutData.spacing;

		// Calculate instruction text block
		m_layoutData.instructionText.height = CalculateDirectWriteTextHeight(INSTRUCTION_TEXT, textWidth, m_pTextFormat);
		m_layoutData.instructionText.rect = D2D1::RectF(margin, currentY, margin + textWidth, currentY + m_layoutData.instructionText.height);
		currentY += m_layoutData.instructionText.height + spacing;

		// Calculate target button
		m_layoutData.targetButtonRect = D2D1::RectF(margin, currentY, margin + textWidth, currentY + TARGET_BUTTON_HEIGHT);
		currentY += TARGET_BUTTON_HEIGHT + spacing;

		// Calculate tab info text block
		m_layoutData.tabInfoText.height = CalculateDirectWriteTextHeight(TAB_INFO_TEXT, textWidth, m_pTextFormat);
		m_layoutData.tabInfoText.rect = D2D1::RectF(margin, currentY, margin + textWidth, currentY + m_layoutData.tabInfoText.height);
		currentY += m_layoutData.tabInfoText.height + spacing;

		// Calculate hour buttons
		for (int i = 0; i < HOUR_COUNT; i++) {
			float buttonX = margin + i * (HOUR_BUTTON_WIDTH + HOUR_BUTTON_SPACING);
			m_layoutData.hourButtonRects[i] = D2D1::RectF(buttonX, currentY, buttonX + HOUR_BUTTON_WIDTH, currentY + HOUR_BUTTON_HEIGHT);
		}
		currentY += HOUR_BUTTON_HEIGHT + spacing;

		// Calculate start info text block
		m_layoutData.startInfoText.height = CalculateDirectWriteTextHeight(START_INFO_TEXT, textWidth, m_pTextFormat);
		m_layoutData.startInfoText.rect = D2D1::RectF(margin, currentY, margin + textWidth, currentY + m_layoutData.startInfoText.height);
		currentY += m_layoutData.startInfoText.height + spacing;

		// Calculate start button
		m_layoutData.startButtonRect = D2D1::RectF(margin, currentY, margin + textWidth, currentY + START_BUTTON_HEIGHT);

		// Calculate start button detailed measurements
		m_layoutData.startButtonMeasurements = CalculateStartButtonMeasurements(m_layoutData.startButtonRect);

		currentY += START_BUTTON_HEIGHT + WINDOW_MARGIN; // Bottom margin

		// Store total height
		m_layoutData.totalContentHeight = currentY;

		// Mark as valid
		m_layoutData.isValid = true;
	}

	// Window height is set to fit the UI elements
	int GetCalculatedContentHeight(float overrideClientWidthDIP = 0.0f) {
		if (!m_layoutData.isValid || overrideClientWidthDIP > 0.0f) {
			CalculateLayout(overrideClientWidthDIP);
		}
		return static_cast<int>(m_layoutData.totalContentHeight);
	}

	// Draw all the things
	void DrawMainContent() {
		if (!m_pRenderTarget || !m_pTextFormat) return;

		// Ensure layout is calculated
		if (!m_layoutData.isValid) {
			CalculateLayout();
		}

		// Draw instruction text
		m_pRenderTarget->DrawText(INSTRUCTION_TEXT, static_cast<UINT32>(wcslen(INSTRUCTION_TEXT)),
			m_pTextFormat, &m_layoutData.instructionText.rect, m_pTextBrush);

		// Draw target button
		DrawButton(m_layoutData.targetButtonRect, true, false);

		// Draw tab info text
		m_pRenderTarget->DrawText(TAB_INFO_TEXT, static_cast<UINT32>(wcslen(TAB_INFO_TEXT)),
			m_pTextFormat, &m_layoutData.tabInfoText.rect, m_pTextBrush);

		// Draw hour buttons
		std::vector<std::wstring> hourTimes = GetNext5HourTimes();
		for (int i = 0; i < HOUR_COUNT; i++) {
			const D2D1_RECT_F& buttonRect = m_layoutData.hourButtonRects[i];

			// Check if mouse is hovering over this button
			bool isHovered = false;
			if (m_mousePos.x >= 0 && m_mousePos.y >= 0) {
				// Convert mouse coordinates to DIP using stored values
				float dipX = PixelToDIP_X(m_mousePos.x);
				float dipY = PixelToDIP_Y(m_mousePos.y);

				isHovered = (dipX >= buttonRect.left && dipX <= buttonRect.right &&
					dipY >= buttonRect.top && dipY <= buttonRect.bottom);
			}

			// Use different colors for selected vs unselected
			ID2D1SolidColorBrush* buttonBrush = (i == m_selectedHourOffset) ? m_pGreenBrush : m_pButtonBrush;
			ID2D1SolidColorBrush* textBrush = (i == m_selectedHourOffset) ? m_pBgBrush : m_pTextBrush;

			m_pRenderTarget->FillRectangle(&buttonRect, buttonBrush);

			// Add green hover effect with opacity (only if not already selected)
			if (isHovered && i != m_selectedHourOffset) {
				m_pGreenBrush->SetOpacity(0.3f);
				m_pRenderTarget->FillRectangle(&buttonRect, m_pGreenBrush);
				m_pGreenBrush->SetOpacity(1.0f); // Reset opacity
			}

			// Draw hour text centered in button
			m_pRenderTarget->DrawText(hourTimes[i].c_str(), static_cast<UINT32>(hourTimes[i].length()),
				m_pButtonTextFormat, &buttonRect, textBrush);
		}

		// Draw start info text
		m_pRenderTarget->DrawText(START_INFO_TEXT, static_cast<UINT32>(wcslen(START_INFO_TEXT)),
			m_pTextFormat, &m_layoutData.startInfoText.rect, m_pTextBrush);

		// Draw start/stop button
		DrawButton(m_layoutData.startButtonRect, false, true); // isTargetButton = false, isStartButton = true
	}

	// Calculates the height of text if rendered at the provided width
	float CalculateDirectWriteTextHeight(const wchar_t* text, float width, IDWriteTextFormat* textFormat) {
		if (!m_pDWriteFactory || !textFormat) return 20.0f;

		IDWriteTextLayout* pTextLayout = nullptr;
		HRESULT hr = m_pDWriteFactory->CreateTextLayout(
			text, static_cast<UINT32>(wcslen(text)), textFormat,
			width, 1000.0f, &pTextLayout);

		if (FAILED(hr) || !pTextLayout) return 20.0f;

		DWRITE_TEXT_METRICS textMetrics;
		pTextLayout->GetMetrics(&textMetrics);

		float height = textMetrics.height;

		pTextLayout->Release();
		return height;
	}

	// Draws either start or target button could refactor this as it's bit long-winded
	void DrawButton(const D2D1_RECT_F& rect, bool isTargetButton, bool isStartButton) {
		if (!m_pRenderTarget) return;

		AppState currentState = GetCurrentAppState();

		// Check if mouse is hovering over this button
		bool isHovered = false;
		if (m_mousePos.x >= 0 && m_mousePos.y >= 0) {
			float dipX = PixelToDIP_X(m_mousePos.x);
			float dipY = PixelToDIP_Y(m_mousePos.y);

			isHovered = (dipX >= rect.left && dipX <= rect.right &&
				dipY >= rect.top && dipY <= rect.bottom);
		}

		// Choose button colors
		ID2D1SolidColorBrush* pButtonBrush = m_pButtonBrush;
		ID2D1SolidColorBrush* pTextBrush = m_pTextBrush;

		if (isTargetButton) {
			if (m_hTargetWindow) {
				pButtonBrush = m_pBgBrush;
				pTextBrush = m_pGreenBrush;
			}
			else {
				pButtonBrush = isHovered ? m_pButtonHoverBrush : m_pBgBrush;
				pTextBrush = m_pAmberBrush;
			}
		}
		else if (isStartButton) {
			switch (currentState) {
			case AppState::Waiting:
				pButtonBrush = isHovered ? m_pRedHoverBrush : m_pRedBrush;
				pTextBrush = m_pBgBrush;
				break;
			case AppState::Ready:
				pButtonBrush = isHovered ? m_pGreenHoverBrush : m_pGreenBrush;
				pTextBrush = m_pBgBrush;
				break;
			case AppState::Idle:
			default:
				pButtonBrush = m_pBgBrush;
				pTextBrush = m_pButtonBrush;
				break;
			}
		}

		// Draw button background
		m_pRenderTarget->FillRectangle(&rect, pButtonBrush);

		// Draw border for target button
		if (isTargetButton) {
			if (m_hTargetWindow) {
				m_pRenderTarget->DrawRectangle(&rect, m_pGreenBrush, BORDER_WIDTH);
			}
			else {
				m_pRenderTarget->DrawRectangle(&rect, m_pAmberBrush, BORDER_WIDTH);
			}
		}

		// Draw button text
		D2D1_RECT_F textRect = D2D1::RectF(
			rect.left + BUTTON_TEXT_PADDING, rect.top + BUTTON_TEXT_PADDING_V,
			rect.right - BUTTON_TEXT_PADDING, rect.bottom - BUTTON_TEXT_PADDING_V);

		std::wstring buttonText = GetButtonText(isTargetButton, isStartButton);

		if (isTargetButton && m_hTargetWindow && buttonText.find(L'\n') != std::wstring::npos) {
			// Draw multiline text
			size_t newlinePos = buttonText.find(L'\n');
			std::wstring firstLine = buttonText.substr(0, newlinePos);
			std::wstring secondLine = buttonText.substr(newlinePos + 1);

			float lineHeight = (textRect.bottom - textRect.top) / 2.0f;
			D2D1_RECT_F firstLineRect = D2D1::RectF(textRect.left, textRect.top,
				textRect.right, textRect.top + lineHeight);
			D2D1_RECT_F secondLineRect = D2D1::RectF(textRect.left, textRect.top + lineHeight,
				textRect.right, textRect.bottom);

			if (m_pBoldTextFormat) {
				m_pRenderTarget->DrawText(firstLine.c_str(), static_cast<UINT32>(firstLine.length()),
					m_pBoldTextFormat, &firstLineRect, pTextBrush);
			}
			if (m_pButtonTextFormat) {
				m_pRenderTarget->DrawText(secondLine.c_str(), static_cast<UINT32>(secondLine.length()),
					m_pButtonTextFormat, &secondLineRect, pTextBrush);
			}
		}
		else {
			if (isStartButton && !m_bTimerActive) {
				// Start button with icon + text
				const StartButtonMeasurements& measurements = m_layoutData.startButtonMeasurements;

				// Draw icon
				D2D1_RECT_F iconRect = D2D1::RectF(
					measurements.startX, textRect.top,
					measurements.startX + measurements.iconWidth, textRect.bottom);
				if (m_pIconTextFormat) {
					m_pRenderTarget->DrawText(ICON_PLAY, 1, m_pIconTextFormat, &iconRect, pTextBrush);
				}

				// Draw text right after icon
				D2D1_RECT_F mainTextRect = D2D1::RectF(
					measurements.startX + measurements.iconWidth, textRect.top,
					textRect.right, textRect.bottom);
				if (m_pBoldLeftTextFormat) {
					m_pRenderTarget->DrawText(BTN_START_CLICK, static_cast<UINT32>(wcslen(BTN_START_CLICK)),
						m_pBoldLeftTextFormat, &mainTextRect, pTextBrush);
				}
			}
			else {
				IDWriteTextFormat* textFormat = isStartButton ? m_pBoldTextFormat : m_pButtonTextFormat;

				if (textFormat) {
					m_pRenderTarget->DrawText(buttonText.c_str(), static_cast<UINT32>(buttonText.length()),
						textFormat, &textRect, pTextBrush);
				}
			}
		}
	}

	// Get button text for either start or target - could be refactored as it's a bit involved
	std::wstring GetButtonText(bool isTargetButton, bool isStartButton) {
		if (isTargetButton) {
			if (m_hTargetWindow && !m_targetProcessName.empty()) {
				std::wstringstream wss;
				// Convert process name to wide string
				std::wstring processNameW;
				size_t len = MultiByteToWideChar(CP_UTF8, 0, m_targetProcessName.c_str(), -1, nullptr, 0);
				if (len > 0) {
					processNameW.resize(len - 1);
					MultiByteToWideChar(CP_UTF8, 0, m_targetProcessName.c_str(), -1, &processNameW[0], static_cast<int>(len));
				}

				std::wstring titleW;
				if (!m_targetWindowTitle.empty()) {
					size_t titleLen = MultiByteToWideChar(CP_UTF8, 0, m_targetWindowTitle.c_str(), -1, nullptr, 0);
					if (titleLen > 0) {
						titleW.resize(titleLen - 1);
						MultiByteToWideChar(CP_UTF8, 0, m_targetWindowTitle.c_str(), -1, &titleW[0], static_cast<int>(titleLen));

						// Limit title to specified character limit with ellipsis
						if (titleW.length() > TITLE_CHAR_LIMIT) {
							titleW = titleW.substr(0, TITLE_CHAR_LIMIT) + TITLE_ELLIPSIS;
						}
					}
				}
				else {
					titleW = TITLE_NO_TITLE;
				}

				wss << processNameW << L"\n\"" << titleW << L"\" (0x" << std::hex << std::uppercase
					<< reinterpret_cast<uintptr_t>(m_hTargetWindow) << L")";
				return wss.str();
			}
			else if (m_bCapturing) {
				return BTN_TARGET_CAPTURE;
			}
			else {
				return BTN_TARGET_SELECT;
			}
		}
		else if (isStartButton) {
			if (m_bTimerActive) {
				return GetCountdownText();
			}
			else {
				return BTN_START_CLICK;
			}
		}
		return L"";
	}

	// When timer is running we show the countdown on the start button
	std::wstring GetCountdownText() const {
		if (!m_bTimerActive) return L"";

		auto now = std::chrono::system_clock::now();
		auto remaining = std::chrono::duration_cast<std::chrono::seconds>(m_targetTime - now);

		if (remaining.count() > 0) {
			int hours = static_cast<int>(remaining.count() / 3600);
			int minutes = static_cast<int>((remaining.count() % 3600) / 60);
			int seconds = static_cast<int>(remaining.count() % 60);

			std::wstringstream wss;
			wss << L"Resuming in " << std::setfill(L'0') << std::setw(2) << hours
				<< L":" << std::setw(2) << minutes << L":" << std::setw(2) << seconds;
			return wss.str();
		}
		return L"";
	}

	// Hours buttons text construction
	std::vector<std::wstring> GetNext5HourTimes() {
		std::vector<std::wstring> times;
		auto now = std::chrono::system_clock::now();

		// Get the start of the next hour
		auto time_t = std::chrono::system_clock::to_time_t(now);
		struct tm local_tm;
		localtime_s(&local_tm, &time_t);

		// Set to start of next hour
		local_tm.tm_min = 0;
		local_tm.tm_sec = 0;
		local_tm.tm_hour += 1;

		auto nextHour = std::chrono::system_clock::from_time_t(mktime(&local_tm));

		// Generate hour times starting from next hour
		for (int i = 0; i < HOUR_COUNT; i++) {
			auto futureTime = nextHour + std::chrono::hours(i);
			auto future_time_t = std::chrono::system_clock::to_time_t(futureTime);
			struct tm future_tm;
			localtime_s(&future_tm, &future_time_t);

			// Format as 12-hour with am/pm
			std::wstringstream wss;
			int hour12 = future_tm.tm_hour % 12;
			if (hour12 == 0) hour12 = 12; // Handle 12am/12pm

			wss << hour12;
			if (future_tm.tm_hour >= 12) {
				wss << L"pm";
			}
			else {
				wss << L"am";
			}

			times.push_back(wss.str());
		}

		return times;
	}

public:
	ARCCApp() : m_hTargetWindow(nullptr), m_hInputHook(nullptr), m_bCapturing(false), m_bTimerActive(false),
		m_selectedHourOffset(0), m_hMainWindow(nullptr), m_bDragging(false), m_bMouseTracking(false),
		m_hBackgroundBrush(nullptr), m_bWindowActive(true), m_currentDpiX(96.0f), m_currentDpiY(96.0f),
		m_titleBarHover(TitleBarHover::None), m_pD2DFactory(nullptr), m_pRenderTarget(nullptr),
		m_pBgBrush(nullptr), m_pTextBrush(nullptr), m_pButtonBrush(nullptr), m_pButtonHoverBrush(nullptr),
		m_pGreenBrush(nullptr), m_pGreenHoverBrush(nullptr), m_pRedBrush(nullptr), m_pRedHoverBrush(nullptr),
		m_pAmberBrush(nullptr), m_pTitleBarBrush(nullptr), m_pWhiteBrush(nullptr), m_pDWriteFactory(nullptr),
		m_pTextFormat(nullptr), m_pTitleTextFormat(nullptr), m_pButtonTextFormat(nullptr),
		m_pBoldTextFormat(nullptr), m_pIconTextFormat(nullptr), m_pBoldIconTextFormat(nullptr),
		m_pBoldLeftTextFormat(nullptr)
	{
		s_pInstance = this;
		m_mousePos.x = m_mousePos.y = -1;

		// Initialize Direct2D
		HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
		if (FAILED(hr)) return;

		// Initialize DirectWrite
		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory),
			reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
		if (FAILED(hr)) return;

		CreateTextFormats();
	}

	~ARCCApp() {
		// Remove message hook if it's active
		if (m_hInputHook) {
			UnhookWindowsHookEx(m_hInputHook);
		}

		// Cleanup window background brush
		if (m_hBackgroundBrush) {
			DeleteObject(m_hBackgroundBrush);
		}

		// Release brush resources
		SafeRelease(&m_pBgBrush);
		SafeRelease(&m_pTextBrush);
		SafeRelease(&m_pButtonBrush);
		SafeRelease(&m_pButtonHoverBrush);
		SafeRelease(&m_pGreenBrush);
		SafeRelease(&m_pGreenHoverBrush);
		SafeRelease(&m_pRedBrush);
		SafeRelease(&m_pRedHoverBrush);
		SafeRelease(&m_pAmberBrush);
		SafeRelease(&m_pTitleBarBrush);
		SafeRelease(&m_pWhiteBrush);
		SafeRelease(&m_pRenderTarget);
		SafeRelease(&m_pD2DFactory);

		// Cleanup DirectWrite resources
		SafeRelease(&m_pTextFormat);
		SafeRelease(&m_pTitleTextFormat);
		SafeRelease(&m_pButtonTextFormat);
		SafeRelease(&m_pBoldTextFormat);
		SafeRelease(&m_pIconTextFormat);
		SafeRelease(&m_pBoldIconTextFormat);
		SafeRelease(&m_pDWriteFactory);

	}

	// Need to access app class instance from wnd procs
	static ARCCApp* GetInstance() { return s_pInstance; }

	int Run(HINSTANCE hInstance) {
		// Register custom window class
		WNDCLASSEXA wcex = {};
		wcex.cbSize = sizeof(WNDCLASSEXA);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = MainWndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		m_hBackgroundBrush = CreateSolidBrush(RGB(0x19, 0x19, 0x22)); // Dark background to match Direct2D
		wcex.hbrBackground = m_hBackgroundBrush;
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = APP_CLASS_NAME;
		wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));

		if (!RegisterClassExA(&wcex)) {
			return 1;
		}

		// Get DPI for proper scaling
		HDC desktopDC = GetDC(nullptr);
		int dpi = GetDeviceCaps(desktopDC, LOGPIXELSX);
		ReleaseDC(nullptr, desktopDC);

		// Store initial DPI for consistent usage
		m_currentDpiX = static_cast<float>(dpi);
		m_currentDpiY = static_cast<float>(dpi);

		// Calculate window size
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		// Create window as a square initially to get proper client width
		int clientWidth = DIPToPixel_X(WINDOW_WIDTH);
		int initialHeight = clientWidth;

		RECT windowRect = { 0, 0, clientWidth, initialHeight };
		AdjustWindowRectEx(&windowRect, WS_POPUP | WS_THICKFRAME, FALSE, WS_EX_APPWINDOW);

		// Center the app on the screen (good enough)
		int windowWidth = windowRect.right - windowRect.left;
		int windowHeight = windowRect.bottom - windowRect.top;
		int x = (screenWidth - windowWidth) / 2;
		int y = (screenHeight - windowHeight) / 2;

		// Create the window
		m_hMainWindow = CreateWindowExA(
			WS_EX_APPWINDOW,
			APP_CLASS_NAME,
			APP_WINDOW_TITLE,
			WS_POPUP | WS_THICKFRAME,
			x, y, windowWidth, windowHeight,
			nullptr, nullptr, hInstance, nullptr);

		if (!m_hMainWindow) {
			return 1;
		}

		// Now calculate proper content height using real client width
		int contentHeight = GetCalculatedContentHeight();
		int properClientHeight = DIPToPixel_Y(contentHeight);

		// Resize window to proper height
		RECT properWindowRect = { 0, 0, clientWidth, properClientHeight };
		AdjustWindowRectEx(&properWindowRect, WS_POPUP | WS_THICKFRAME, FALSE, WS_EX_APPWINDOW);

		int properWindowHeight = properWindowRect.bottom - properWindowRect.top;
		int properY = (screenHeight - properWindowHeight) / 2;

		SetWindowPos(m_hMainWindow, nullptr, x, properY, windowWidth, properWindowHeight,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

		// Force immediate processing of the frame change
		UpdateWindow(m_hMainWindow);

		// Apply Windows 11 rounded corners and OS theming
		ApplyModernWindowStyling();

		// Now show the properly sized window
		ShowWindow(m_hMainWindow, SW_SHOW);
		UpdateWindow(m_hMainWindow);

		// Message loop
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return static_cast<int>(msg.wParam);
	}

private:
	static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		ARCCApp* pApp = GetInstance();
		if (pApp) {
			return pApp->HandleWindowMessage(hWnd, message, wParam, lParam);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	LRESULT HandleWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		switch (message) {
		case WM_CREATE:
			OnInitialize();
			return 0;
		case WM_TIMER:
			OnTimer(hWnd, wParam);
			return 0;
		case WM_PAINT:
			OnPaint(hWnd);
			return 0;
		case WM_KEYDOWN:
			// User presses ESC when we are capturing other app window
			if (wParam == VK_ESCAPE && m_bCapturing) {
				m_hTargetWindow = nullptr;
				m_targetWindowTitle.clear();
				m_targetProcessName.clear();
				StopWindowCapture();
				UpdateUI();
			}
			return 0;
		case WM_KILLFOCUS:
			// Might change this later. But we cancel capture when we lose focus. Would be nice if we 
			// allowed user to click on explorer/taskbar to navigate to a window. But good enough for now.
			if (m_bCapturing) {
				m_hTargetWindow = nullptr;
				m_targetWindowTitle.clear();
				m_targetProcessName.clear();
				StopWindowCapture();
				UpdateUI();
			}
			break;
		case WM_LBUTTONDOWN:
			OnMouseLeftClick(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_LBUTTONUP:
			if (m_bDragging) {
				m_bDragging = false;
				ReleaseCapture();
			}
			return 0;
		case WM_MOUSEMOVE:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);

			if (m_bDragging) {
				OnTitleBarDrag(hWnd, x, y);
			}
			else {
				// Track mouse for hover effects
				if (m_mousePos.x != x || m_mousePos.y != y) {
					m_mousePos.x = x;
					m_mousePos.y = y;

					// Enable mouse tracking to get WM_MOUSELEAVE
					if (!m_bMouseTracking) {
						TRACKMOUSEEVENT tme = {};
						tme.cbSize = sizeof(tme);
						tme.dwFlags = TME_LEAVE;
						tme.hwndTrack = hWnd;
						TrackMouseEvent(&tme);
						m_bMouseTracking = true;
					}

					// Check if hovering over any interactive element
					bool overButton = false;

					// Update title bar button hover state
					TitleBarHover newHover = TitleBarHover::None;

					// Convert mouse Y to DIP for comparison with TITLEBAR_HEIGHT
					float dipY_check = PixelToDIP_Y(y);

					if (dipY_check <= TITLEBAR_HEIGHT) {
						// Convert mouse coordinates to DIP using stored DPI values
						float dipX = PixelToDIP_X(x);
						float dipY = PixelToDIP_Y(y);

						// Use cached button positions
						const TitleBarButtonPositions& pos = m_titleBarButtonPositions;

						// Check hover state using DIP coordinates
						if (dipX >= pos.closeButtonX && dipX <= pos.closeButtonX + pos.buttonWidth &&
							dipY >= pos.buttonY && dipY <= pos.buttonY + pos.buttonHeight) {
							newHover = TitleBarHover::Close;
							overButton = true;
						}
						else if (dipX >= pos.minimizeButtonX && dipX <= pos.minimizeButtonX + pos.buttonWidth &&
							dipY >= pos.buttonY && dipY <= pos.buttonY + pos.buttonHeight) {
							newHover = TitleBarHover::Minimize;
							overButton = true;
						}
						else if (dipX >= pos.helpButtonX && dipX <= pos.helpButtonX + pos.buttonWidth &&
							dipY >= pos.buttonY && dipY <= pos.buttonY + pos.buttonHeight) {
							newHover = TitleBarHover::Help;
							overButton = true;
						}
					}
					else {
						// Check main content buttons using stored DPI values
						float dipX = PixelToDIP_X(x);
						float dipY = PixelToDIP_Y(y);

						// Ensure layout is available for hit testing
						if (!m_layoutData.isValid) {
							CalculateLayout();
						}

						// Check target button
						if (dipX >= m_layoutData.targetButtonRect.left && dipX <= m_layoutData.targetButtonRect.right &&
							dipY >= m_layoutData.targetButtonRect.top && dipY <= m_layoutData.targetButtonRect.bottom) {
							overButton = true;
						}
						// Check start button
						else if (dipX >= m_layoutData.startButtonRect.left && dipX <= m_layoutData.startButtonRect.right &&
							dipY >= m_layoutData.startButtonRect.top && dipY <= m_layoutData.startButtonRect.bottom) {
							overButton = true;
						}
						// Check hour buttons
						else {
							for (int i = 0; i < HOUR_COUNT; i++) {
								if (dipX >= m_layoutData.hourButtonRects[i].left && dipX <= m_layoutData.hourButtonRects[i].right &&
									dipY >= m_layoutData.hourButtonRects[i].top && dipY <= m_layoutData.hourButtonRects[i].bottom) {
									overButton = true;
									break;
								}
							}
						}
					}

					// Set appropriate cursor
					if (overButton) {
						SetCursor(LoadCursor(nullptr, IDC_HAND));
					}
					else {
						SetCursor(LoadCursor(nullptr, IDC_ARROW));
					}

					// Only repaint if title bar hover state changed
					if (m_titleBarHover != newHover) {
						m_titleBarHover = newHover;
						// Invalidate only the title bar area for efficiency
						RECT clientRect;
						GetClientRect(hWnd, &clientRect);
						RECT titleBarRect = { 0, 0, clientRect.right, static_cast<LONG>(TITLEBAR_HEIGHT) };
						InvalidateRect(hWnd, &titleBarRect, FALSE);
					}

					// Invalidate to trigger repaint for hover effects
					InvalidateRect(hWnd, nullptr, FALSE);
				}
			}
		}
		return 0;
		case WM_MOUSELEAVE:
			m_bMouseTracking = false;
			m_mousePos.x = m_mousePos.y = -1;
			m_titleBarHover = TitleBarHover::None;
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
			InvalidateRect(hWnd, nullptr, FALSE);
			return 0;
		case WM_SIZE:
			if (m_pRenderTarget) {
				RECT rc;
				GetClientRect(hWnd, &rc);
				D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
				m_pRenderTarget->Resize(size);
				m_layoutData.isValid = false;
				UpdateTitleBarButtonPositions(hWnd);
			}
			return 0;
		case WM_NCACTIVATE:
		{
			bool wasActive = m_bWindowActive;
			m_bWindowActive = (wParam != FALSE);
			if (wasActive != m_bWindowActive) {
				InvalidateRect(hWnd, nullptr, FALSE);
				UpdateWindow(hWnd);
			}
			// Let Windows handle the borders, but prevent title bar redraw by setting lParam to -1
			return DefWindowProc(hWnd, message, wParam, (LPARAM)-1);
		}
		case WM_NCCALCSIZE:
		{
			if (wParam == TRUE) {
				NCCALCSIZE_PARAMS* pParams = (NCCALCSIZE_PARAMS*)lParam;
				LRESULT result = DefWindowProc(hWnd, message, wParam, lParam);

				int scaledOffset = MulDiv(8, static_cast<int>(m_currentDpiY), 96);
				pParams->rgrc[0].top -= scaledOffset;

				return result;
			}
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
		case WM_DPICHANGED:
		{
			// Get the new DPI from the message
			int newDpiX = LOWORD(wParam);
			int newDpiY = HIWORD(wParam);

			// Use the suggested window rect from Windows for position
			RECT* pSuggestedRect = (RECT*)lParam;

			// Calculate our intended width with new DPI
			int ourClientWidth = DIPToPixel_X(WINDOW_WIDTH);
			int tempClientHeight = DIPToPixel_Y(400);

			RECT tempWindowRect = { 0, 0, ourClientWidth, tempClientHeight };
			AdjustWindowRectEx(&tempWindowRect, WS_POPUP | WS_THICKFRAME, FALSE, WS_EX_APPWINDOW);

			SetWindowPos(hWnd, nullptr,
				pSuggestedRect->left, pSuggestedRect->top,
				tempWindowRect.right - tempWindowRect.left,
				tempWindowRect.bottom - tempWindowRect.top,
				SWP_NOZORDER | SWP_NOACTIVATE);

			// First recreate text formats and render target with new DPI
			CreateTextFormats();
			ApplyModernWindowStyling();
			DiscardDeviceResources();
			CreateDeviceResources(hWnd, (float)newDpiX, (float)newDpiY);

			// Calculate content height with the new DPI-scaled fonts using intended client width
			int contentHeight = GetCalculatedContentHeight(WINDOW_WIDTH);
			int properClientHeight = DIPToPixel_Y(contentHeight);

			RECT properWindowRect = { 0, 0,
				pSuggestedRect->right - pSuggestedRect->left,
				properClientHeight };
			AdjustWindowRectEx(&properWindowRect, WS_POPUP | WS_THICKFRAME, FALSE, WS_EX_APPWINDOW);

			int properWindowHeight = properWindowRect.bottom - properWindowRect.top;

			// Final resize to correct height
			SetWindowPos(hWnd, nullptr,
				pSuggestedRect->left, pSuggestedRect->top,
				pSuggestedRect->right - pSuggestedRect->left, properWindowHeight,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

			// Invalidate cached measurements and recalculate with new window size
			m_layoutData.isValid = false;
			CalculateLayout();
			UpdateTitleBarButtonPositions(hWnd);
			InvalidateRect(hWnd, nullptr, FALSE);

			return 0;
		}
		case WM_ACTIVATE:
		{
			bool wasActive = m_bWindowActive;
			m_bWindowActive = (LOWORD(wParam) != WA_INACTIVE);
			if (wasActive != m_bWindowActive) {
				InvalidateRect(hWnd, nullptr, FALSE);
				UpdateWindow(hWnd);
			}
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
		case WM_DESTROY:
			// Ensure sleep prevention is disabled on exit
			StopTimer();
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	void OnInitialize() {
		UpdateUI();
	}

	AppState GetCurrentAppState() const {
		if (m_bTimerActive) {
			return AppState::Waiting;
		}
		else if (m_hTargetWindow != nullptr) {
			return AppState::Ready;
		}
		else {
			return AppState::Idle;
		}
	}

	// Paint all the things
	void OnPaint(HWND hWnd) {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		HRESULT hr = CreateDeviceResources(hWnd);
		if (SUCCEEDED(hr)) {
			m_pRenderTarget->BeginDraw();

			// Clear background
			m_pRenderTarget->Clear(BG_COLOR);

			// Draw custom title bar
			D2D1_SIZE_F renderTargetSize = m_pRenderTarget->GetSize();
			D2D1_RECT_F titleBarRect = D2D1::RectF(0, 0, renderTargetSize.width, TITLEBAR_HEIGHT);
			m_pRenderTarget->FillRectangle(&titleBarRect, m_pTitleBarBrush);

			// Draw application icon
			HICON hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_MAIN_ICON));
			if (hIcon) {
				// Calculate icon size and position
				constexpr int iconSize = 24;
				constexpr float iconX = 8.0f;
				constexpr float iconY = (TITLEBAR_HEIGHT - iconSize) / 2;

				// Create a compatible DC for the icon
				HDC hdcScreen = GetDC(nullptr);
				HDC hdcMem = CreateCompatibleDC(hdcScreen);
				HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, iconSize, iconSize);
				HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

				// Fill background
				RECT iconRect = { 0, 0, iconSize, iconSize };
				HBRUSH hBrush = CreateSolidBrush(RGB(0x2A, 0x2A, 0x2A));
				FillRect(hdcMem, &iconRect, hBrush);
				DeleteObject(hBrush);

				// Draw icon
				DrawIconEx(hdcMem, 0, 0, hIcon, iconSize, iconSize, 0, nullptr, DI_NORMAL);

				// Convert to Direct2D bitmap
				ID2D1Bitmap* pBitmap = nullptr;
				D2D1_SIZE_U bitmapSize = D2D1::SizeU(iconSize, iconSize);
				D2D1_BITMAP_PROPERTIES bitmapProps = D2D1::BitmapProperties(
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

				// Get bitmap bits
				BITMAPINFO bmi = {};
				bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				bmi.bmiHeader.biWidth = iconSize;
				bmi.bmiHeader.biHeight = -iconSize;
				bmi.bmiHeader.biPlanes = 1;
				bmi.bmiHeader.biBitCount = 32;
				bmi.bmiHeader.biCompression = BI_RGB;

				void* pBits = malloc(static_cast<size_t>(iconSize * iconSize * 4));
				if (pBits && GetDIBits(hdcMem, hBitmap, 0, iconSize, pBits, &bmi, DIB_RGB_COLORS)) {
					HRESULT hr = m_pRenderTarget->CreateBitmap(bitmapSize, pBits, iconSize * 4, &bitmapProps, &pBitmap);
					if (SUCCEEDED(hr) && pBitmap) {
						D2D1_RECT_F destRect = D2D1::RectF(iconX, iconY, iconX + iconSize, iconY + iconSize);
						m_pRenderTarget->DrawBitmap(pBitmap, &destRect);
						pBitmap->Release();
					}
				}
				free(pBits);

				// Cleanup
				SelectObject(hdcMem, hOldBitmap);
				DeleteObject(hBitmap);
				DeleteDC(hdcMem);
				ReleaseDC(nullptr, hdcScreen);
				DestroyIcon(hIcon);
			}

			// Draw title text using DirectWrite
			if (m_pTitleTextFormat && m_pTextBrush) {
				int iconSize = 24;
				float textStartX = 8.0f + static_cast<float>(iconSize) + 8.0f;
				// Draw main title
				D2D1_RECT_F mainTitleRect = D2D1::RectF(textStartX, 0, 340, TITLEBAR_HEIGHT);
				m_pRenderTarget->DrawText(
					APP_TITLE_MAIN,
					static_cast<UINT32>(wcslen(APP_TITLE_MAIN)),
					m_pTitleTextFormat,
					&mainTitleRect,
					m_pTextBrush
				);

				// Calculate width of main title to position subtitle
				IDWriteTextLayout* pMainLayout = nullptr;
				HRESULT hr = m_pDWriteFactory->CreateTextLayout(
					APP_TITLE_MAIN, static_cast<UINT32>(wcslen(APP_TITLE_MAIN)),
					m_pTitleTextFormat, 1000.0f, TITLEBAR_HEIGHT, &pMainLayout);

				if (SUCCEEDED(hr) && pMainLayout) {
					DWRITE_TEXT_METRICS mainMetrics;
					pMainLayout->GetMetrics(&mainMetrics);

					// Draw subtitle with opacity and gap
					float subtitleStartX = textStartX + mainMetrics.width + 12; // 12px gap
					D2D1_RECT_F subtitleRect = D2D1::RectF(subtitleStartX, 0, 340, TITLEBAR_HEIGHT);

					// Set opacity for subtitle
					m_pTextBrush->SetOpacity(0.6f);
					m_pRenderTarget->DrawText(
						APP_TITLE_SUB,
						static_cast<UINT32>(wcslen(APP_TITLE_SUB)),
						m_pTitleTextFormat,
						&subtitleRect,
						m_pTextBrush
					);
					m_pTextBrush->SetOpacity(1.0f);

					pMainLayout->Release();
				}
			}

			// Ensure positions are calculated and draw title bar buttons
			if (m_titleBarButtonPositions.buttonWidth == 0) {
				UpdateTitleBarButtonPositions(hWnd);
			}
			const TitleBarButtonPositions& pos = m_titleBarButtonPositions;

			// Draw help button
			D2D1_RECT_F helpButtonRect = D2D1::RectF(
				pos.helpButtonX, pos.buttonY,
				pos.helpButtonX + pos.buttonWidth, pos.buttonY + pos.buttonHeight);
			m_pRenderTarget->FillRectangle(&helpButtonRect, m_pTitleBarBrush);

			// Draw hover effect with opacity
			if (m_titleBarHover == TitleBarHover::Help) {
				m_pWhiteBrush->SetOpacity(0.15f);
				m_pRenderTarget->FillRectangle(&helpButtonRect, m_pWhiteBrush);
				m_pWhiteBrush->SetOpacity(1.0f);
			}

			// Draw help icon (bold)
			if (m_pBoldIconTextFormat) {
				m_pRenderTarget->DrawText(
					ICON_HELP, 1, m_pBoldIconTextFormat,
					&helpButtonRect, m_pTextBrush
				);
			}

			// Draw minimize button
			D2D1_RECT_F minimizeButtonRect = D2D1::RectF(
				pos.minimizeButtonX, pos.buttonY,
				pos.minimizeButtonX + pos.buttonWidth, pos.buttonY + pos.buttonHeight);
			m_pRenderTarget->FillRectangle(&minimizeButtonRect, m_pTitleBarBrush);

			// Draw hover effect with opacity
			if (m_titleBarHover == TitleBarHover::Minimize) {
				m_pWhiteBrush->SetOpacity(0.15f);
				m_pRenderTarget->FillRectangle(&minimizeButtonRect, m_pWhiteBrush);
				m_pWhiteBrush->SetOpacity(1.0f);
			}

			// Draw minimize icon
			if (m_pIconTextFormat && m_pTextBrush) {
				m_pRenderTarget->DrawText(
					ICON_MINIMIZE, 1, m_pIconTextFormat,
					&minimizeButtonRect, m_pTextBrush
				);
			}

			// Draw close button
			D2D1_RECT_F closeButtonRect = D2D1::RectF(
				pos.closeButtonX, pos.buttonY,
				pos.closeButtonX + pos.buttonWidth, pos.buttonY + pos.buttonHeight);
			m_pRenderTarget->FillRectangle(&closeButtonRect, m_pTitleBarBrush);

			// Draw hover effect with opacity
			if (m_titleBarHover == TitleBarHover::Close) {
				m_pRedBrush->SetOpacity(0.8f);
				m_pRenderTarget->FillRectangle(&closeButtonRect, m_pRedBrush);
				m_pRedBrush->SetOpacity(1.0f);
			}

			// Draw close icon
			if (m_pIconTextFormat) {
				m_pRenderTarget->DrawText(
					ICON_CLOSE, 1, m_pIconTextFormat,
					&closeButtonRect, m_pTextBrush
				);
			}

			// Draw main UI content
			DrawMainContent();

			hr = m_pRenderTarget->EndDraw();

			if (hr == D2DERR_RECREATE_TARGET) {
				DiscardDeviceResources();
			}
		}

		EndPaint(hWnd, &ps);
	}

	void OnMouseLeftClick(HWND hWnd, int x, int y) {
		float dipY_check = PixelToDIP_Y(y);

		// Deal with a title bar click
		if (dipY_check <= TITLEBAR_HEIGHT) {
			float dipX = PixelToDIP_X(x);
			float dipY = PixelToDIP_Y(y);

			// Use cached button positions
			const TitleBarButtonPositions& pos = m_titleBarButtonPositions;

			if (dipX >= pos.closeButtonX && dipX <= pos.closeButtonX + pos.buttonWidth &&
				dipY >= pos.buttonY && dipY <= pos.buttonY + pos.buttonHeight) {
				PostMessage(hWnd, WM_DESTROY, 0, 0);
			}
			else if (dipX >= pos.minimizeButtonX && dipX <= pos.minimizeButtonX + pos.buttonWidth &&
				dipY >= pos.buttonY && dipY <= pos.buttonY + pos.buttonHeight) {
				ShowWindow(hWnd, SW_MINIMIZE);
			}
			else if (dipX >= pos.helpButtonX && dipX <= pos.helpButtonX + pos.buttonWidth &&
				dipY >= pos.buttonY && dipY <= pos.buttonY + pos.buttonHeight) {
				ShellExecuteA(nullptr, "open", HELP_URL, nullptr, nullptr, SW_SHOWNORMAL);
			}
			else {
				// Start dragging
				m_bDragging = true;
				POINT pt = { x, y };
				ClientToScreen(hWnd, &pt);
				RECT rect;
				GetWindowRect(hWnd, &rect);
				m_dragOffset.x = pt.x - rect.left;
				m_dragOffset.y = pt.y - rect.top;
				SetCapture(hWnd);
			}
		}
		else {
			// Handle main content button clicks
			HandleContentClick(hWnd, x, y);
		}
	}

	// Clicking all the things
	void HandleContentClick(HWND hWnd, int x, int y) {
		RECT clientRect, windowRect;
		GetClientRect(hWnd, &clientRect);
		GetWindowRect(hWnd, &windowRect);

		float dipX = PixelToDIP_X(x);
		float dipY = PixelToDIP_Y(y);

		// Ensure layout is available for hit testing
		if (!m_layoutData.isValid) {
			CalculateLayout();
		}

		if (dipX >= m_layoutData.targetButtonRect.left && dipX <= m_layoutData.targetButtonRect.right &&
			dipY >= m_layoutData.targetButtonRect.top && dipY <= m_layoutData.targetButtonRect.bottom) {
			StartWindowCapture();
			return;
		}

		if (dipX >= m_layoutData.startButtonRect.left && dipX <= m_layoutData.startButtonRect.right &&
			dipY >= m_layoutData.startButtonRect.top && dipY <= m_layoutData.startButtonRect.bottom) {
			AppState currentState = GetCurrentAppState();
			if (currentState != AppState::Idle) {
				ToggleTimer();
			}
			return;
		}

		// Hour button hit tests
		for (int i = 0; i < HOUR_COUNT; i++) {
			if (dipX >= m_layoutData.hourButtonRects[i].left && dipX <= m_layoutData.hourButtonRects[i].right &&
				dipY >= m_layoutData.hourButtonRects[i].top && dipY <= m_layoutData.hourButtonRects[i].bottom) {
				m_selectedHourOffset = i;
				InvalidateRect(hWnd, nullptr, FALSE);
				return;
			}
		}
	}

	void OnTitleBarDrag(HWND hWnd, int x, int y) const {
		POINT pt = { x, y };
		ClientToScreen(hWnd, &pt);
		SetWindowPos(hWnd, nullptr, pt.x - m_dragOffset.x, pt.y - m_dragOffset.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}

	void OnTimer(HWND hWnd, WPARAM timerID) {
		switch (timerID) {
		case TIMER_COUNTDOWN:
			CheckCountdown();
			break;
		case TIMER_STATUS_UPDATE:
			UpdateUI();
			break;
		}
	}

	void StartWindowCapture() {
		// Clear any existing target to start fresh
		m_hTargetWindow = nullptr;
		m_targetWindowTitle.clear();
		m_targetProcessName.clear();

		m_bCapturing = true;

		UpdateUI();

		m_hInputHook = SetWindowsHookEx(WH_MOUSE_LL, InputHookProc, GetModuleHandle(nullptr), 0);
		if (!m_hInputHook) {
			MessageBoxA(m_hMainWindow, ERR_HOOK_FAILED, ERR_TITLE, MB_OK | MB_ICONERROR);
			m_bCapturing = false;
			UpdateUI();
		}
	}

	static LRESULT CALLBACK InputHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
		ARCCApp* pApp = GetInstance();
		if (pApp && pApp->m_bCapturing) {
			return pApp->HandleInputHook(nCode, wParam, lParam);
		}
		return CallNextHookEx(nullptr, nCode, wParam, lParam);
	}

	// Mouse tracking during app window selection
	LRESULT HandleInputHook(int nCode, WPARAM wParam, LPARAM lParam) {
		if (nCode >= 0 && wParam == WM_LBUTTONDOWN) {
			POINT pt;
			GetCursorPos(&pt);
			HWND hWnd = WindowFromPoint(pt);

			if (hWnd) {
				// Get process name
				DWORD processId;
				GetWindowThreadProcessId(hWnd, &processId);
				HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
				if (hSnapshot != INVALID_HANDLE_VALUE) {
					PROCESSENTRY32 pe{};
					pe.dwSize = sizeof(PROCESSENTRY32);
					if (Process32First(hSnapshot, &pe)) {
						do {
							if (pe.th32ProcessID == processId) {
								m_targetProcessName = pe.szExeFile;
								// Remove .exe extension
								size_t pos = m_targetProcessName.find(FILE_EXT_EXE);
								if (pos != std::string::npos) {
									m_targetProcessName = m_targetProcessName.substr(0, pos);
								}
								break;
							}
						} while (Process32Next(hSnapshot, &pe));
					}
					CloseHandle(hSnapshot);
				}

				// Skip explorer and our own app - this isn't working atm as we cancel tracking on losing focus
				if (m_targetProcessName == PROCESS_EXPLORER || m_targetProcessName == PROCESS_ARCC) {
					return CallNextHookEx(m_hInputHook, nCode, wParam, lParam);
				}

				// Get window title
				wchar_t titleW[256];
				GetWindowTextW(hWnd, titleW, sizeof(titleW) / sizeof(wchar_t));

				// Convert to UTF-8
				int utf8Length = WideCharToMultiByte(CP_UTF8, 0, titleW, -1, nullptr, 0, nullptr, nullptr);
				if (utf8Length > 0) {
					std::string utf8Title(utf8Length - 1, '\0');
					WideCharToMultiByte(CP_UTF8, 0, titleW, -1, &utf8Title[0], utf8Length, nullptr, nullptr);
					m_targetWindowTitle = utf8Title;
				}
				else {
					m_targetWindowTitle.clear();
				}

				m_hTargetWindow = hWnd;
				StopWindowCapture();
				UpdateUI();
			}
		}

		return CallNextHookEx(m_hInputHook, nCode, wParam, lParam);
	}

	void StopWindowCapture() {
		if (m_hInputHook) {
			UnhookWindowsHookEx(m_hInputHook);
			m_hInputHook = nullptr;
		}
		m_bCapturing = false;
	}

	void ToggleTimer() {
		if (m_bTimerActive) {
			StopTimer();
		}
		else {
			// Start timer
			if (!m_hTargetWindow) {
				return;
			}

			// Calculate target time based on selected hour offset
			auto now = std::chrono::system_clock::now();
			auto time_t = std::chrono::system_clock::to_time_t(now);
			struct tm local_tm;
			localtime_s(&local_tm, &time_t);

			// Add 1 to get the start of the next hour, then add the selected offset
			int targetHour = local_tm.tm_hour + 1 + m_selectedHourOffset;

			// Handle day rollover
			int dayOffset = 0;
			if (targetHour >= 24) {
				dayOffset = targetHour / 24;
				targetHour = targetHour % 24;
			}

			// Set target time with the calculated hour
			local_tm.tm_hour = targetHour;
			local_tm.tm_min = 0;
			local_tm.tm_sec = 10;	// want to resume a moment after limit reset

			auto target = std::chrono::system_clock::from_time_t(mktime(&local_tm));
			if (dayOffset > 0) {
				target += std::chrono::hours(24 * dayOffset);
			}

			m_targetTime = target;

			// Start timers
			SetTimer(m_hMainWindow, TIMER_COUNTDOWN, 1000, nullptr);
			SetTimer(m_hMainWindow, TIMER_STATUS_UPDATE, 1000, nullptr);
			m_bTimerActive = true;

			// Prevent system sleep while timer is active
			SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
		}

		UpdateUI();
	}

	void CheckCountdown() {
		auto now = std::chrono::system_clock::now();
		// see if it's time to send resume
		if (now >= m_targetTime) {
			SendResumeMessage();
			StopTimer();
			m_selectedHourOffset = 0;
			UpdateUI();
		}
	}

	// Send the resume message
	void SendResumeMessage() const {
		if (!m_hTargetWindow || !IsWindow(m_hTargetWindow)) {
			MessageBoxA(m_hMainWindow, ERR_TARGET_GONE, WARN_TITLE, MB_OK | MB_ICONWARNING);
			return;
		}

		// Bring target window to foreground
		SetForegroundWindow(m_hTargetWindow);
		Sleep(500);

		// Send "RESUME" text
		for (int i = 0; RESUME_MESSAGE[i]; i++) {
			SHORT vk = VkKeyScanA(RESUME_MESSAGE[i]);
			BYTE key = LOBYTE(vk);
			BYTE shift = HIBYTE(vk);

			if (shift & 1) keybd_event(VK_SHIFT, 0, 0, 0);
			keybd_event(key, 0, 0, 0);
			keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
			if (shift & 1) keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
		}

		// Send Enter
		keybd_event(VK_RETURN, 0, 0, 0);
		keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
	}

	// Update all the things
	void UpdateUI() const {
		if (m_hMainWindow) {
			InvalidateRect(m_hMainWindow, nullptr, FALSE);
			UpdateWindow(m_hMainWindow);
		}
	}
};

ARCCApp* ARCCApp::s_pInstance = nullptr;

// Define static color constants
const D2D1_COLOR_F ARCCApp::BG_COLOR = D2D1::ColorF(0x19 / 255.0f, 0x19 / 255.0f, 0x22 / 255.0f);
const D2D1_COLOR_F ARCCApp::TEXT_COLOR = D2D1::ColorF(0xDD / 255.0f, 0xDD / 255.0f, 0xDD / 255.0f);
const D2D1_COLOR_F ARCCApp::BUTTON_COLOR = D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x3A / 255.0f);
const D2D1_COLOR_F ARCCApp::BUTTON_HOVER_COLOR = D2D1::ColorF(0x3D / 255.0f, 0x3D / 255.0f, 0x4A / 255.0f);
const D2D1_COLOR_F ARCCApp::BUTTON_GREEN_COLOR = D2D1::ColorF(0x7F / 255.0f, 0xB5 / 255.0f, 0x8A / 255.0f);
const D2D1_COLOR_F ARCCApp::BUTTON_GREEN_HOVER_COLOR = D2D1::ColorF(0x8F / 255.0f, 0xC5 / 255.0f, 0x9A / 255.0f);
const D2D1_COLOR_F ARCCApp::BUTTON_RED_COLOR = D2D1::ColorF(0xDC / 255.0f, 0x7C / 255.0f, 0x7C / 255.0f);
const D2D1_COLOR_F ARCCApp::BUTTON_RED_HOVER_COLOR = D2D1::ColorF(0xEC / 255.0f, 0x8C / 255.0f, 0x8C / 255.0f);
const D2D1_COLOR_F ARCCApp::TARGET_BUTTON_COLOR = D2D1::ColorF(0xE5 / 255.0f, 0xBB / 255.0f, 0x6E / 255.0f);
const D2D1_COLOR_F ARCCApp::TITLEBAR_COLOR = D2D1::ColorF(0x2A / 255.0f, 0x2A / 255.0f, 0x2A / 255.0f);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	ARCCApp app;
	return app.Run(hInstance);
}