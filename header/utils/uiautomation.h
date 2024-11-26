#ifndef UIAUTOMATION_H
#define UIAUTOMATION_H

#include <UIAutomation.h>
#include <QString>
#include <QRect>
#include <QDebug>

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

    UIElement(const UIElement&) = delete;
    UIElement& operator=(const UIElement&) = delete; // 如果不实现又不delete，默认浅拷贝指针就寄了，直接卡死

    // 最好用std::unique_ptr管理，避免自己实现移动
    UIElement& operator=(UIElement&& other) noexcept {
        // 1. 检查自移动
        if (this != &other) {
            // 2. 释放当前对象的资源
            if (pElement) pElement->Release();

            // 3. 移动资源
            pElement = other.pElement;

            // 4. 重置源对象
            other.pElement = nullptr;
        }
        return *this;
    }

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
