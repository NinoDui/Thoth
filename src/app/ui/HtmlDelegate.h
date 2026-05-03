#pragma once

#include <QApplication>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTextDocument>

class HtmlDelegate : public QStyledItemDelegate {
   public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        painter->save();
        painter->setClipRect(opt.rect);

        QTextDocument doc;
        doc.setDefaultFont(opt.font);
        doc.setHtml(opt.text);
        doc.setTextWidth(opt.rect.width());
        opt.text.clear();

        QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        painter->translate(opt.rect.topLeft());
        QRect clip(0, 0, opt.rect.width(), opt.rect.height());
        doc.drawContents(painter, clip);
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QTextDocument doc;
        doc.setHtml(opt.text);
        doc.setTextWidth(opt.rect.width() > 0 ? opt.rect.width() : 400);
        return QSize(static_cast<int>(doc.idealWidth()), static_cast<int>(doc.size().height()));
    }
};
