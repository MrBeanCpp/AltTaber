#include "utils/uiautomation.h"
#include <windows.h>
#include <QDebug>
#include "utils/Util.h"

IUIAutomation* UIAutomation::pAutomation = nullptr;

bool UIAutomation::init() {
    qDebug() << "UIAutomation initializing";
    pAutomation = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER, IID_IUIAutomation, (void**) &pAutomation);
    if (FAILED(hr) || !pAutomation) {
        qCritical() << "Failed to create UIAutomation instance." << qt_error_string(hr);
        return false;
    }

    return true;
}

/// based on physical cursor position
UIElement UIAutomation::getElementUnderMouse() {
    if (!pAutomation) {
        if (!init()) {
            qWarning() << "Failed to initialize UIAutomation.";
            return {};
        }
    }

    POINT pt;
    GetCursorPos(&pt);
    IUIAutomationElement* pElement = nullptr;
    auto hr = pAutomation->ElementFromPoint(pt, &pElement);

    if (FAILED(hr) || !pElement) {
        qWarning() << "Failed to get element under mouse." << hr << pElement << qt_error_string(hr);
        return {};
    }

    return UIElement{pElement};
}

UIElement UIAutomation::getParentWithHWND(const UIElement& element) {
    IUIAutomationElement* pParent = nullptr;
    IUIAutomationElement* pElement = element.inner();
    IUIAutomationTreeWalker* pTreeWalker = nullptr;
    if (SUCCEEDED(pAutomation->get_ControlViewWalker(&pTreeWalker))) {
        UIA_HWND hwnd = nullptr;
        do {
            pTreeWalker->GetParentElement(pElement, &pParent);
            pParent->get_CurrentNativeWindowHandle(&hwnd);
            pElement = pParent;
        } while (pElement && !hwnd);
        pTreeWalker->Release();
    }
    return UIElement{pParent};
}

void UIAutomation::cleanup() {
    if (pAutomation) {
        pAutomation->Release();
        pAutomation = nullptr;
    }
}

QString UIElement::getName() const {
    if (!pElement) return {};

    QString res;
    BSTR name;
    if (auto hr = pElement->get_CurrentName(&name); SUCCEEDED(hr)) {
        res = QString::fromWCharArray(name);
        SysFreeString(name);
    } else {
        qWarning() << "Failed to get name." << hr;
    }
    return res;
}

QString UIElement::getClassName() const {
    if (!pElement) return {};

    QString res;
    BSTR className;
    if (auto hr = pElement->get_CurrentClassName(&className); SUCCEEDED(hr)) {
        res = QString::fromWCharArray(className);
        SysFreeString(className);
    } else {
        qWarning() << "Failed to get class name." << hr;
    }
    return res;
}

QRect UIElement::getBoundingRect() const {
    if (!pElement) return {};

    RECT rect;
    pElement->get_CurrentBoundingRectangle(&rect);
    return {rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top};
}

CONTROLTYPEID UIElement::getControlType() const {
    if (!pElement) return 0;

    CONTROLTYPEID type;
    pElement->get_CurrentControlType(&type);
    return type;
}

HWND UIElement::getNativeWindowHandle() const {
    if (!pElement) return nullptr;

    UIA_HWND hwnd;
    pElement->get_CurrentNativeWindowHandle(&hwnd);
    return (HWND) hwnd;
}

QString UIElement::getNativeWindowClass() const {
    if (!pElement) return {};

    HWND hwnd = getNativeWindowHandle();
    return Util::getClassName(hwnd);
}

QString UIElement::getSelfOrParentNativeWindowClass() const {
    if (!pElement) return {};

    HWND hwnd = getNativeWindowHandle();
    if (!hwnd)
        hwnd = UIAutomation::getParentWithHWND(*this).getNativeWindowHandle();
    return Util::getClassName(hwnd);
}

QString UIElement::getAutomationId() const {
    if (!pElement) return {};

    QString res;
    BSTR id;
    if (auto hr = pElement->get_CurrentAutomationId(&id); SUCCEEDED(hr)) {
        res = QString::fromWCharArray(id);
        SysFreeString(id);
    } else {
        qWarning() << "Failed to get automation id." << hr;
    }
    return res;
}
