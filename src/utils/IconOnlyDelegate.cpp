#include "utils/IconOnlyDelegate.h"

void IconOnlyDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen); //取消边框
    // option.rect.size() == QListWidgetItem::sizeHint()
    if (option.state & QStyle::State_Selected) {
        painter->setBrush(selectedColor);
        painter->drawRoundedRect(option.rect, radius, radius);
    }
    else if (option.state & QStyle::State_MouseOver) {
        painter->setBrush(hoverColor);
        painter->drawRoundedRect(option.rect, radius, radius);
    }

    // 居中绘制图标
    auto icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    if (!icon.isNull()) {
        QRect iconRect {{}, option.decorationSize}; // QListWidget::iconSize()
        iconRect.moveCenter(option.rect.center());
        icon.paint(painter, iconRect);
    }
}
