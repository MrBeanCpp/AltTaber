#ifndef WIN_SWITCHER_KEYBOARDHOOKER_H
#define WIN_SWITCHER_KEYBOARDHOOKER_H

#include <Windows.h>
#include <QWidget>

class KeyboardHooker {
public:
    explicit KeyboardHooker(QWidget* _receiver);
    ~KeyboardHooker(); // RAII
    inline static QWidget* receiver = nullptr;

private:
    HHOOK h_keyboard = nullptr;
};


#endif //WIN_SWITCHER_KEYBOARDHOOKER_H
