#include "utils/uiautomation.h"
#include <windows.h>
#include <QDebug>
#include "utils/Util.h"

#pragma comment(lib, "Oleacc.lib")
#pragma comment(lib, "OleAut32.lib")


IUIAutomation* UIAutomation::pAutomation = nullptr;

bool UIAutomation::init() {
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        qCritical() << "Failed to initialize COM library.";
        return false;
    }

    pAutomation = nullptr;
    hr = CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER, IID_IUIAutomation, (void**) &pAutomation);
    if (FAILED(hr) || !pAutomation) {
        qCritical() << "Failed to create UIAutomation instance.";
        CoUninitialize();
        return false;
    }

    return true;
}

UIElement UIAutomation::getElementUnderMouse() {
    if (!pAutomation) {
        if (!init())
            return {};
    }

    POINT pt;
    GetCursorPos(&pt);
    IUIAutomationElement* pElement = nullptr;
    pAutomation->ElementFromPoint(pt, &pElement);
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
        CoUninitialize();
    }
}

QString UIElement::getName() const {
    if (!pElement) return {};

    BSTR name;
    pElement->get_CurrentName(&name);
    QString res = QString::fromWCharArray(name);
    SysFreeString(name);
    return res;
}

QString UIElement::getClassName() const {
    if (!pElement) return {};

    BSTR className;
    pElement->get_CurrentClassName(&className);
    QString res = QString::fromWCharArray(className);
    SysFreeString(className);
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

    BSTR id;
    pElement->get_CurrentAutomationId(&id);
    QString res = QString::fromWCharArray(id);
    SysFreeString(id);
    return res;
}
