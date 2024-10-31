#include "utils/IconOnlyDelegate.h"
#include "widget.h"

void IconOnlyDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen); //取消边框
    // option.rect.size() == QListWidgetItem::sizeHint()
    if (option.state & QStyle::State_Selected) {
        painter->setBrush(selectedColor);
        painter->drawRoundedRect(option.rect, radius, radius);
    } else if (option.state & QStyle::State_MouseOver) {
        painter->setBrush(hoverColor);
        painter->drawRoundedRect(option.rect, radius, radius);
    }

    // 居中绘制图标
    auto icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    if (!icon.isNull()) {
        QRect iconRect{{}, option.decorationSize}; // QListWidget::iconSize()
        iconRect.moveCenter(option.rect.center());
        icon.paint(painter, iconRect);
    }

    // draw badge
    auto num = qvariant_cast<WindowGroup>(index.data(Qt::UserRole)).windows.size();
    if (num > 1) {
        auto text = QString::number(num);
        const auto extraWidth = 8 * (text.size() - 1);
        constexpr auto R = 12;
        auto badgeCenter = option.rect.topRight() + QPoint(-(R + 3), R + 3);
        // extra Width for extra number
        auto badgeRect = QRect(badgeCenter + QPoint(-R - extraWidth, -R), QSize(2 * R + extraWidth, 2 * R));
        painter->setPen(QColor(200, 200, 200, 50));
        painter->setBrush(QColor(133, 114, 97)); // learn from iOS
        painter->drawRoundedRect(badgeRect, R, R);

        QFont font{"Microsoft YaHei"};
        font.setPointSizeF(12.8);
        font.setBold(true);
        painter->setFont(font);
        painter->setPen(QColor(214, 192, 171));
        painter->drawText(badgeRect, Qt::AlignCenter, text);
    }
}
