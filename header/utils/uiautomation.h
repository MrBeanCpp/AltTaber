#ifndef UIAUTOMATION_H
#define UIAUTOMATION_H

#include <UIAutomation.h>
#include <QString>
#include <QRect>

class UIElement; // forward declaration
/// simplify UIAutomation Library
///
/// [UIAutomation](https://learn.microsoft.com/zh-cn/windows/win32/winauto/uiauto-uiautomationoverview)
/// [Inspect.exe](https://learn.microsoft.com/zh-cn/windows/win32/winauto/inspect-objects)
class UIAutomation final {
    UIAutomation() = delete;
    static bool init();
public:
    static UIElement getElementUnderMouse();
    // 获取第一个有HWND的父元素
    static UIElement getParentWithHWND(const UIElement& element);
    static void cleanup();

private:
    static IUIAutomation* pAutomation;
};

class UIElement {
public:
    UIElement() = default;

    explicit UIElement(IUIAutomationElement* pElement) : pElement(pElement) {}

    ~UIElement() {
        if (pElement) pElement->Release();
    }

    bool isValid() const { return pElement != nullptr; }

    IUIAutomationElement* inner() const { return pElement; }

    QString getName() const;
    QString getClassName() const;
    QRect getBoundingRect() const;
    CONTROLTYPEID getControlType() const; // e.g.`UIA_PaneControlTypeId`
    HWND getNativeWindowHandle() const;
    QString getNativeWindowClass() const;
    // 如果自身的HWND为空，则循环查找HWND不为空的父元素的ClassName
    QString getSelfOrParentNativeWindowClass() const;
    QString getAutomationId() const;

private:
    IUIAutomationElement* pElement = nullptr;
};

#endif // UIAUTOMATION_H
