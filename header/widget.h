#ifndef WIN_SWITCHER_WIDGET_H
#define WIN_SWITCHER_WIDGET_H

#include <QWidget>
#include <Windows.h>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget {
Q_OBJECT
protected:
    void keyPressEvent(QKeyEvent *event) override;

    void keyReleaseEvent(QKeyEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

public:
    explicit Widget(QWidget *parent = nullptr);
    Q_INVOKABLE bool forceShow();
    HWND hWnd() { return (HWND)winId(); }
    bool isForeground() { return GetForegroundWindow() == hWnd(); }
    ~Widget() override;

private:
    Ui::Widget *ui;
};


#endif //WIN_SWITCHER_WIDGET_H
